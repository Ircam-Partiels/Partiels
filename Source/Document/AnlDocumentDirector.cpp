#include "AnlDocumentDirector.h"
#include "../Group/AnlGroupTools.h"
#include "../Zoom/AnlZoomTools.h"
#include "AnlDocumentAudioReader.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

Document::Director::Director(Accessor& accessor, juce::AudioFormatManager& audioFormatManager, juce::UndoManager& undoManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
, mUndoManager(undoManager)
{
    mAccessor.onAttrUpdated = [&](AttrType attribute, NotificationType notification)
    {
        switch(attribute)
        {
            case AttrType::reader:
            {
                clearFilesToWatch();
                auto reader = mAccessor.getAttr<AttrType::reader>();
                for(auto const& channelLayout : reader)
                {
                    auto const file = channelLayout.file;
                    if(file != juce::File() && !file.existsAsFile())
                    {
                        if(AlertWindow::showOkCancel(AlertWindow::MessageType::warning, "Audio file cannot be found!", "The audio file FILENAME has been moved or deleted. Would you like to restore  it?", {{"FILENAME", file.getFullPathName()}}))
                        {
                            auto const audioFormatWildcard = mAudioFormatManager.getWildcardForAllFormats();
                            juce::FileChooser fc(juce::translate("Restore the audio file..."), file, audioFormatWildcard);
                            if(fc.browseForFileToOpen())
                            {
                                auto const newFile = fc.getResult();
                                for(auto& copyChannelLayout : reader)
                                {
                                    if(copyChannelLayout.file == file)
                                    {
                                        copyChannelLayout.file = newFile;
                                    }
                                }
                                mAccessor.setAttr<AttrType::reader>(reader, notification);
                                return;
                            }
                        }
                    }
                    addFileToWatch(file);
                }
                initializeAudioReaders(notification);
            }
            break;
            case AttrType::grid:
            {
                auto const mode = mAccessor.getAttr<AttrType::grid>();
                auto trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
                for(auto trackAcsr : trackAcsrs)
                {
                    trackAcsr.get().setAttr<Track::AttrType::grid>(mode, notification);
                }
            }
            break;
            case AttrType::samplerate:
            {
                if(mSampleRate.has_value())
                {
                    mAccessor.setAttr<AttrType::samplerate>(*mSampleRate, notification);
                }
            }
            break;
            case AttrType::layout:
            case AttrType::viewport:
            case AttrType::path:
                break;
        }
    };

    mAccessor.onAccessorInserted = [&](AcsrType type, size_t index, NotificationType notification)
    {
        juce::ignoreUnused(notification);
        switch(type)
        {
            case AcsrType::tracks:
            {
                auto trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
                anlStrongAssert(index <= mTracks.size() && index < trackAcsrs.size());
                if(index > mTracks.size() || index >= trackAcsrs.size())
                {
                    return;
                }
                auto& trackAcsr = trackAcsrs[index].get();
                trackAcsr.setAttr<Track::AttrType::grid>(mAccessor.getAttr<AttrType::grid>(), notification);
                auto director = std::make_unique<Track::Director>(trackAcsr, mUndoManager, std::get<0>(createAudioFormatReader(mAccessor, mAudioFormatManager)));
                anlStrongAssert(director != nullptr);
                if(director != nullptr)
                {
                    director->setAlertCatcher(mAlertCatcher);
                    director->onIdentifierUpdated = [this](NotificationType localNotification)
                    {
                        for(auto& group : mGroups)
                        {
                            if(group != nullptr)
                            {
                                group->updateTracks(localNotification);
                            }
                        }
                    };
                }
                mTracks.insert(mTracks.begin() + static_cast<long>(index), std::move(director));

                auto groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
                for(auto& groupAcsr : groupAcsrs)
                {
                    groupAcsr.get().setAttr<Group::AttrType::tracks>(trackAcsrs, notification);
                }
            }
            break;
            case AcsrType::groups:
            {
                auto const groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
                anlStrongAssert(index < groupAcsrs.size());
                if(index > mGroups.size() || index >= groupAcsrs.size())
                {
                    return;
                }
                auto& groupAcsr = groupAcsrs[index].get();
                auto director = std::make_unique<Group::Director>(groupAcsr, *this, mUndoManager);
                anlStrongAssert(director != nullptr);
                mGroups.insert(mGroups.begin() + static_cast<long>(index), std::move(director));

                groupAcsr.setAttr<Group::AttrType::tracks>(mAccessor.getAcsrs<AcsrType::tracks>(), notification);
            }
            case AcsrType::transport:
            case AcsrType::timeZoom:
                break;
        }
    };

    mAccessor.onAccessorErased = [&](AcsrType type, size_t index, NotificationType notification)
    {
        juce::ignoreUnused(notification);
        switch(type)
        {
            case AcsrType::tracks:
            {
                anlStrongAssert(index < mTracks.size());
                if(index >= mTracks.size())
                {
                    return;
                }
                mTracks.erase(mTracks.begin() + static_cast<long>(index));

                auto trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
                auto groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
                for(auto& groupAcsr : groupAcsrs)
                {
                    groupAcsr.get().setAttr<Group::AttrType::tracks>(trackAcsrs, notification);
                }
            }
            break;
            case AcsrType::groups:
            {
                anlStrongAssert(index < mGroups.size());
                if(index >= mGroups.size())
                {
                    return;
                }
                mGroups.erase(mGroups.begin() + static_cast<long>(index));
            }
            case AcsrType::transport:
            case AcsrType::timeZoom:
                break;
        }
    };

    auto& transportAcsr = mAccessor.getAcsr<AcsrType::transport>();
    transportAcsr.onAttrUpdated = [&](Transport::AttrType attribute, NotificationType notification)
    {
        auto& zoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
        auto updateTime = [&](double time)
        {
            auto const range = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            if(!range.contains(time))
            {
                zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range.movedToStartAt(time), notification);
            }
        };
        switch(attribute)
        {
            case Transport::AttrType::playback:
                break;
            case Transport::AttrType::startPlayhead:
            {
                auto const time = transportAcsr.getAttr<Transport::AttrType::startPlayhead>();
                if(!transportAcsr.getAttr<Transport::AttrType::playback>())
                {
                    updateTime(time);
                }
                zoomAcsr.setAttr<Zoom::AttrType::anchor>(std::make_tuple(false, time), notification);
            }
            break;
            case Transport::AttrType::runningPlayhead:
            {
                auto const time = transportAcsr.getAttr<Transport::AttrType::runningPlayhead>();
                updateTime(time);
            }
            break;
            case Transport::AttrType::looping:
            case Transport::AttrType::loopRange:
            case Transport::AttrType::gain:
                break;
        }
    };

    auto& zoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
    zoomAcsr.onAttrUpdated = [&](Zoom::AttrType attribute, NotificationType notification)
    {
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            {
                zoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{0.0, mDuration}, notification);
            }
            break;
            case Zoom::AttrType::minimumLength:
            {
                auto const sampleRate = mAccessor.getAttr<AttrType::samplerate>();
                zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(sampleRate > 0.0 ? 512.0 / sampleRate : mDuration, notification);
            }
            break;
            case Zoom::AttrType::visibleRange:
            case Zoom::AttrType::anchor:
                break;
        }
    };

    auto& gridAcsr = zoomAcsr.getAcsr<Zoom::AcsrType::grid>();
    gridAcsr.onAttrUpdated = [&](Zoom::Grid::AttrType attribute, NotificationType notification)
    {
        juce::ignoreUnused(attribute);
        gridAcsr.setAttr<Zoom::Grid::AttrType::tickReference>(0.0, notification);
        gridAcsr.setAttr<Zoom::Grid::AttrType::mainTickInterval>(0_z, notification);
        gridAcsr.setAttr<Zoom::Grid::AttrType::tickPowerBase>(2.0, notification);
        gridAcsr.setAttr<Zoom::Grid::AttrType::tickDivisionFactor>(10.0, notification);
    };
    gridAcsr.onAttrUpdated(Zoom::Grid::AttrType::tickReference, NotificationType::synchronous);
}

