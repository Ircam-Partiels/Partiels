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
            case AttrType::file:
            {
                auto const file = mAccessor.getAttr<AttrType::file>();
                if(file != juce::File() && !file.existsAsFile())
                {
                    if(AlertWindow::showOkCancel(AlertWindow::MessageType::warning, "Audio file cannot be found!", "The audio file FILENAME has been moved or deleted. Would you like to restore  it?", {{"FILENAME", file.getFullPathName()}}))
                    {
                        auto const audioFormatWildcard = mAudioFormatManager.getWildcardForAllFormats();
                        juce::FileChooser fc(juce::translate("Restore the audio file..."), file, audioFormatWildcard);
                        if(!fc.browseForFileToOpen())
                        {
                            setFile(file);
                            return;
                        }
                        mAccessor.setAttr<AttrType::file>(fc.getResult(), NotificationType::synchronous);
                        return;
                    }
                }
                setFile(file);
                initializeAudioReaders(notification);
            }
            break;
            case AttrType::layout:
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
                auto audioFormatReader = createAudioFormatReader(mAccessor, mAudioFormatManager, AlertType::silent);
                auto director = std::make_unique<Track::Director>(trackAcsr, mUndoManager, std::move(audioFormatReader));
                anlStrongAssert(director != nullptr);
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
        switch(attribute)
        {
            case Transport::AttrType::playback:
                break;
            case Transport::AttrType::startPlayhead:
            {
                auto const time = transportAcsr.getAttr<Transport::AttrType::startPlayhead>();
                zoomAcsr.setAttr<Zoom::AttrType::anchor>(std::make_tuple(false, time), notification);
                if(!transportAcsr.getAttr<Transport::AttrType::playback>())
                {
                    transportAcsr.setAttr<Transport::AttrType::runningPlayhead>(time, notification);
                }
            }
            break;
            case Transport::AttrType::runningPlayhead:
            {
                auto const time = transportAcsr.getAttr<Transport::AttrType::runningPlayhead>();
                auto const numAnlAcsrs = mAccessor.getNumAcsr<AcsrType::tracks>();
                for(size_t i = 0; i < numAnlAcsrs; ++i)
                {
                    auto& trackAcsr = mAccessor.getAcsr<AcsrType::tracks>(i);
                    trackAcsr.setAttr<Track::AttrType::time>(time, notification);
                }
                auto const range = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
                if(!range.contains(time))
                {
                    zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range.movedToStartAt(time), notification);
                }
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
                zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(512.0 / mSampleRate, notification);
            }
            break;
            case Zoom::AttrType::visibleRange:
            case Zoom::AttrType::anchor:
                break;
        }
    };
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

void Document::Director::startAction()
{
    mSavedState.copyFrom(mAccessor, NotificationType::synchronous);
}

