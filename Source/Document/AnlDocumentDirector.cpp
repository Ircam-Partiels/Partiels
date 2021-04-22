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

                for(auto const& anl : mTracks)
                {
                    if(anl != nullptr)
                    {
                        anl->setAudioFormatReader(createAudioFormatReader(mAccessor, mAudioFormatManager, AlertType::silent), notification);
                    }
                }
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
        auto gIds = groupAcsr.get().getAttr<Group::AttrType::layout>();
        std::erase(gIds, identifier);
        groupAcsr.get().setAttr<Group::AttrType::layout>(gIds, NotificationType::synchronous);
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

    auto const layout = it->get().getAttr<Group::AttrType::layout>();
    for(auto const& tarckIdentifier : layout)
    {
        removeTrack(tarckIdentifier, notification);
    }

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
    anlWeakAssert(Tools::hasGroupAcsr(mAccessor, trackIdentifier));
    if(Tools::hasGroupAcsr(mAccessor, trackIdentifier))
    {
        auto& groupAcsr = Tools::getGroupAcsr(mAccessor, trackIdentifier);
        auto const identifier = groupAcsr.getAttr<Group::AttrType::identifier>();

        // The previous groups is the same as the new group
        anlWeakAssert(identifier != groupIdentifier);
        if(identifier == groupIdentifier)
        {
            return false;
        }
        auto layout = groupAcsr.getAttr<Group::AttrType::layout>();
        std::erase(layout, trackIdentifier);
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

    auto const& trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
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
    auto groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
    if(!groupAcsrs.empty())
    {
        auto& lastAcsr = groupAcsrs.back().get();
        for(auto const& trackAcsr : trackAcsrs)
        {
            auto const trackIdentifier = trackAcsr.get().getAttr<Track::AttrType::identifier>();
            if(std::none_of(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](auto const& groupAcsr)
                            {
                                auto const& groupLayout = groupAcsr.get().template getAttr<Group::AttrType::layout>();
                                return std::any_of(groupLayout.cbegin(), groupLayout.cend(), [&](auto const identifier)
                                                   {
                                                       return identifier == trackIdentifier;
                                                   });
                            }))
            {
                auto groupLayout = lastAcsr.getAttr<Group::AttrType::layout>();
                groupLayout.push_back(trackIdentifier);
                lastAcsr.setAttr<Group::AttrType::layout>(groupLayout, notification);
            }
        }
    }

    auto layout = mAccessor.getAttr<AttrType::layout>();
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

ANALYSE_FILE_END
