#include "AnlDocumentDirector.h"
#include "../Group/AnlGroupTools.h"
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
                FileWatcher::clearAllFiles();
                auto const reader = mAccessor.getAttr<AttrType::reader>();
                for(auto const& channelLayout : reader)
                {
                    FileWatcher::addFile(channelLayout.file);
                }
                if(std::none_of(reader.cbegin(), reader.cend(), [](auto const& channelLayout)
                                {
                                    auto const file = channelLayout.file;
                                    return file != juce::File{} && !file.existsAsFile();
                                }))
                {
                    initializeAudioReaders(notification);
                }
                else
                {
                    auto const options = juce::MessageBoxOptions()
                                             .withIconType(juce::AlertWindow::WarningIcon)
                                             .withTitle(juce::translate("Audio(s) files cannot be found!"))
                                             .withMessage(juce::translate("Audio file(s) cannot be found. Would you like to use the audio files layout panel to restore the file(s)?"))
                                             .withButton(juce::translate("Open Panel"))
                                             .withButton(juce::translate("Ignore"));

                    juce::WeakReference<Director> weakReference(this);
                    juce::AlertWindow::showAsync(options, [=, this](int result)
                                                 {
                                                     if(weakReference.get() == nullptr)
                                                     {
                                                         return;
                                                     }
                                                     if(result == 1)
                                                     {
                                                         mAccessor.sendSignal(SignalType::showReaderPanel, {}, NotificationType::asynchronous);
                                                     }
                                                 });
                }
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
            case AttrType::layout:
            case AttrType::viewport:
            case AttrType::path:
            case AttrType::autoresize:
            case AttrType::editMode:
            case AttrType::samplerate:
            case AttrType::channels:
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
                    director->setPluginTable(mPluginTableContainer);
                    director->setLoaderSelector(mLoaderSelectorContainer);
                    director->setBackupDirectory(mBackupDirectory);
                    director->onIdentifierUpdated = [this, ptr = director.get()](NotificationType localNotification)
                    {
                        for(auto& group : mGroups)
                        {
                            if(group != nullptr)
                            {
                                group->updateTracks(localNotification);
                            }
                        }
                        Track::SafeAccessorRetriever sav;
                        sav.getAccessorFn = getSafeTrackAccessorFn(ptr->getAccessor().getAttr<Track::AttrType::identifier>());
                        sav.getTimeZoomAccessorFn = getSafeTimeZoomAccessorFn();
                        sav.getTransportAccessorFn = getSafeTransportZoomAccessorFn();
                        ptr->setSafeAccessorRetriever(sav);
                    };
                    director->onResultsUpdated = [this](NotificationType localNotification)
                    {
                        updateMarkers(localNotification);
                    };
                    director->onChannelsLayoutUpdated = [this](NotificationType localNotification)
                    {
                        updateMarkers(localNotification);
                    };

                    Track::SafeAccessorRetriever sav;
                    sav.getAccessorFn = getSafeTrackAccessorFn(trackAcsr.getAttr<Track::AttrType::identifier>());
                    sav.getTimeZoomAccessorFn = getSafeTimeZoomAccessorFn();
                    sav.getTransportAccessorFn = getSafeTransportZoomAccessorFn();
                    director->setSafeAccessorRetriever(sav);
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
                if(director != nullptr)
                {
                    director->onIdentifierUpdated = [this, ptr = director.get()](NotificationType n)
                    {
                        juce::ignoreUnused(n);
                        ptr->setSafeAccessorRetriever(getSafeGroupAccessorFn(ptr->getAccessor().getAttr<Group::AttrType::identifier>()));
                    };
                    director->setSafeAccessorRetriever(getSafeGroupAccessorFn(groupAcsr.getAttr<Group::AttrType::identifier>()));
                }
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
                updateMarkers(notification);
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
            case Transport::AttrType::stopAtLoopEnd:
            case Transport::AttrType::gain:
            case Transport::AttrType::markers:
            case Transport::AttrType::magnetize:
            case Transport::AttrType::autoScroll:
            case Transport::AttrType::selection:
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
                auto const globalRange = Zoom::Range{0.0, mDuration};
                zoomAcsr.setAttr<Zoom::AttrType::globalRange>(globalRange, notification);
                auto const loopRange = transportAcsr.getAttr<Transport::AttrType::loopRange>();
                transportAcsr.setAttr<Transport::AttrType::loopRange>(globalRange.getIntersectionWith(loopRange), notification);
                auto const selection = transportAcsr.getAttr<Transport::AttrType::selection>();
                transportAcsr.setAttr<Transport::AttrType::selection>(globalRange.getIntersectionWith(selection), notification);
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
    setPluginTable(nullptr);
    setLoaderSelector(nullptr);
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