Document::Director::~Director()
{
    auto& zoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
    zoomAcsr.onAttrUpdated = nullptr;
    mAccessor.onAttrUpdated = nullptr;
    mAccessor.onAccessorInserted = nullptr;
    mAccessor.onAccessorErased = nullptr;
}

Document::Accessor& Document::Director::getAccessor()
{
    return mAccessor;
}

juce::AudioFormatManager& Document::Director::getAudioFormatManager()
{
    return mAudioFormatManager;
}

Group::Director& Document::Director::getGroupDirector(juce::String const& identifier)
{
    auto it = std::find_if(mGroups.begin(), mGroups.end(), [&](auto const& groupDirector)
                           {
                               auto const& groupAcsr = groupDirector->getAccessor();
                               return groupAcsr.template getAttr<Group::AttrType::identifier>() == identifier;
                           });
    anlStrongAssert(it != mGroups.end());
    return *it->get();
}

Track::Director& Document::Director::getTrackDirector(juce::String const& identifier)
{
    auto it = std::find_if(mTracks.begin(), mTracks.end(), [&](auto const& trackDirector)
                           {
                               auto const& trackAcsr = trackDirector->getAccessor();
                               return trackAcsr.template getAttr<Track::AttrType::identifier>() == identifier;
                           });
    anlStrongAssert(it != mTracks.end());
    return *it->get();
}

