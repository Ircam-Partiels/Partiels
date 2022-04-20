#include "AnlTrackCommandTarget.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::CommandTarget::CommandTarget(Director& director, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAcsr)
: mDirector(director)
, mTimeZoomAccessor(timeZoomAccessor)
, mTransportAccessor(transportAcsr)
{
    auto const updateCommandManager = []()
    {
        auto* commandManager = Misc::App::getApplicationCommandManager();
        MiscWeakAssert(commandManager != nullptr);
        if(commandManager != nullptr)
        {
            commandManager->commandStatusChanged();
        }
    };

    mListener.onAttrChanged = [=](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        updateCommandManager();
    };

    mTimeZoomListener.onAttrChanged = [=](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        updateCommandManager();
    };

    mTransportListener.onAttrChanged = [=](Transport::Accessor const& acsr, Transport::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        updateCommandManager();
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mTimeZoomAccessor.addListener(mTimeZoomListener, NotificationType::synchronous);
    mTransportAccessor.addListener(mTransportListener, NotificationType::synchronous);
}

Track::CommandTarget::~CommandTarget()
{
    mAccessor.removeListener(mListener);
    mTimeZoomAccessor.removeListener(mTimeZoomListener);
    mTransportAccessor.removeListener(mTransportListener);
}

void Track::CommandTarget::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    // clang-format off
    commands.addArray
    ({
          CommandIDs::editSelectAll
        , CommandIDs::editDeleteSelection
        , CommandIDs::editCopySelection
        , CommandIDs::editCutSelection
        , CommandIDs::editPasteSelection
        , CommandIDs::editDuplicateSelection
    });
    // clang-format on
}

void Track::CommandTarget::getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result)
{
    auto const isActive = [this]()
    {
        if(Tools::getDisplayType(mAccessor) == Tools::DisplayType::columns)
        {
            return false;
        }
        if(!mAccessor.getAttr<AttrType::focused>())
        {
            return false;
        }
        if(auto* comp = dynamic_cast<juce::Component const*>(this))
        {
            return comp->hasKeyboardFocus(true);
        }
        return false;
    };

    switch(commandID)
    {
        case CommandIDs::editSelectAll:
        {
            auto const selection = mTransportAccessor.getAttr<Transport::AttrType::selection>();
            auto const globalRange = mTimeZoomAccessor.getAttr<Zoom::AttrType::globalRange>();
            result.setInfo(juce::translate("Select All"), juce::translate("Select All"), "Edit", 0);
            result.setActive(isActive() && selection != globalRange);
            result.defaultKeypresses.add(juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0));
        }
        break;
        case CommandIDs::editDeleteSelection:
        {
            result.setInfo(juce::translate("Delete Selection"), juce::translate("Delete Selection"), "Edit", 0);
            auto const selection = mTransportAccessor.getAttr<Transport::AttrType::selection>();
            result.setActive(isActive() && !Result::Modifier::getIndices(mAccessor, 0_z, selection).empty());
            result.defaultKeypresses.add(juce::KeyPress(0x08, juce::ModifierKeys::noModifiers, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::backspaceKey, juce::ModifierKeys::noModifiers, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::deleteKey, juce::ModifierKeys::noModifiers, 0));
        }
        break;
        case CommandIDs::editCopySelection:
        {
            result.setInfo(juce::translate("Copy Selection"), juce::translate("Copy Selection"), "Edit", 0);
            auto const selection = mTransportAccessor.getAttr<Transport::AttrType::selection>();
            result.setActive(isActive() && !Result::Modifier::getIndices(mAccessor, 0_z, selection).empty());
            result.defaultKeypresses.add(juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0));
        }
        break;
        case CommandIDs::editCutSelection:
        {
            result.setInfo(juce::translate("Cut Selection"), juce::translate("Cut Selection"), "Edit", 0);
            auto const selection = mTransportAccessor.getAttr<Transport::AttrType::selection>();
            result.setActive(isActive() && !Result::Modifier::getIndices(mAccessor, 0_z, selection).empty());
            result.defaultKeypresses.add(juce::KeyPress('x', juce::ModifierKeys::commandModifier, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::deleteKey, juce::ModifierKeys::shiftModifier, 0));
        }
        break;
        case CommandIDs::editPasteSelection:
        {
            result.setInfo(juce::translate("Paste Selection"), juce::translate("Paste Selection"), "Edit", 0);
            result.setActive(isActive() && !Result::Modifier::getIndices(mCopiedData).empty());
            result.defaultKeypresses.add(juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::insertKey, juce::ModifierKeys::shiftModifier, 0));
        }
        break;
        case CommandIDs::editDuplicateSelection:
        {
            result.setInfo(juce::translate("Duplicate Selection"), juce::translate("Duplicate Selection"), "Edit", 0);
            auto const selection = mTransportAccessor.getAttr<Transport::AttrType::selection>();
            result.setActive(isActive() && !Result::Modifier::getIndices(mAccessor, 0_z, selection).empty());
            result.defaultKeypresses.add(juce::KeyPress('d', juce::ModifierKeys::commandModifier, 0));
        }
        break;
    }
}