Track::Director const& Document::Director::getTrackDirector(juce::String const& identifier) const
{
    auto const it = std::find_if(mTracks.cbegin(), mTracks.cend(), [&](auto const& trackDirector)
                                 {
                                     auto const& trackAcsr = trackDirector->getAccessor();
                                     return trackAcsr.template getAttr<Track::AttrType::identifier>() == identifier;
                                 });
    anlStrongAssert(it != mTracks.cend());
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

juce::UndoManager& Document::Director::getUndoManager()
{
    return mUndoManager;
}

std::function<Track::Accessor&()> Document::Director::getSafeTrackAccessorFn(juce::String const& identifier)
{
    return [this, identifier]() -> Track::Accessor&
    {
        MiscWeakAssert(identifier.isNotEmpty());
        return Tools::getTrackAcsr(mAccessor, identifier);
    };
}

std::function<Group::Accessor&()> Document::Director::getSafeGroupAccessorFn(juce::String const& identifier)
{
    return [this, identifier]() -> Group::Accessor&
    {
        MiscWeakAssert(identifier.isNotEmpty());
        return Tools::getGroupAcsr(mAccessor, identifier);
    };
}

std::function<Zoom::Accessor&()> Document::Director::getSafeTimeZoomAccessorFn()
{
    return [this]() -> Zoom::Accessor&
    {
        return mAccessor.getAcsr<AcsrType::timeZoom>();
    };
}

std::function<Transport::Accessor&()> Document::Director::getSafeTransportZoomAccessorFn()
{
    return [this]() -> Transport::Accessor&
    {
        return mAccessor.getAcsr<AcsrType::transport>();
    };
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

void Document::Director::setPluginTable(PluginTableContainer* table)
{
    if(mPluginTableContainer != table)
    {
        mPluginTableContainer = table;
        for(auto& track : mTracks)
        {
            if(track != nullptr)
            {
                track->setPluginTable(table);
            }
        }
    }
}

void Document::Director::setLoaderSelector(LoaderSelectorContainer* selector)
{
    if(mLoaderSelectorContainer != selector)
    {
        mLoaderSelectorContainer = selector;
        for(auto& track : mTracks)
        {
            if(track != nullptr)
            {
                track->setLoaderSelector(selector);
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

void Document::Director::fileHasBeenRemoved(juce::File const& file)
{
    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::WarningIcon)
                             .withTitle(juce::translate("Audio file has been removed!"))
                             .withMessage(juce::translate("The audio file FLNAME has been removed. Would you like to restore it?").replace("FLNAME", file.getFullPathName()))
                             .withButton(juce::translate("Restore"))
                             .withButton(juce::translate("Cancel"));

    juce::WeakReference<Director> weakReference(this);
    juce::AlertWindow::showAsync(options, [=, this](int result)
                                 {
                                     if(weakReference.get() == nullptr)
                                     {
                                         return;
                                     }
                                     if(result == 1)
                                     {
                                         restoreAudioFile(file);
                                     }
                                 });
}

void Document::Director::fileHasBeenRestored(juce::File const& file)
{
    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::WarningIcon)
                             .withTitle(juce::translate("Audio file has been restored!"))
                             .withMessage(juce::translate("The audio file FLNAME has been restored. Would you like to reload it?").replace("FLNAME", file.getFullPathName()))
                             .withButton(juce::translate("Reload"))
                             .withButton(juce::translate("Cancel"));

    juce::WeakReference<Director> weakReference(this);
    juce::AlertWindow::showAsync(options, [=, this](int result)
                                 {
                                     if(weakReference.get() == nullptr || result != 1)
                                     {
                                         return;
                                     }
                                     initializeAudioReaders(NotificationType::synchronous);
                                 });
}

void Document::Director::fileHasBeenModified(juce::File const& file)
{
    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::WarningIcon)
                             .withTitle(juce::translate("Audio file has been modified!"))
                             .withMessage(juce::translate("The audio file FLNAME has been modified. Would you like to reload it?").replace("FLNAME", file.getFullPathName()))
                             .withButton(juce::translate("Reload"))
                             .withButton(juce::translate("Cancel"));

    juce::WeakReference<Director> weakReference(this);
    juce::AlertWindow::showAsync(options, [=, this](int result)
                                 {
                                     if(weakReference.get() == nullptr || result != 1)
                                     {
                                         return;
                                     }
                                     initializeAudioReaders(NotificationType::synchronous);
                                 });
}

void Document::Director::restoreAudioFile(juce::File const& file)
{
    auto const wildcard = mAudioFormatManager.getWildcardForAllFormats();
    mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Restore the audio file..."), file, wildcard);
    MiscWeakAssert(mFileChooser != nullptr);
    if(mFileChooser == nullptr)
    {
        return;
    }
    using Flags = juce::FileBrowserComponent::FileChooserFlags;
    juce::WeakReference<Director> weakReference(this);
    mFileChooser->launchAsync(Flags::openMode | Flags::canSelectFiles, [=, this](juce::FileChooser const& fileChooser)
                              {
                                  if(weakReference.get() == nullptr)
                                  {
                                      return;
                                  }
                                  auto const results = fileChooser.getResults();
                                  if(results.isEmpty())
                                  {
                                      return;
                                  }
                                  auto const newFile = results.getFirst();
                                  auto copyReader = mAccessor.getAttr<AttrType::reader>();
                                  for(auto& copyChannelLayout : copyReader)
                                  {
                                      if(copyChannelLayout.file == file)
                                      {
                                          copyChannelLayout.file = newFile;
                                      }
                                  }
                                  mAccessor.setAttr<AttrType::reader>(copyReader, NotificationType::synchronous);
                              });
}

void Document::Director::initializeAudioReaders(NotificationType notification)
{
    auto const channels = mAccessor.getAttr<AttrType::reader>();
    auto& transportAcsr = mAccessor.getAcsr<AcsrType::transport>();
    if(channels.empty())
    {
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
        if(mAlertCatcher == nullptr)
        {
            auto const options = juce::MessageBoxOptions()
                                     .withIconType(juce::AlertWindow::WarningIcon)
                                     .withTitle(juce::translate("Audio format reader cannot be loaded!"))
                                     .withMessage(errors.joinIntoString("\n"))
                                     .withButton(juce::translate("Ok"));
            juce::AlertWindow::showAsync(options, nullptr);
        }
        mAccessor.setAttr<AttrType::samplerate>(0.0, notification);
        mDuration = 0.0;
        zoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{0.0, 1.0}, notification);
        zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(Zoom::epsilon(), notification);
        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(Zoom::Range{0.0, 1.0}, notification);
        return;
    }
    else if(!errors.isEmpty() && mAlertCatcher == nullptr)
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("Invalid audio format reader!"))
                                 .withMessage(errors.joinIntoString("\n"))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    }

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

    juce::StringArray files;
    for(auto const& anl : mTracks)
    {
        if(anl != nullptr)
        {
            auto const& trackFile = anl->getAccessor().getAttr<Track::AttrType::file>();
            if(!trackFile.isEmpty())
            {
                files.add(anl->getAccessor().getAttr<Track::AttrType::name>());
            }
            anl->setAudioFormatReader(std::get<0>(createAudioFormatReader(mAccessor, mAudioFormatManager)), notification);
        }
    }
    if(!files.isEmpty() && mAlertCatcher == nullptr)
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::InfoIcon)
                                 .withTitle(juce::translate("Audio files layout cannot be applied to all tracks!"))
                                 .withMessage(juce::translate("The analysis results of the track(s) \"TRACKNAMES\" were modified, consolidated, or loaded from file(s). The results of these tracks won't be updated by the new audio files layout but you can detach the files using the track properties panels to trigger and update the analyses.").replace("TRACKNAMES", files.joinIntoString(", ")))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    }
}