void Document::Director::setAlertCatcher(AlertWindow::Catcher* catcher)
{
    if(mAlertCatcher != catcher)
    {
        mAlertCatcher = catcher;
        for(auto& track : mTracks)
        {
            if(track != nullptr)
            {
                track->setAlertCatcher(catcher);
            }
        }
    }
}

void Document::Director::startAction()
{
    mSavedState.copyFrom(mAccessor, NotificationType::synchronous);
}

void Document::Director::endAction(ActionState state, juce::String const& name)
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

std::optional<juce::String> Document::Director::addTrack(juce::String const groupIdentifer, size_t position, NotificationType const notification)
{
    anlStrongAssert(Tools::hasGroupAcsr(mAccessor, groupIdentifer));
    if(!Tools::hasGroupAcsr(mAccessor, groupIdentifer))
    {
        return std::optional<juce::String>();
    }

    auto const index = mAccessor.getNumAcsrs<AcsrType::tracks>();
    if(!mAccessor.insertAcsr<AcsrType::tracks>(index, notification))
    {
        return std::optional<juce::String>();
    }

    auto const identifier = juce::Uuid().toString();

    auto& trackAcsr = mAccessor.getAcsr<AcsrType::tracks>(index);
    trackAcsr.setAttr<Track::AttrType::identifier>(identifier, notification);

    auto& groupAcsr = Tools::getGroupAcsr(mAccessor, groupIdentifer);
    auto layout = groupAcsr.getAttr<Group::AttrType::layout>();
    anlStrongAssert(position <= layout.size());
    position = std::min(position, layout.size());
    layout.insert(layout.begin() + static_cast<long>(position), identifier);
    groupAcsr.setAttr<Group::AttrType::layout>(layout, NotificationType::synchronous);

    sanitize(notification);
    return std::optional<juce::String>(identifier);
}

bool Document::Director::removeTrack(juce::String const identifier, NotificationType const notification)
{
    auto const trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
    auto const it = std::find_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](Track::Accessor const& acsr)
                                 {
                                     return acsr.getAttr<Track::AttrType::identifier>() == identifier;
                                 });
    anlWeakAssert(it != trackAcsrs.cend());
    if(it == trackAcsrs.cend())
    {
        return false;
    }

    auto groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
    for(auto& groupAcsr : groupAcsrs)
    {
        auto const groupLayout = copy_with_erased(groupAcsr.get().getAttr<Group::AttrType::layout>(), identifier);
        groupAcsr.get().setAttr<Group::AttrType::layout>(groupLayout, NotificationType::synchronous);
    }

    auto const index = static_cast<size_t>(std::distance(trackAcsrs.cbegin(), it));
    mAccessor.eraseAcsr<AcsrType::tracks>(index, notification);

    sanitize(notification);
    return true;
}