void Document::Director::endAction(juce::String const& name, ActionState state)
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
            case ActionState::apply:
            {
                mUndoManager.beginNewTransaction(name);
                mUndoManager.perform(action.release());
            }
            break;
            case ActionState::abort:
            {
                action->undo();
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

    auto const index = mAccessor.getNumAcsr<AcsrType::tracks>();
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
    auto const index = mAccessor.getNumAcsr<AcsrType::groups>();
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

bool Document::Director::moveTrack(juce::String const groupIdentifier, juce::String const trackIdentifier, NotificationType const notification)
{
    anlStrongAssert(Tools::hasGroupAcsr(mAccessor, groupIdentifier));
    anlStrongAssert(Tools::hasTrackAcsr(mAccessor, trackIdentifier));
    if(!Tools::hasGroupAcsr(mAccessor, groupIdentifier) || !Tools::hasTrackAcsr(mAccessor, trackIdentifier))
    {
        return false;
    }
    // The track should already be in a group
    anlWeakAssert(Tools::isTrackInGroup(mAccessor, trackIdentifier));
    if(Tools::isTrackInGroup(mAccessor, trackIdentifier))
    {
        auto& groupAcsr = Tools::getGroupAcsrForTrack(mAccessor, trackIdentifier);
        auto const identifier = groupAcsr.getAttr<Group::AttrType::identifier>();

        // The previous groups is the same as the new group
        anlWeakAssert(identifier != groupIdentifier);
        if(identifier == groupIdentifier)
        {
            return false;
        }

        auto const layout = copy_with_erased(groupAcsr.getAttr<Group::AttrType::layout>(), trackIdentifier);
        groupAcsr.setAttr<Group::AttrType::layout>(layout, notification);
    }

    auto& groupAcsr = Tools::getGroupAcsr(mAccessor, groupIdentifier);
    auto layout = groupAcsr.getAttr<Group::AttrType::layout>();
    if(std::any_of(layout.cbegin(), layout.cend(), [&](auto const& identifier)
                   {
                       return identifier == trackIdentifier;
                   }))
    {
        anlWeakAssert(false);
        return false;
    }
    layout.insert(layout.cbegin(), trackIdentifier);
    groupAcsr.setAttr<Group::AttrType::layout>(layout, notification);
    groupAcsr.setAttr<Group::AttrType::expanded>(true, notification);
    sanitize(notification);
    return true;
}

void Document::Director::sanitize(NotificationType const notification)
{
    if(mAccessor.getNumAcsr<AcsrType::tracks>() == 0_z && mAccessor.getNumAcsr<AcsrType::groups>() == 0_z)
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
        anlWeakAssert(trackAcsrs.empty() || mAccessor.getNumAcsr<AcsrType::groups>() != 0_z);
        if(!trackAcsrs.empty() && mAccessor.getNumAcsr<AcsrType::groups>() == 0_z)
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
    if(mAccessor.getNumAcsr<AcsrType::groups>() != 0_z)
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
        anlWeakAssert(mAccessor.getNumAcsr<AcsrType::groups>() == mAccessor.getAttr<AttrType::layout>().size());
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

void Document::Director::fileHasBeenRemoved()
{
    auto const file = mAccessor.getAttr<AttrType::file>();
    if(AlertWindow::showOkCancel(AlertWindow::MessageType::warning, "Audio file cannt be found!", "The audio file FILENAME has been moved or deleted. Would you like to restore  it?", {{"FILENAME", file.getFullPathName()}}))
    {
        auto const audioFormatWildcard = mAudioFormatManager.getWildcardForAllFormats();
        juce::FileChooser fc(juce::translate("Restore the audio file..."), file, audioFormatWildcard);
        if(!fc.browseForFileToOpen())
        {
            return;
        }
        mAccessor.setAttr<AttrType::file>(fc.getResult(), NotificationType::synchronous);
    }
}

void Document::Director::fileHasBeenModified()
{
    auto const file = mAccessor.getAttr<AttrType::file>();
    if(AlertWindow::showOkCancel(AlertWindow::MessageType::warning, "Audio file  has been modified!", "The audio file FILENAME has been modified. Would you like to reload it?", {{"FILENAME", file.getFullPathName()}}))
    {
        initializeAudioReaders(NotificationType::synchronous);
    }
}

void Document::Director::initializeAudioReaders(NotificationType notification)
{
    auto const file = mAccessor.getAttr<AttrType::file>();
    auto& transportAcsr = mAccessor.getAcsr<AcsrType::transport>();
    if(!file.existsAsFile())
    {
        transportAcsr.setAttr<Transport::AttrType::startPlayhead>(0.0, notification);
        transportAcsr.setAttr<Transport::AttrType::runningPlayhead>(0.0, notification);
        transportAcsr.setAttr<Transport::AttrType::loopRange>(Zoom::Range{}, notification);
        return;
    }

    auto reader = createAudioFormatReader(mAccessor, mAudioFormatManager, AlertType::window);
    auto& zoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
    if(reader == nullptr)
    {
        mDuration = 0.0;
        mSampleRate = 44100.0;
        zoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{0.0, 0.0}, notification);
        zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(0.0, notification);
        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(Zoom::Range{0.0, 0.0}, notification);
        return;
    }
    mSampleRate = reader->sampleRate > 0.0 ? reader->sampleRate : 44100.0;
    mDuration = static_cast<double>(reader->lengthInSamples) / mSampleRate;
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
            anl->setAudioFormatReader(createAudioFormatReader(mAccessor, mAudioFormatManager, AlertType::silent), notification);
        }
    }
}

ANALYSE_FILE_END