void Document::Director::updateMarkers(NotificationType notification)
{
    std::set<double> markers;
    auto const& trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
    for(auto const& trackAcsr : trackAcsrs)
    {
        auto const& results = trackAcsr.get().getAttr<Track::AttrType::results>();
        auto const access = results.getReadAccess();
        if(static_cast<bool>(access))
        {
            auto const channelsLayout = trackAcsr.get().getAttr<Track::AttrType::channelsLayout>();
            auto const markerResults = results.getMarkers();
            if(markerResults != nullptr)
            {
                for(auto channelIndex = 0_z; channelIndex < std::min(markerResults->size(), channelsLayout.size()); ++channelIndex)
                {
                    if(channelsLayout[channelIndex])
                    {
                        auto const& channelMarkers = markerResults->at(channelIndex);
                        for(auto const& marker : channelMarkers)
                        {
                            markers.insert(std::get<0_z>(marker));
                        }
                    }
                }
            }
        }
    }
    mAccessor.getAcsr<AcsrType::transport>().setAttr<Transport::AttrType::markers>(markers, notification);
}

void Document::Director::setBackupDirectory(juce::File const& directory)
{
    if(mBackupDirectory != directory)
    {
        mBackupDirectory = directory;
        for(auto& track : mTracks)
        {
            if(track != nullptr)
            {
                track->setBackupDirectory(mBackupDirectory);
            }
        }
    }
}

ANALYSE_FILE_END