std::optional<juce::String> Document::Director::addGroup(size_t position, NotificationType const notification)
{
    auto const index = mAccessor.getNumAcsrs<AcsrType::groups>();
    if(!mAccessor.insertAcsr<AcsrType::groups>(index, notification))
    {
        return std::optional<juce::String>();
    }

    auto const identifier = juce::Uuid().toString();
    auto const name = juce::String("Group ") + juce::String(index + 1_z);
    auto& groupAcsr = mAccessor.getAcsr<AcsrType::groups>(index);
    groupAcsr.setAttr<Group::AttrType::identifier>(identifier, notification);
    groupAcsr.setAttr<Group::AttrType::name>(name, notification);

    auto layout = mAccessor.getAttr<AttrType::layout>();
    anlStrongAssert(position <= layout.size());
    position = std::min(position, layout.size());
    layout.insert(layout.begin() + static_cast<long>(position), identifier);
    mAccessor.setAttr<AttrType::layout>(layout, notification);
    sanitize(notification);
    return std::optional<juce::String>(identifier);
}

bool Document::Director::removeGroup(juce::String const identifier, NotificationType const notification)
{
    auto const groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
    auto const it = std::find_if(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](Group::Accessor const& acsr)
                                 {
                                     return acsr.getAttr<Group::AttrType::identifier>() == identifier;
                                 });
    anlWeakAssert(it != groupAcsrs.cend());
    if(it == groupAcsrs.cend())
    {
        return false;
    }

    auto const groupLayout = it->get().getAttr<Group::AttrType::layout>();
    for(auto const& tarckIdentifier : groupLayout)
    {
        removeTrack(tarckIdentifier, notification);
    }

    auto const layout = copy_with_erased(mAccessor.getAttr<AttrType::layout>(), identifier);
    mAccessor.setAttr<AttrType::layout>(layout, notification);

    auto const index = static_cast<size_t>(std::distance(groupAcsrs.cbegin(), it));
    mAccessor.eraseAcsr<AcsrType::groups>(index, notification);

    sanitize(notification);
    return true;
}

bool Document::Director::moveTrack(juce::String const groupIdentifier, size_t index, juce::String const trackIdentifier, NotificationType const notification)
{
    anlStrongAssert(Tools::hasGroupAcsr(mAccessor, groupIdentifier));
    anlStrongAssert(Tools::hasTrackAcsr(mAccessor, trackIdentifier));
    if(!Tools::hasGroupAcsr(mAccessor, groupIdentifier) || !Tools::hasTrackAcsr(mAccessor, trackIdentifier))
    {
        return false;
    }
    // The track should already be in a group
    anlWeakAssert(Tools::isTrackInGroup(mAccessor, trackIdentifier));
    if(!Tools::isTrackInGroup(mAccessor, trackIdentifier))
    {
        return false;
    }

    // Remove track from previous group owner
    {
        auto& groupAcsr = Tools::getGroupAcsrForTrack(mAccessor, trackIdentifier);
        auto layout = copy_with_erased(groupAcsr.getAttr<Group::AttrType::layout>(), trackIdentifier);
        groupAcsr.setAttr<Group::AttrType::layout>(layout, notification);
    }

    // Add track to new group owner
    {
        auto& groupAcsr = Tools::getGroupAcsr(mAccessor, groupIdentifier);
        auto layout = copy_with_erased(groupAcsr.getAttr<Group::AttrType::layout>(), trackIdentifier);
        layout.insert(layout.cbegin() + static_cast<long>(index), trackIdentifier);
        groupAcsr.setAttr<Group::AttrType::layout>(layout, notification);
        groupAcsr.setAttr<Group::AttrType::expanded>(true, notification);
    }

    sanitize(notification);
    return true;
}

bool Document::Director::copyTrack(juce::String const groupIdentifier, size_t index, juce::String const trackIdentifier, NotificationType const notification)
{
    anlStrongAssert(Tools::hasGroupAcsr(mAccessor, groupIdentifier));
    anlStrongAssert(Tools::hasTrackAcsr(mAccessor, trackIdentifier));
    if(!Tools::hasGroupAcsr(mAccessor, groupIdentifier) || !Tools::hasTrackAcsr(mAccessor, trackIdentifier))
    {
        return false;
    }
    // The track should already be in a group
    anlWeakAssert(Tools::isTrackInGroup(mAccessor, trackIdentifier));
    if(!Tools::isTrackInGroup(mAccessor, trackIdentifier))
    {
        return false;
    }

    auto newTrackIdentifier = addTrack(groupIdentifier, index, NotificationType::synchronous);
    if(!newTrackIdentifier.has_value())
    {
        return false;
    }

    auto& newTrackAcsr = Tools::getTrackAcsr(mAccessor, *newTrackIdentifier);
    Track::Accessor copy;
    copy.copyFrom(Tools::getTrackAcsr(mAccessor, trackIdentifier), NotificationType::synchronous);
    copy.setAttr<Track::AttrType::identifier>(*newTrackIdentifier, NotificationType::synchronous);
    newTrackAcsr.copyFrom(copy, NotificationType::synchronous);
    sanitize(notification);
    return true;
}

