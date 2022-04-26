#include "AnlDocumentCommandTarget.h"
#include "../Group/AnlGroupTools.h"
#include "../Track/AnlTrackTools.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

Document::CommandTarget::CommandTarget(Director& director, juce::ApplicationCommandManager& commandManager)
: mDirector(director)
, mCommandManager(commandManager)
{
    mListener.onAccessorInserted = [this](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr, index);
        switch(type)
        {
            case AcsrType::tracks:
            {
                mCommandManager.commandStatusChanged();
            }
            break;
            case AcsrType::groups:
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
        }
    };

    mListener.onAccessorErased = [this](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr, index);
        switch(type)
        {
            case AcsrType::tracks:
            {
                mCommandManager.commandStatusChanged();
            }
            break;
            case AcsrType::groups:
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
        }
    };

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        mCommandManager.commandStatusChanged();
    };

    mTransportListener.onAttrChanged = [this](Transport::Accessor const& acsr, Transport::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        mCommandManager.commandStatusChanged();
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::transport>().addListener(mTransportListener, NotificationType::synchronous);
    mCommandManager.registerAllCommandsForTarget(this);
}

Document::CommandTarget::~CommandTarget()
{
    mAccessor.removeListener(mListener);
    mAccessor.getAcsr<AcsrType::transport>().removeListener(mTransportListener);
}

void Document::CommandTarget::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    // clang-format off
    commands.addArray
    ({
          CommandIDs::selectAll
        , CommandIDs::editDelete
        , CommandIDs::editCopy
        , CommandIDs::editCut
        , CommandIDs::editPaste
        , CommandIDs::editDuplicate
    });
    // clang-format on
}

