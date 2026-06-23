#include "AnlGroupDirector.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

Group::Director::Director(Accessor& accessor, Track::MultiDirector& trackMultiDirector, HierarchyManager& hierarchyManager, juce::UndoManager& undoManager)
: mAccessor(accessor)
, mTrackMultiDirector(trackMultiDirector)
, mHierarchyManager(hierarchyManager)
, mUndoManager(undoManager)
{
    mAccessor.onAttrUpdated = [&](AttrType attribute, NotificationType notification)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            {
                if(onIdentifierUpdated != nullptr)
                {
                    onIdentifierUpdated(notification);
                }
            }
            break;
            case AttrType::name:
            {
                if(onNameUpdated != nullptr)
                {
                    onNameUpdated(notification);
                }
            }
            break;
            case AttrType::height:
            case AttrType::colour:
            case AttrType::expanded:
            case AttrType::referenceid:
                break;
            case AttrType::focused:
            {
                if(onFocusUpdated != nullptr)
                {
                    onFocusUpdated(notification);
                }
            }
            break;
            case AttrType::layout:
            {
                if(onLayoutUpdated != nullptr)
                {
                    onLayoutUpdated(notification);
                }
                updateTracks(notification);
            }
            break;
            case AttrType::tracks:
            {
                updateTracks(notification);
            }
            break;
        }
    };
    resetSavedState(false);
}

Group::Director::~Director()
{
    mAccessor.onAttrUpdated = nullptr;
    mAccessor.onAccessorInserted = nullptr;
    mAccessor.onAccessorErased = nullptr;
    for(auto const& identifier : mAccessor.getAttr<AttrType::layout>())
    {
        auto trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
        if(trackAcsr.has_value())
        {
            trackAcsr->get().setAttr<Track::AttrType::zoomAcsr>(std::optional<std::reference_wrapper<Zoom::Accessor>>{}, NotificationType::synchronous);
        }
    }
}

Group::Accessor& Group::Director::getAccessor()
{
    return mAccessor;
}

Track::HierarchyManager& Group::Director::getHierarchyManager()
{
    return mHierarchyManager;
}

void Group::Director::updateTracks(NotificationType notification)
{
    auto& zoomAcsr = mAccessor.getAcsr<AcsrType::zoom>();
    for(auto const& identifier : mAccessor.getAttr<AttrType::layout>())
    {
        auto trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
        if(trackAcsr.has_value())
        {
            trackAcsr->get().setAttr<Track::AttrType::zoomAcsr>(std::ref(zoomAcsr), notification);
        }
    }
}

bool Group::Director::hasChanged(bool includeTracks) const
{
    if(includeTracks)
    {
        auto trackAcrs = Tools::getTrackAcsrs(mAccessor);
        for(auto& trackAcr : trackAcrs)
        {
            auto const& trackDirector = getTrackDirector(trackAcr.get().getAttr<Track::AttrType::identifier>());
            if(trackDirector.hasChanged())
            {
                return true;
            }
        }
    }
    return !mAccessor.isEquivalentTo(mSavedState);
}

void Group::Director::resetSavedState(bool includeTracks)
{
    mSavedState.copyFrom(mAccessor, NotificationType::synchronous);
    if(includeTracks)
    {
        auto trackAcrs = Tools::getTrackAcsrs(mAccessor);
        for(auto& trackAcr : trackAcrs)
        {
            auto& trackDirector = getTrackDirector(trackAcr.get().getAttr<Track::AttrType::identifier>());
            trackDirector.resetSavedState();
        }
    }
}

bool Group::Director::isPerformingAction() const
{
    return mIsPerformingAction;
}

void Group::Director::startAction(bool includeTracks)
{
    MiscDebug("Group::Director", "startAction");
    MiscWeakAssert(mIsPerformingAction == false);
    if(!std::exchange(mIsPerformingAction, true))
    {
#if PARTIELS_DEBUG_DIFF
        MiscWeakAssert(!hasChanged(true));
        if(hasChanged(true))
        {
            MiscDebug("Group::Director", mAccessor.getDiff(mSavedState).dump());
        }
#endif
        resetSavedState(false);
        if(includeTracks)
        {
            auto trackAcrs = Tools::getTrackAcsrs(mAccessor);
            for(auto& trackAcr : trackAcrs)
            {
                auto& trackDirector = getTrackDirector(trackAcr.get().getAttr<Track::AttrType::identifier>());
                trackDirector.startAction();
            }
        }
    }
}

