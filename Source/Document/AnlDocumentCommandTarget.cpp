#include "AnlDocumentCommandTarget.h"
#include "../Group/AnlGroupTools.h"
#include "../Track/AnlTrackExporter.h"
#include "../Track/AnlTrackTools.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

Document::CommandTarget::CommandTarget(Director& director, juce::ApplicationCommandManager& commandManager)
: mDirector(director)
, mCommandManager(commandManager)
{
    mListener.onAccessorInserted = [this]([[maybe_unused]] Accessor const& acsr, AcsrType type, [[maybe_unused]] size_t index)
    {
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

    mListener.onAccessorErased = [this]([[maybe_unused]] Accessor const& acsr, AcsrType type, [[maybe_unused]] size_t index)
    {
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

    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acsr, [[maybe_unused]] AttrType attribute)
    {
        mCommandManager.commandStatusChanged();
    };

    mTransportListener.onAttrChanged = [this]([[maybe_unused]] Transport::Accessor const& acsr, [[maybe_unused]] Transport::AttrType attribute)
    {
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
          CommandIDs::frameSelectAll
        , CommandIDs::frameDelete
        , CommandIDs::frameCopy
        , CommandIDs::frameCut
        , CommandIDs::framePaste
        , CommandIDs::frameDuplicate
        , CommandIDs::frameInsert
        , CommandIDs::frameBreak
        , CommandIDs::frameSystemCopy
    });
    // clang-format on
}