bool Track::CommandTarget::perform(juce::ApplicationCommandTarget::InvocationInfo const& info)
{
    auto constexpr defaultChannel = 0_z;
    switch(info.commandID)
    {
        case CommandIDs::editSelectAll:
        {
            auto const globalRange = mTimeZoomAccessor.getAttr<Zoom::AttrType::globalRange>();
            mTransportAccessor.setAttr<Transport::AttrType::selection>(globalRange, NotificationType::synchronous);
            return true;
        }
        case CommandIDs::editDeleteSelection:
        {
            auto action = std::make_unique<Result::Modifier::ActionErase>(mAccessor, mTransportAccessor, defaultChannel);
            if(action != nullptr)
            {
                mDirector.getUndoManager().beginNewTransaction(juce::translate("Erase Frames"));
                mDirector.getUndoManager().perform(action.release());
            }
            return true;
        }
        case CommandIDs::editCopySelection:
        {
            auto const selection = mTransportAccessor.getAttr<Transport::AttrType::selection>();
            auto const indices = Result::Modifier::getIndices(mAccessor, defaultChannel, selection);
            if(indices.empty())
            {
                return true;
            }
            mCopiedData = Result::Modifier::copyFrames(mAccessor, defaultChannel, indices);
            mCopiedSelection = selection;
            return true;
        }
        case CommandIDs::editCutSelection:
        {
            auto const selection = mTransportAccessor.getAttr<Transport::AttrType::selection>();
            auto const indices = Result::Modifier::getIndices(mAccessor, defaultChannel, selection);
            if(indices.empty())
            {
                return true;
            }
            mCopiedData = Result::Modifier::copyFrames(mAccessor, defaultChannel, indices);
            auto action = std::make_unique<Result::Modifier::ActionErase>(mAccessor, mTransportAccessor, defaultChannel);
            if(action != nullptr)
            {
                mDirector.getUndoManager().beginNewTransaction(juce::translate("Cut Frames"));
                mDirector.getUndoManager().perform(action.release());
            }
            return true;
        }
        case CommandIDs::editPasteSelection:
        {
            auto const indices = Result::Modifier::getIndices(mCopiedData);
            if(indices.empty())
            {
                return true;
            }
            auto action = std::make_unique<Result::Modifier::ActionPaste>(mAccessor, mTransportAccessor, defaultChannel, mCopiedData, mCopiedSelection);
            if(action != nullptr)
            {
                mDirector.getUndoManager().beginNewTransaction(juce::translate("Paste Frames"));
                mDirector.getUndoManager().perform(action.release());
            }
            return true;
        }
        case CommandIDs::editDuplicateSelection:
        {
            auto const selection = mTransportAccessor.getAttr<Transport::AttrType::selection>();
            auto const indices = Result::Modifier::getIndices(mAccessor, defaultChannel, selection);
            if(indices.empty())
            {
                return true;
            }
            auto const playhead = mTransportAccessor.getAttr<Transport::AttrType::startPlayhead>();
            mCopiedData = Result::Modifier::copyFrames(mAccessor, defaultChannel, indices);
            mCopiedSelection = selection;
            mTransportAccessor.setAttr<Transport::AttrType::startPlayhead>(mCopiedSelection.getEnd(), NotificationType::synchronous);
            auto action = std::make_unique<Result::Modifier::ActionPaste>(mAccessor, mTransportAccessor, defaultChannel, mCopiedData, mCopiedSelection);
            if(action != nullptr)
            {
                mDirector.getUndoManager().beginNewTransaction(juce::translate("Duplicate Frames"));
                if(!mDirector.getUndoManager().perform(action.release()))
                {
                    mTransportAccessor.setAttr<Transport::AttrType::startPlayhead>(playhead, NotificationType::synchronous);
                    MiscWeakAssert(false);
                }
            }
            return true;
        }
    }
    return false;
}

ANALYSE_FILE_END