void Document::Director::sanitize(NotificationType const notification)
{
    if(mAccessor.getNumAcsrs<AcsrType::tracks>() == 0_z && mAccessor.getNumAcsrs<AcsrType::groups>() == 0_z)
    {
        return;
    }

    // Ensures all tracks and groups have a unique identifier
    {
        std::set<juce::String> identifiers;
        auto trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
        for(auto trackAcsr : trackAcsrs)
        {
            auto const identifier = trackAcsr.get().getAttr<Track::AttrType::identifier>();
            anlWeakAssert(!identifier.isEmpty() && identifiers.count(identifier) == 0_z);
            if(identifier.isEmpty() || identifiers.count(identifier) > 0_z)
            {
                auto const newidentifier = juce::Uuid().toString();
                trackAcsr.get().setAttr<Track::AttrType::identifier>(newidentifier, NotificationType::synchronous);
                identifiers.insert(newidentifier);
            }
            else
            {
                identifiers.insert(identifier);
            }
        }

        auto groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
        for(auto groupAcsr : groupAcsrs)
        {
            auto const identifier = groupAcsr.get().getAttr<Group::AttrType::identifier>();
            anlWeakAssert(!identifier.isEmpty() && identifiers.count(identifier) == 0_z);
            if(identifier.isEmpty() || identifiers.count(identifier) > 0_z)
            {
                auto const newidentifier = juce::Uuid().toString();
                groupAcsr.get().setAttr<Group::AttrType::identifier>(newidentifier, NotificationType::synchronous);
                identifiers.insert(newidentifier);
            }
            else
            {
                identifiers.insert(identifier);
            }
        }
    }

    // Ensures that there is also at least one group if there is one or more tracks
    {
        auto const trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
        anlWeakAssert(trackAcsrs.empty() || mAccessor.getNumAcsrs<AcsrType::groups>() != 0_z);
        if(!trackAcsrs.empty() && mAccessor.getNumAcsrs<AcsrType::groups>() == 0_z)
        {
            auto const identifier = addGroup(0, notification);
            anlStrongAssert(identifier.has_value());
            if(!identifier.has_value())
            {
                return;
            }
        }
    }

    // Ensures that groups' layouts are valid
    {
        auto groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
        for(auto groupAcsr : groupAcsrs)
        {
            auto const groupLayout = copy_with_erased_if(groupAcsr.get().getAttr<Group::AttrType::layout>(), [&](auto const identifier)
                                                         {
                                                             return !Tools::hasTrackAcsr(mAccessor, identifier);
                                                         });
            anlWeakAssert(groupAcsr.get().getAttr<Group::AttrType::layout>().size() == groupLayout.size());
            groupAcsr.get().setAttr<Group::AttrType::layout>(groupLayout, notification);
        }
    }

    // Ensures that all tracks are in one group
    if(mAccessor.getNumAcsrs<AcsrType::groups>() != 0_z)
    {
        auto groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
        auto const trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
        auto& lastAcsr = groupAcsrs.back().get();
        for(auto const& trackAcsr : trackAcsrs)
        {
            auto const trackIdentifier = trackAcsr.get().getAttr<Track::AttrType::identifier>();
            auto numGroupds = std::count_if(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](auto const& groupAcsr)
                                            {
                                                return Group::Tools::hasTrackAcsr(groupAcsr.get(), trackIdentifier);
                                            });
            anlWeakAssert(numGroupds == 1l);
            if(numGroupds == 0l)
            {
                auto groupLayout = lastAcsr.getAttr<Group::AttrType::layout>();
                groupLayout.push_back(trackIdentifier);
                lastAcsr.setAttr<Group::AttrType::layout>(groupLayout, notification);
            }
            while(numGroupds > 1l)
            {
                auto groupIt = std::find_if(groupAcsrs.begin(), groupAcsrs.end(), [&](auto const& groupAcsr)
                                            {
                                                return Group::Tools::hasTrackAcsr(groupAcsr.get(), trackIdentifier);
                                            });
                anlStrongAssert(groupIt != groupAcsrs.end());
                if(groupIt != groupAcsrs.end())
                {
                    auto const groupLayout = copy_with_erased(groupIt->get().getAttr<Group::AttrType::layout>(), trackIdentifier);
                    groupIt->get().setAttr<Group::AttrType::layout>(groupLayout, notification);
                    --numGroupds;
                }
            }
        }
    }

    // Ensures that document's layout is valid
    {
        anlWeakAssert(mAccessor.getNumAcsrs<AcsrType::groups>() == mAccessor.getAttr<AttrType::layout>().size());
        auto const layout = copy_with_erased_if(mAccessor.getAttr<AttrType::layout>(), [&](auto const identifier)
                                                {
                                                    return !Tools::hasGroupAcsr(mAccessor, identifier);
                                                });
        mAccessor.setAttr<AttrType::layout>(layout, notification);
    }

    // Ensures that all groups are in the document
    {
        auto groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
        auto layout = mAccessor.getAttr<AttrType::layout>();
        anlWeakAssert(groupAcsrs.size() == layout.size());
        for(auto const& groupAcsr : groupAcsrs)
        {
            auto const groupIdentifier = groupAcsr.get().getAttr<Group::AttrType::identifier>();
            if(std::none_of(layout.cbegin(), layout.cend(), [&](auto const identifier)
                            {
                                return identifier == groupIdentifier;
                            }))
            {
                layout.push_back(groupIdentifier);
            }
        }
        mAccessor.setAttr<AttrType::layout>(layout, notification);
    }
}