void Document::CommandTarget::getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result)
{
    auto const& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
    auto const& transportAcsr = mAccessor.getAcsr<AcsrType::transport>();
    auto const isSelectionEmpty = [&](juce::Range<double> const& selection)
    {
        auto const& trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
        for(auto const& trackAcsr : trackAcsrs)
        {
            if(Track::Tools::getFrameType(trackAcsr.get()) != Track::FrameType::vector)
            {
                auto const selectedChannels = Track::Tools::getSelectedChannels(trackAcsr.get());
                for(auto const& channel : selectedChannels)
                {
                    if(Track::Result::Modifier::containFrames(trackAcsr.get(), channel, selection))
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    };

    auto const matchTime = [&](double time)
    {
        auto const& trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
        for(auto const& trackAcsr : trackAcsrs)
        {
            if(Track::Tools::getFrameType(trackAcsr.get()) != Track::FrameType::vector)
            {
                auto const selectedChannels = Track::Tools::getSelectedChannels(trackAcsr.get());
                for(auto const& channel : selectedChannels)
                {
                    if(Track::Result::Modifier::matchFrame(trackAcsr.get(), channel, time))
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    };

    auto const canBreak = [&](double time)
    {
        auto const& trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
        for(auto const& trackAcsr : trackAcsrs)
        {
            if(Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::value)
            {
                auto const selectedChannels = Track::Tools::getSelectedChannels(trackAcsr.get());
                for(auto const& channel : selectedChannels)
                {
                    if(Track::Result::Modifier::canBreak(trackAcsr.get(), channel, time))
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    };

    auto const isClipboardEmpty = [&]()
    {
        auto const& trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
        for(auto const& trackAcsr : trackAcsrs)
        {
            if(Track::Tools::getFrameType(trackAcsr.get()) != Track::FrameType::vector)
            {
                auto const trackId = trackAcsr.get().getAttr<Track::AttrType::identifier>();
                auto const trackIt = mClipboardData.find(trackId);
                if(trackIt != mClipboardData.cend())
                {
                    auto const selectedChannels = Track::Tools::getSelectedChannels(trackAcsr.get());
                    for(auto const& channel : selectedChannels)
                    {
                        auto const channelIt = trackIt->second.find(channel);
                        if(channelIt != trackIt->second.cend() && !Track::Result::Modifier::isEmpty(channelIt->second))
                        {
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    };

    auto const& selection = transportAcsr.getAttr<Transport::AttrType::selection>();
    auto const isModeActive = mAccessor.getAttr<AttrType::editMode>() == EditMode::frames;
    switch(commandID)
    {
        case CommandIDs::frameSelectAll:
        {
            result.setInfo(juce::translate("Select All"), juce::translate("Select all frame(s)"), "Select", 0);
            result.defaultKeypresses.add(juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0));
            result.setActive(isModeActive && timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>() != selection);
            break;
        }
        case CommandIDs::frameDelete:
        {
            result.setInfo(juce::translate("Delete Frame(s)"), juce::translate("Delete the selected frame(s)"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress(0x08, juce::ModifierKeys::noModifiers, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::backspaceKey, juce::ModifierKeys::noModifiers, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::deleteKey, juce::ModifierKeys::noModifiers, 0));
            result.setActive(isModeActive && !isSelectionEmpty(selection));
            break;
        }
        case CommandIDs::frameCopy:
        {
            result.setInfo(juce::translate("Copy Frame(s)"), juce::translate("Copy the selected frame(s) to the application clipboard"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0));
            result.setActive(isModeActive && !isSelectionEmpty(selection));
            break;
        }
        case CommandIDs::frameCut:
        {
            result.setInfo(juce::translate("Cut Frame(s)"), juce::translate("Cut the selected frame(s) to the application clipboard"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('x', juce::ModifierKeys::commandModifier, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::deleteKey, juce::ModifierKeys::shiftModifier, 0));
            result.setActive(isModeActive && !isSelectionEmpty(selection));
            break;
        }
        case CommandIDs::framePaste:
        {
            result.setInfo(juce::translate("Paste Frame(s)"), juce::translate("Paste frame(s) from the application clipboard"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0));
            result.defaultKeypresses.add(juce::KeyPress(juce::KeyPress::insertKey, juce::ModifierKeys::shiftModifier, 0));
            result.setActive(isModeActive && !isClipboardEmpty());
            break;
        }
        case CommandIDs::frameDuplicate:
        {
            result.setInfo(juce::translate("Duplicate Frame(s)"), juce::translate("Duplicate the selected frame(s)"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('d', juce::ModifierKeys::commandModifier, 0));
            result.setActive(isModeActive && !isSelectionEmpty(selection));
            break;
        }
        case CommandIDs::frameInsert:
        {
            result.setInfo(juce::translate("Insert Frame(s)"), juce::translate("Insert frame(s)"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('i', juce::ModifierKeys::noModifiers, 0));
            result.setActive(isModeActive && (!matchTime(selection.getStart()) || !matchTime(selection.getEnd())));
            break;
        }
        case CommandIDs::frameBreak:
        {
            result.setInfo(juce::translate("Break Frame(s)"), juce::translate("Break frame(s)"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('b', juce::ModifierKeys::noModifiers, 0));
            result.setActive(isModeActive && canBreak(selection.getStart()));
            break;
        }
        case CommandIDs::frameSystemCopy:
        {
            result.setInfo(juce::translate("Copy Frame(s) to System Clipboard"), juce::translate("Copy frame(s) to system clipboard"), "Edit", 0);
            result.defaultKeypresses.add(juce::KeyPress('c', juce::ModifierKeys::altModifier, 0));
            result.setActive(isModeActive && !isSelectionEmpty(selection));
            break;
        }
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

    auto const getTransportAcsr = [this]() -> Transport::Accessor&
    {
        return mAccessor.getAcsr<AcsrType::transport>();
    };

    using namespace Track::Result::Modifier;
    auto const performForAllTracks = [&](std::function<void(Track::Accessor & trackAcsr)> fn)
    {
        auto const& trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
        for(auto const& trackAcsr : trackAcsrs)
        {
            if(Track::Tools::getFrameType(trackAcsr.get()) != Track::FrameType::vector)
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
        case CommandIDs::frameSelectAll:
        {
            transportAcsr.setAttr<Transport::AttrType::selection>(timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), NotificationType::synchronous);
            return true;
        }

        case CommandIDs::frameDelete:
        {
            undoManager.beginNewTransaction(juce::translate("Delete Frame(s)"));
            performForAllTracks([&](Track::Accessor& trackAcsr)
                                {
                                    auto const trackId = trackAcsr.getAttr<Track::AttrType::identifier>();
                                    auto const fn = mDirector.getSafeTrackAccessorFn(trackId);
                                    for(auto const& index : Track::Tools::getSelectedChannels(trackAcsr))
                                    {
                                        undoManager.perform(std::make_unique<ActionErase>(fn, index, selection).release());
                                    }
                                });
            undoManager.perform(std::make_unique<FocusRestorer>(mAccessor).release());
            undoManager.perform(std::make_unique<Transport::Action::Restorer>(getTransportAcsr, playhead, selection).release());
            return true;
        }
        case CommandIDs::frameCopy:
        {
            mClipboardData.clear();
            mClipboardRange = selection;
            performForAllTracks([&](Track::Accessor& trackAcsr)
                                {
                                    auto const trackId = trackAcsr.getAttr<Track::AttrType::identifier>();
                                    auto const selectedChannels = Track::Tools::getSelectedChannels(trackAcsr);
                                    auto& trackData = mClipboardData[trackId];
                                    for(auto const& index : selectedChannels)
                                    {
                                        trackData[index] = copyFrames(trackAcsr, index, selection);
                                    }
                                });
            return true;
        }
        case CommandIDs::frameCut:
        {
            undoManager.beginNewTransaction(juce::translate("Cut Frame(s)"));
            perform({CommandIDs::frameCopy});
            perform({CommandIDs::frameDelete});
            return true;
        }
        case CommandIDs::framePaste:
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
                                    auto const fn = mDirector.getSafeTrackAccessorFn(trackId);
                                    auto const trackData = trackIt->second;
                                    for(auto const& data : trackData)
                                    {
                                        undoManager.perform(std::make_unique<ActionPaste>(fn, data.first, data.second, playhead).release());
                                    }
                                });
            undoManager.perform(std::make_unique<FocusRestorer>(mAccessor).release());
            undoManager.perform(std::make_unique<Transport::Action::Restorer>(getTransportAcsr, playhead, selection.movedToStartAt(playhead)).release());
            return true;
        }
        case CommandIDs::frameDuplicate:
        {
            perform({CommandIDs::frameCopy});
            transportAcsr.setAttr<Transport::AttrType::startPlayhead>(mClipboardRange.getEnd(), NotificationType::synchronous);
            perform({CommandIDs::framePaste});
            return true;
        }
        case CommandIDs::frameInsert:
        {
            auto const insertFrame = [&](Track::Accessor& trackAcsr, size_t const channel, double const time)
            {
                if(!Track::Result::Modifier::matchFrame(trackAcsr, channel, time))
                {
                    auto const trackId = trackAcsr.getAttr<Track::AttrType::identifier>();
                    auto const fn = mDirector.getSafeTrackAccessorFn(trackId);
                    auto const data = Track::Result::Modifier::createFrame(trackAcsr, channel, time);
                    undoManager.perform(std::make_unique<ActionPaste>(fn, channel, data, time).release());
                }
            };
            auto const isPlaying = transportAcsr.getAttr<Transport::AttrType::playback>();
            auto const runningPlayhead = transportAcsr.getAttr<Transport::AttrType::runningPlayhead>();
            undoManager.beginNewTransaction(juce::translate("Insert Frame(s)"));
            performForAllTracks([&](Track::Accessor& trackAcsr)
                                {
                                    auto const selectedChannels = Track::Tools::getSelectedChannels(trackAcsr);
                                    for(auto const& channel : selectedChannels)
                                    {
                                        if(isPlaying)
                                        {
                                            insertFrame(trackAcsr, channel, runningPlayhead);
                                        }
                                        else
                                        {
                                            insertFrame(trackAcsr, channel, selection.getStart());
                                            if(!selection.isEmpty())
                                            {
                                                insertFrame(trackAcsr, channel, selection.getEnd());
                                            }
                                        }
                                    }
                                });
            undoManager.perform(std::make_unique<FocusRestorer>(mAccessor).release());
            undoManager.perform(std::make_unique<Transport::Action::Restorer>(getTransportAcsr, playhead, selection.movedToStartAt(playhead)).release());
            return true;
        }
        case CommandIDs::frameBreak:
        {
            auto const breakFrame = [&](Track::Accessor& trackAcsr, size_t const channel, double const time)
            {
                if(Track::Result::Modifier::canBreak(trackAcsr, channel, time))
                {
                    auto const trackId = trackAcsr.getAttr<Track::AttrType::identifier>();
                    auto const fn = mDirector.getSafeTrackAccessorFn(trackId);
                    Track::Result::ChannelData const data = std::vector<Track::Result::Data::Point>{{time, 0.0, std::optional<float>{}, std::vector<float>{}}};
                    undoManager.perform(std::make_unique<ActionPaste>(fn, channel, data, time).release());
                }
            };
            auto const isPlaying = transportAcsr.getAttr<Transport::AttrType::playback>();
            auto const runningPlayhead = transportAcsr.getAttr<Transport::AttrType::runningPlayhead>();
            undoManager.beginNewTransaction(juce::translate("Break Frame(s)"));
            performForAllTracks([&](Track::Accessor& trackAcsr)
                                {
                                    if(Track::Tools::getFrameType(trackAcsr) == Track::FrameType::value)
                                    {
                                        auto const selectedChannels = Track::Tools::getSelectedChannels(trackAcsr);
                                        for(auto const& channel : selectedChannels)
                                        {
                                            if(isPlaying)
                                            {
                                                breakFrame(trackAcsr, channel, runningPlayhead);
                                            }
                                            else
                                            {
                                                breakFrame(trackAcsr, channel, selection.getStart());
                                            }
                                        }
                                    }
                                });
            undoManager.perform(std::make_unique<FocusRestorer>(mAccessor).release());
            undoManager.perform(std::make_unique<Transport::Action::Restorer>(getTransportAcsr, playhead, selection.movedToStartAt(playhead)).release());
            return true;
        }
        case CommandIDs::frameSystemCopy:
        {
            auto const& trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
            for(auto const& trackAcsr : trackAcsrs)
            {
                if(Track::Tools::getFrameType(trackAcsr.get()) != Track::FrameType::vector)
                {
                    std::atomic<bool> shouldAbort{false};
                    auto const selectedChannels = Track::Tools::getSelectedChannels(trackAcsr);
                    if(!selectedChannels.empty())
                    {
                        juce::String clipboardResults;
                        Track::Exporter::toJson(trackAcsr.get(), selection, selectedChannels, clipboardResults, false, shouldAbort);
                        juce::SystemClipboard::copyTextToClipboard(clipboardResults);
                        return true;
                    }
                }
            }
            return true;
        }
    }
    return false;
}

ANALYSE_FILE_END