void Group::Director::endAction(bool includeTracks, ActionState state, juce::String const& name)
{
    MiscDebug("Group::Director", "endAction");
    MiscWeakAssert(mIsPerformingAction == true);
    mIsPerformingAction = false;
    if(!hasChanged(includeTracks))
    {
        if(includeTracks)
        {
            auto trackAcrs = Tools::getTrackAcsrs(mAccessor);
            for(auto& trackAcr : trackAcrs)
            {
                auto& trackDirector = getTrackDirector(trackAcr.get().getAttr<Track::AttrType::identifier>());
                trackDirector.endAction(ActionState::abort);
            }
        }
        return;
    }

    class Action
    : public juce::UndoableAction
    {
    public:
        Action(std::function<Accessor&()> sar, Accessor const& undoAcsr, std::function<void()> resetFn)
        : mSafeAccessorRetrieverFn(std::move(sar))
        , mResetFn(std::move(resetFn))
        {
            mRedoAccessor.copyFrom(mSafeAccessorRetrieverFn(), NotificationType::synchronous);
            mUndoAccessor.copyFrom(undoAcsr, NotificationType::synchronous);
        }

        ~Action() override = default;

        bool perform() override
        {
            mSafeAccessorRetrieverFn().copyFrom(mRedoAccessor, NotificationType::synchronous);
            if(mResetFn != nullptr)
            {
                mResetFn();
            }
            return true;
        }

        bool undo() override
        {
            mSafeAccessorRetrieverFn().copyFrom(mUndoAccessor, NotificationType::synchronous);
            if(mResetFn != nullptr)
            {
                mResetFn();
            }
            return true;
        }

    private:
        std::function<Accessor&()> const mSafeAccessorRetrieverFn;
        std::function<void()> mResetFn;
        Accessor mRedoAccessor;
        Accessor mUndoAccessor;
    };

    juce::WeakReference<Director> weakThis(this);
    auto action = std::make_unique<Action>(getSafeAccessorFn(), mSavedState, [weakThis]()
                                           {
                                               if(weakThis != nullptr)
                                               {
                                                   // No need to reset the saved state of tracks because it will be managed
                                                   // by the track directors
                                                   weakThis->resetSavedState(false);
                                               }
                                           });
    switch(state)
    {
        case ActionState::abort:
        {
            action->undo();
            if(includeTracks)
            {
                auto trackAcrs = Tools::getTrackAcsrs(mAccessor);
                for(auto& trackAcr : trackAcrs)
                {
                    auto& trackDirector = getTrackDirector(trackAcr.get().getAttr<Track::AttrType::identifier>());
                    trackDirector.endAction(ActionState::abort);
                }
            }
        }
        break;
        case ActionState::newTransaction:
        {
            mUndoManager.beginNewTransaction(name);
            mUndoManager.perform(action.release());
            if(includeTracks)
            {
                auto trackAcrs = Tools::getTrackAcsrs(mAccessor);
                for(auto& trackAcr : trackAcrs)
                {
                    auto& trackDirector = getTrackDirector(trackAcr.get().getAttr<Track::AttrType::identifier>());
                    trackDirector.endAction(ActionState::continueTransaction);
                }
            }
        }
        break;
        case ActionState::continueTransaction:
        {
            mUndoManager.perform(action.release());
            if(includeTracks)
            {
                auto trackAcrs = Tools::getTrackAcsrs(mAccessor);
                for(auto& trackAcr : trackAcrs)
                {
                    auto& trackDirector = getTrackDirector(trackAcr.get().getAttr<Track::AttrType::identifier>());
                    trackDirector.endAction(ActionState::continueTransaction);
                }
            }
        }
        break;
    }
}

std::function<Group::Accessor&()> Group::Director::getSafeAccessorFn()
{
    return mSafeAccessorRetrieverFn;
}

void Group::Director::setSafeAccessorRetriever(std::function<Accessor&()> const& sar)
{
    mSafeAccessorRetrieverFn = sar;
}

Track::Director const& Group::Director::getTrackDirector(juce::String const& identifier) const
{
    return mTrackMultiDirector.getTrackDirector(identifier);
}

Track::Director& Group::Director::getTrackDirector(juce::String const& identifier)
{
    return mTrackMultiDirector.getTrackDirector(identifier);
}

ANALYSE_FILE_END