juce::Result Document::Director::consolidate(juce::File const& file)
{
    juce::Result result = file.createDirectory();
    if(result.failed())
    {
        return result;
    }
    auto reader = mAccessor.getAttr<AttrType::reader>();
    std::set<juce::File> audioFiles;
    for(auto i = 0_z; i < reader.size(); ++i)
    {
        auto const newAudioFile = file.getChildFile(reader[i].file.getFileName());
        if(audioFiles.insert(reader[i].file).second)
        {
            if(reader[i].file.exists() && reader[i].file != newAudioFile && !reader[i].file.copyFileTo(newAudioFile))
            {
                return juce::Result::fail(juce::translate("Cannot copy to SRCFLNAME to DSTFLNAME").replace("SRCFLNAME", reader[i].file.getFullPathName()).replace("DSTFLNAME", newAudioFile.getFullPathName()));
            }
        }
        reader[i].file = newAudioFile;
    }

    auto const trackDirectory = file.getChildFile("Tracks");
    result = trackDirectory.createDirectory();
    if(result.failed())
    {
        return result;
    }

    auto const trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
    auto const childFiles = trackDirectory.findChildFiles(juce::File::TypesOfFileToFind::findFilesAndDirectories, true);
    for(auto& childFile : childFiles)
    {
        if(childFile.isDirectory() || !childFile.hasFileExtension("dat") || std::none_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcrs)
                                                                                         {
                                                                                             return trackAcrs.get().template getAttr<Track::AttrType::identifier>() == childFile.getFileNameWithoutExtension();
                                                                                         }))
        {
            childFile.deleteRecursively();
        }
    }

    for(auto& trackDirector : mTracks)
    {
        if(trackDirector != nullptr)
        {
            result = trackDirector->consolidate(trackDirectory);
            if(result.failed())
            {
                return result;
            }
        }
    }
    mAccessor.setAttr<AttrType::reader>(reader, NotificationType::synchronous);
    return juce::Result::ok();
}

bool Document::Director::fileHasBeenRemoved(juce::File const& file)
{
    if(AlertWindow::showOkCancel(AlertWindow::MessageType::warning, "Audio file cannot be found!", "The audio file FILENAME has been moved or deleted. Would you like to restore  it?", {{"FILENAME", file.getFullPathName()}}))
    {
        auto const audioFormatWildcard = mAudioFormatManager.getWildcardForAllFormats();
        juce::FileChooser fc(juce::translate("Restore the audio file..."), file, audioFormatWildcard);
        if(!fc.browseForFileToOpen())
        {
            return true;
        }
        auto const newFile = fc.getResult();
        auto reader = mAccessor.getAttr<AttrType::reader>();
        for(auto& channelLayout : reader)
        {
            if(channelLayout.file == file)
            {
                channelLayout.file = newFile;
            }
        }
        mAccessor.setAttr<AttrType::reader>(reader, NotificationType::asynchronous);
        return false;
    }
    return true;
}

