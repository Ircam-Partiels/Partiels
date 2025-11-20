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

void Group::Director::startAction(bool includeTracks)
{
    mSavedState.copyFrom(mAccessor, NotificationType::synchronous);
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

void Group::Director::endAction(bool includeTracks, ActionState state, juce::String const& name)
{
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
        Action(std::function<Accessor&()> sar, Accessor const& undoAcsr)
        : mSafeAccessorRetrieverFn(std::move(sar))
        {
            mRedoAccessor.copyFrom(mSafeAccessorRetrieverFn(), NotificationType::synchronous);
            mUndoAccessor.copyFrom(undoAcsr, NotificationType::synchronous);
        }

        ~Action() override = default;

        bool perform() override
        {
            mSafeAccessorRetrieverFn().copyFrom(mRedoAccessor, NotificationType::synchronous);
            return true;
        }

        bool undo() override
        {
            mSafeAccessorRetrieverFn().copyFrom(mUndoAccessor, NotificationType::synchronous);
            return true;
        }

    private:
        std::function<Accessor&()> const mSafeAccessorRetrieverFn;
        Accessor mRedoAccessor;
        Accessor mUndoAccessor;
    };

    auto action = std::make_unique<Action>(getSafeAccessorFn(), mSavedState);
    if(action != nullptr)
    {
        switch(state)
        {
            case ActionState::abort:
            {
                action->undo();
            }
            break;
            case ActionState::newTransaction:
            {
                mUndoManager.beginNewTransaction(name);
                mUndoManager.perform(action.release());
            }
            break;
            case ActionState::continueTransaction:
            {
                mUndoManager.perform(action.release());
            }
            break;
        }
    }

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