void Document::CommandTarget::getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result)
{
    auto const& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
    auto const& transportAcsr = mAccessor.getAcsr<AcsrType::transport>();

    auto const& selection = transportAcsr.getAttr<Transport::AttrType::selection>();
    auto const isSelectionEmpty = [&]()
    {
        auto const& trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
        for(auto const& trackAcsr : trackAcsrs)
        {
            if(Track::Tools::getDisplayType(trackAcsr.get()) != Track::Tools::DisplayType::columns)
            {
                auto const selectedChannels = getSelectedChannels(trackAcsr.get());
                for(auto const& channel : selectedChannels)
                {
                    if(!Track::Result::Modifier::getIndices(trackAcsr.get(), channel, selection).empty())
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    };

    auto const isClipboardEmpty = [&]()
    {
        auto const& trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
        for(auto const& trackAcsr : trackAcsrs)
        {
            if(Track::Tools::getDisplayType(trackAcsr.get()) != Track::Tools::DisplayType::columns)
            {
                auto const trackId = trackAcsr.get().getAttr<Track::AttrType::identifier>();
                auto const trackIt = mClipboardData.find(trackId);
                if(trackIt != mClipboardData.cend())
                {
                    auto const selectedChannels = getSelectedChannels(trackAcsr.get());
                    for(auto const& channel : selectedChannels)
                    {
                        auto const channelIt = trackIt->second.find(channel);
                        if(channelIt != trackIt->second.cend() && !Track::Result::Modifier::getIndices(channelIt->second).empty())
                        {
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    };

    auto const isModeActive = mAccessor.getAttr<AttrType::editMode>() == EditMode::frames;
    switch(commandID)
    {
        case CommandIDs::selectAll:
        {
            result.setInfo(juce::translate("Select All"), juce::translate("Select All"), "Select", 0);
            result.defaultKeypresses.add(juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0));
            result.setActive(isModeActive && timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>() != selection);
        }
        break;
        case CommandIDs::editDelete:
        {
            result.setInfo(juce::translate("Delete Frame(s)"), juce::translate("Delete Frame(s)"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress(0x08, juce::ModifierKeys::noModifiers, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::backspaceKey, juce::ModifierKeys::noModifiers, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::deleteKey, juce::ModifierKeys::noModifiers, 0));
            result.setActive(isModeActive && !isSelectionEmpty());
        }
        break;
        case CommandIDs::editCopy:
        {
            result.setInfo(juce::translate("Copy Frame(s)"), juce::translate("Copy Frame(s)"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0));
            result.setActive(isModeActive && !isSelectionEmpty());
        }
        break;
        case CommandIDs::editCut:
        {
            result.setInfo(juce::translate("Cut Frame(s)"), juce::translate("Cut Frame(s)"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('x', juce::ModifierKeys::commandModifier, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::deleteKey, juce::ModifierKeys::shiftModifier, 0));
            result.setActive(isModeActive && !isSelectionEmpty());
        }
        break;
        case CommandIDs::editPaste:
        {
            result.setInfo(juce::translate("Paste Frame(s)"), juce::translate("Paste Frame(s)"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::insertKey, juce::ModifierKeys::shiftModifier, 0));
            result.setActive(isModeActive && !isClipboardEmpty());
        }
        break;
        case CommandIDs::editDuplicate:
        {
            result.setInfo(juce::translate("Duplicate Frame(s)"), juce::translate("Duplicate Frame(s)"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('d', juce::ModifierKeys::commandModifier, 0));
            result.setActive(isModeActive && !isSelectionEmpty());
        }
        break;
    }
}

bool Document::CommandTarget::perform(juce::ApplicationCommandTarget::InvocationInfo const& info)
{
    class FocusRestorer
    : public juce::UndoableAction
    {
    public:
        FocusRestorer(Accessor& accessor)
        : mAccessor(accessor)
        {
            for(auto const& trackAcsr : mAccessor.getAcsrs<AcsrType::tracks>())
            {
                auto const& identifier = trackAcsr.get().getAttr<Track::AttrType::identifier>();
                mTrackFocus[identifier] = trackAcsr.get().getAttr<Track::AttrType::focused>();
            }
            for(auto const& groupAcsr : mAccessor.getAcsrs<AcsrType::groups>())
            {
                auto const& identifier = groupAcsr.get().getAttr<Group::AttrType::identifier>();
                mGroupFocus[identifier] = groupAcsr.get().getAttr<Group::AttrType::focused>();
            }
        }

        ~FocusRestorer() override = default;

        bool perform() override
        {
            for(auto const& trackAcsr : mAccessor.getAcsrs<AcsrType::tracks>())
            {
                auto const& identifier = trackAcsr.get().getAttr<Track::AttrType::identifier>();
                trackAcsr.get().setAttr<Track::AttrType::focused>(mTrackFocus[identifier], NotificationType::synchronous);
            }
            for(auto const& groupAcsr : mAccessor.getAcsrs<AcsrType::groups>())
            {
                auto const& identifier = groupAcsr.get().getAttr<Group::AttrType::identifier>();
                groupAcsr.get().setAttr<Group::AttrType::focused>(mGroupFocus[identifier], NotificationType::synchronous);
            }
            return true;
        }

        bool undo() override
        {
            return perform();
        }

    protected:
        Accessor& mAccessor;
        std::map<juce::String, Track::FocusInfo> mTrackFocus;
        std::map<juce::String, Group::FocusInfo> mGroupFocus;
    };

    using namespace Track::Result::Modifier;
    auto const performForAllTracks = [&](std::function<void(Track::Accessor & trackAcsr)> fn)
    {
        auto const& trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
        for(auto const& trackAcsr : trackAcsrs)
        {
            if(Track::Tools::getDisplayType(trackAcsr.get()) != Track::Tools::DisplayType::columns)
            {
                fn(trackAcsr.get());
            }
        }
    };

    auto& undoManager = mDirector.getUndoManager();
    auto& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
    auto& transportAcsr = mAccessor.getAcsr<AcsrType::transport>();

    auto const playhead = transportAcsr.getAttr<Transport::AttrType::startPlayhead>();
    auto const selection = transportAcsr.getAttr<Transport::AttrType::selection>();

    switch(info.commandID)
    {
        case CommandIDs::selectAll:
        {
            transportAcsr.setAttr<Transport::AttrType::selection>(timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), NotificationType::synchronous);
            return true;
        }

        case CommandIDs::editDelete:
        {
            undoManager.beginNewTransaction(juce::translate("Delete Frame(s)"));
            performForAllTracks([&](Track::Accessor& trackAcsr)
                                {
                                    for(auto const& index : getSelectedChannels(trackAcsr))
                                    {
                                        undoManager.perform(std::make_unique<ActionErase>(trackAcsr, index, selection).release());
                                    }
                                });
            undoManager.perform(std::make_unique<FocusRestorer>(mAccessor).release());
            undoManager.perform(std::make_unique<Transport::Action::Restorer>(transportAcsr, playhead, selection).release());
            return true;
        }
        case CommandIDs::editCopy:
        {
            mClipboardData.clear();
            mClipboardRange = selection;
            performForAllTracks([&](Track::Accessor& trackAcsr)
                                {
                                    auto const trackId = trackAcsr.getAttr<Track::AttrType::identifier>();
                                    auto const selectedChannels = getSelectedChannels(trackAcsr);
                                    auto& trackData = mClipboardData[trackId];
                                    for(auto const& index : selectedChannels)
                                    {
                                        auto const indices = getIndices(trackAcsr, index, selection);
                                        trackData[index] = copyFrames(trackAcsr, index, indices);
                                    }
                                });
            return true;
        }
        case CommandIDs::editCut:
        {
            undoManager.beginNewTransaction(juce::translate("Cut Frame(s)"));
            perform({CommandIDs::editCopy});
            perform({CommandIDs::editDelete});
            return true;
        }
        case CommandIDs::editPaste:
        {
            undoManager.beginNewTransaction(juce::translate("Paste Frame(s)"));
            performForAllTracks([&](Track::Accessor& trackAcsr)
                                {
                                    auto const trackId = trackAcsr.getAttr<Track::AttrType::identifier>();
                                    auto const trackIt = mClipboardData.find(trackId);
                                    if(trackIt == mClipboardData.cend())
                                    {
                                        return;
                                    }
                                    auto const trackData = trackIt->second;
                                    for(auto const& data : trackData)
                                    {
                                        undoManager.perform(std::make_unique<ActionPaste>(trackAcsr, data.first, mClipboardRange, data.second, playhead).release());
                                    }
                                });
            undoManager.perform(std::make_unique<FocusRestorer>(mAccessor).release());
            undoManager.perform(std::make_unique<Transport::Action::Restorer>(transportAcsr, playhead, selection.movedToStartAt(playhead)).release());
            return true;
        }
        case CommandIDs::editDuplicate:
        {
            perform({CommandIDs::editCopy});
            transportAcsr.setAttr<Transport::AttrType::startPlayhead>(mClipboardRange.getEnd(), NotificationType::synchronous);
            perform({CommandIDs::editPaste});
            return true;
        }
    }
    return false;
}

std::set<size_t> Document::CommandTarget::getSelectedChannels(Track::Accessor const& trackAcsr) const
{
    auto const trackId = trackAcsr.getAttr<Track::AttrType::identifier>();
    auto const numChannels = trackAcsr.getAttr<Track::AttrType::channelsLayout>().size();
    auto trackChannels = Track::Tools::getSelectedChannels(trackAcsr);

    auto const& groupAcsr = Tools::getGroupAcsrForTrack(mAccessor, trackId);
    auto const groupChannels = Group::Tools::getSelectedChannels(groupAcsr);
    for(auto const& groupChannel : groupChannels)
    {
        if(numChannels == 1_z)
        {
            trackChannels.insert(0_z);
            return trackChannels;
        }
        if(groupChannel < numChannels)
        {
            trackChannels.insert(groupChannel);
        }
    }
    return trackChannels;
}

ANALYSE_FILE_END