bool Document::Director::fileHasBeenRestored(juce::File const& file)
{
    if(AlertWindow::showOkCancel(AlertWindow::MessageType::warning, "Audio file has been restored!", "The audio file FILENAME has been restored. Would you like to reload it?", {{"FILENAME", file.getFullPathName()}}))
    {
        initializeAudioReaders(NotificationType::asynchronous);
    }
    return true;
}

bool Document::Director::fileHasBeenModified(juce::File const& file)
{
    if(AlertWindow::showOkCancel(AlertWindow::MessageType::warning, "Audio file has been modified!", "The audio file FILENAME has been modified. Would you like to reload it?", {{"FILENAME", file.getFullPathName()}}))
    {
        initializeAudioReaders(NotificationType::asynchronous);
    }
    return true;
}

void Document::Director::initializeAudioReaders(NotificationType notification)
{
    auto const channels = mAccessor.getAttr<AttrType::reader>();
    auto& transportAcsr = mAccessor.getAcsr<AcsrType::transport>();
    if(channels.empty())
    {
        mSampleRate.reset();
        mAccessor.setAttr<AttrType::samplerate>(0.0, notification);
        transportAcsr.setAttr<Transport::AttrType::startPlayhead>(0.0, notification);
        transportAcsr.setAttr<Transport::AttrType::runningPlayhead>(0.0, notification);
        transportAcsr.setAttr<Transport::AttrType::loopRange>(Zoom::Range{}, notification);
        return;
    }
    auto& zoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
    auto const result = createAudioFormatReader(mAccessor, mAudioFormatManager);
    auto const& reader = std::get<0>(result);
    auto const& errors = std::get<1>(result);
    if(reader == nullptr)
    {
        AlertWindow::showMessage(AlertWindow::MessageType::warning, "Audio format reader cannot be loaded!", errors.joinIntoString("\n"));
        mSampleRate.reset();
        mAccessor.setAttr<AttrType::samplerate>(0.0, notification);
        mDuration = 0.0;
        zoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{0.0, 1.0}, notification);
        zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(Zoom::epsilon(), notification);
        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(Zoom::Range{0.0, 1.0}, notification);
        return;
    }
    else if(!errors.isEmpty())
    {
        AlertWindow::showMessage(AlertWindow::MessageType::warning, "Invalid audio format reader!", errors.joinIntoString("\n"));
    }

    mSampleRate = reader->sampleRate;
    mAccessor.setAttr<AttrType::samplerate>(reader->sampleRate, notification);

    mDuration = reader->sampleRate > 0.0 ? static_cast<double>(reader->lengthInSamples) / reader->sampleRate : 0.0;
    auto const visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    zoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{0.0, mDuration}, notification);
    zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(512.0 / reader->sampleRate, notification);
    if(visibleRange == Zoom::Range{0.0, 0.0})
    {
        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(Zoom::Range{0.0, mDuration}, notification);
    }

    auto const startPlayhead = transportAcsr.getAttr<Transport::AttrType::startPlayhead>();
    if(startPlayhead >= mDuration)
    {
        transportAcsr.setAttr<Transport::AttrType::startPlayhead>(0.0, notification);
    }
    auto const runningPlayhead = transportAcsr.getAttr<Transport::AttrType::runningPlayhead>();
    if(runningPlayhead >= mDuration)
    {
        transportAcsr.setAttr<Transport::AttrType::runningPlayhead>(0.0, notification);
    }
    auto const loopRange = transportAcsr.getAttr<Transport::AttrType::loopRange>();
    transportAcsr.setAttr<Transport::AttrType::loopRange>(Zoom::Range(0.0, mDuration).getIntersectionWith(loopRange), notification);

    for(auto const& anl : mTracks)
    {
        if(anl != nullptr)
        {
            anl->setAudioFormatReader(std::get<0>(createAudioFormatReader(mAccessor, mAudioFormatManager)), notification);
        }
    }
}

ANALYSE_FILE_END
