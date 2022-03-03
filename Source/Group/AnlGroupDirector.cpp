#include "AnlGroupDirector.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

Group::Director::Director(Accessor& accessor, Track::MultiDirector& trackMultiDirector, juce::UndoManager& undoManager)
: mAccessor(accessor)
, mTrackMultiDirector(trackMultiDirector)
, mUndoManager(undoManager)
{
    mAccessor.onAttrUpdated = [&](AttrType attribute, NotificationType notification)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::height:
            case AttrType::colour:
            case AttrType::expanded:
            case AttrType::focused:
            case AttrType::zoomid:
                break;
            case AttrType::layout:
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
}

Group::Accessor& Group::Director::getAccessor()
{
    return mAccessor;
}

void Group::Director::updateTracks(NotificationType notification)
{
    auto trackAcsrs = mAccessor.getAttr<AttrType::tracks>();
    auto const layout = mAccessor.getAttr<AttrType::layout>();
    auto& zoomAcsr = mAccessor.getAcsr<AcsrType::zoom>();
    for(auto const& identifier : layout)
    {
        auto trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
        if(trackAcsr.has_value())
        {
            trackAcsr->get().setAttr<Track::AttrType::zoomAcsr>(std::ref(zoomAcsr), notification);
        }
    }
}

void Group::Director::startAction()
{
    mSavedState.copyFrom(mAccessor, NotificationType::synchronous);
}

void Group::Director::endAction(ActionState state, juce::String const& name)
{
    if(mAccessor.isEquivalentTo(mSavedState))
    {
        return;
    }

    class Action
    : public juce::UndoableAction
    {
    public:
        Action(Accessor& accessor, Accessor const& undoAcsr)
        : mAccessor(accessor)
        {
            mRedoAccessor.copyFrom(mAccessor, NotificationType::synchronous);
            mUndoAccessor.copyFrom(undoAcsr, NotificationType::synchronous);
        }

        ~Action() override = default;

        bool perform() override
        {
            mAccessor.copyFrom(mRedoAccessor, NotificationType::synchronous);
            return true;
        }

        bool undo() override
        {
            mAccessor.copyFrom(mUndoAccessor, NotificationType::synchronous);
            return true;
        }

    private:
        Accessor& mAccessor;
        Accessor mRedoAccessor;
        Accessor mUndoAccessor;
    };

    auto action = std::make_unique<Action>(mAccessor, mSavedState);
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
