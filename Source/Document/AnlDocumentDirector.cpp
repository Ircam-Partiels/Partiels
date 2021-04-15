#include "AnlDocumentDirector.h"
#include "AnlDocumentAudioReader.h"
#include "../Zoom/AnlZoomTools.h"
#include "../Group/AnlGroupTools.h"

ANALYSE_FILE_BEGIN

Document::Director::Director(Accessor& accessor, PluginList::Accessor& pluginListAccessor, PluginList::Scanner& pluginListScanner, juce::AudioFormatManager& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
, mPluginListTable(pluginListAccessor, pluginListScanner)
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
                auto director = std::make_unique<Track::Director>(trackAcsr, std::move(audioFormatReader));
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
                if(index >= groupAcsrs.size())
                {
                    return;
                }
                
                auto& groupAcsr = groupAcsrs[index].get();
                groupAcsr.onAttrUpdated = [&](Group::AttrType attribute, NotificationType notification)
                {
                    switch(attribute)
                    {
                        case Group::AttrType::identifier:
                        case Group::AttrType::name:
                        case Group::AttrType::height:
                        case Group::AttrType::colour:
                        case Group::AttrType::expanded:
                            break;
                        case Group::AttrType::layout:
                        case Group::AttrType::tracks:
                        {
                            auto trackAcsrs = groupAcsr.getAttr<Group::AttrType::tracks>();
                            auto const layout = groupAcsr.getAttr<Group::AttrType::layout>();
                            for(size_t i = 0; i < trackAcsrs.size(); ++i)
                            {
                                auto const trackIdentifier = trackAcsrs[i].get().getAttr<Track::AttrType::identifier>();
                                if(std::any_of(layout.cbegin(), layout.cend(), [&](auto const identifier)
                                {
                                    return identifier == trackIdentifier;
                                }))
                                {
                                    mTracks[i]->setLinkedZoom(&groupAcsr.getAcsr<Group::AcsrType::zoom>(), notification);
                                }
                            };
                        }
                            break;
                    }
                };
                groupAcsr.setAttr<Group::AttrType::tracks>(mAccessor.getAcsrs<AcsrType::tracks>(), notification);
                auto& groupZoomAcsr = groupAcsr.getAcsr<Group::AcsrType::zoom>();
                if(groupZoomAcsr.getAttr<Zoom::AttrType::globalRange>().isEmpty())
                {
                    groupZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{0.0, 1.0}, notification);
                    groupZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(Zoom::Range{0.0, 1.0}, notification);
                }
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
                auto const time = transportAcsr .getAttr<Transport::AttrType::startPlayhead>();
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
                    auto& trackAcsr  = mAccessor.getAcsr<AcsrType::tracks>(i);
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

void Document::Director::addTrack(AlertType const alertType, NotificationType const notification)
{
    if(mModalWindow != nullptr)
    {
        mModalWindow->exitModalState(0);
        mModalWindow = nullptr;
    }
    
    mPluginListTable.onPluginSelected = [this, alertType, notification](Plugin::Key const& key, Plugin::Description const& description)
    {
        if(mModalWindow != nullptr)
        {
            mModalWindow->exitModalState(0);
            mModalWindow = nullptr;
        }
        
        auto const index = mAccessor.getNumAcsr<AcsrType::tracks>();
        if(!mAccessor.insertAcsr<AcsrType::tracks>(index, notification))
        {
            if(alertType == AlertType::window)
            {
                auto constexpr icon = juce::AlertWindow::AlertIconType::WarningIcon;
                auto const title = juce::translate("Analysis cannot be created!");
                auto const message = juce::translate("The analysis \"PLGNAME: SPECNAME\" cannot be inserted into the document.").replace("PLGNAME", description.name).replace("SPECNAME", description.output.name);
                juce::AlertWindow::showMessageBox(icon, title, message);
            }
            return;
        }
        
        auto const identifier = juce::Uuid().toString();

        auto& trackAcsr = mAccessor.getAcsr<Document::AcsrType::tracks>(index);
        trackAcsr.setAttr<Track::AttrType::identifier>(identifier, notification);
        trackAcsr.setAttr<Track::AttrType::name>(description.name, NotificationType::synchronous);
        trackAcsr.setAttr<Track::AttrType::description>(description, NotificationType::synchronous);
        trackAcsr.setAttr<Track::AttrType::state>(description.defaultState, NotificationType::synchronous);
        trackAcsr.setAttr<Track::AttrType::key>(key, notification);
        
        auto groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
        anlWeakAssert(!groupAcsrs.empty());
        if(groupAcsrs.empty())
        {
            return;
        }
        auto& lastAcsr = groupAcsrs.back().get();
        auto groupLayout = lastAcsr.getAttr<Group::AttrType::layout>();
        groupLayout.push_back(identifier);
        lastAcsr.setAttr<Group::AttrType::layout>(groupLayout, NotificationType::synchronous);
    };
    
    auto const& laf = juce::Desktop::getInstance().getDefaultLookAndFeel();
    auto const bgColor = laf.findColour(juce::ResizableWindow::backgroundColourId);
    
    juce::DialogWindow::LaunchOptions o;
    o.dialogTitle = juce::translate("Add Analysis...");
    o.content.setNonOwned(&mPluginListTable);
    o.componentToCentreAround = nullptr;
    o.dialogBackgroundColour = bgColor;
    o.escapeKeyTriggersCloseButton = true;
    o.useNativeTitleBar = false;
    o.resizable = false;
    o.useBottomRightCornerResizer = false;
    mModalWindow = o.launchAsync();
    if(mModalWindow != nullptr)
    {
        mModalWindow->runModalLoop();
    }
}

void Document::Director::removeTrack(AlertType const alertType, juce::String const identifier, NotificationType const notification)
{
    auto const trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
    auto const it = std::find_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](Track::Accessor const& acsr)
    {
        return acsr.getAttr<Track::AttrType::identifier>() == identifier;
    });
    anlWeakAssert(it != trackAcsrs.cend());
    if(it == trackAcsrs.cend())
    {
        return;
    }
    
    auto constexpr icon = juce::AlertWindow::AlertIconType::QuestionIcon;
    auto const title = juce::translate("Remove Analysis");
    auto const message = juce::translate("Are you sure you want to remove the \"ANLNAME\" analysis from the project? If you edited the results of the analysis, the changes will be lost!").replace("ANLNAME", it->get().getAttr<Track::AttrType::name>());
    if(alertType == AlertType::window && !juce::AlertWindow::showOkCancelBox(icon, title, message))
    {
        return;
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
}


void Document::Director::moveTrack(AlertType const alertType, juce::String const groupIdentifier, juce::String const trackIdentifier, NotificationType const notification)
{
    juce::ignoreUnused(alertType);
    auto const trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
    auto const it = std::find_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](Track::Accessor const& acsr)
    {
        return acsr.getAttr<Track::AttrType::identifier>() == trackIdentifier;
    });
    anlWeakAssert(it != trackAcsrs.cend());
    if(it == trackAcsrs.cend())
    {
        return;
    }
    
    auto groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
    auto goupIt = std::find_if(groupAcsrs.begin(), groupAcsrs.end(), [&](auto const& groupAcsr)
    {
        return groupAcsr.get().template getAttr<Group::AttrType::identifier>() == groupIdentifier;
    });
    anlWeakAssert(goupIt != groupAcsrs.end());
    if(goupIt == groupAcsrs.end())
    {
        return;
    }
    for(auto& groupAcsr : groupAcsrs)
    {
        auto gIds = groupAcsr.get().getAttr<Group::AttrType::layout>();
        if(std::addressof(*goupIt) == std::addressof(groupAcsr))
        {
            if(std::none_of(gIds.cbegin(), gIds.cend(), [&](auto const& identifier)
            {
                return identifier == trackIdentifier;
            }))
            {
                gIds.push_back(trackIdentifier);
            }
            else
            {
                anlWeakAssert(false);
            }
        }
        else
        {
            std::erase(gIds, trackIdentifier);
        }
        groupAcsr.get().setAttr<Group::AttrType::layout>(gIds, notification);
    }
}

void Document::Director::addGroup(AlertType const alertType, NotificationType const notification)
{
    auto const index = mAccessor.getNumAcsr<AcsrType::groups>();
    if(!mAccessor.insertAcsr<AcsrType::groups>(index, notification))
    {
        if(alertType == AlertType::window)
        {
            auto constexpr icon = juce::AlertWindow::AlertIconType::WarningIcon;
            auto const title = juce::translate("Group cannot be created!");
            auto const message = juce::translate("The group cannot be inserted into the document.");
            juce::AlertWindow::showMessageBox(icon, title, message);
        }
        return;
    }
    
    auto const identifier = juce::Uuid().toString();
    
    auto& groupAcsr = mAccessor.getAcsr<Document::AcsrType::groups>(index);
    groupAcsr.setAttr<Group::AttrType::identifier>(identifier, notification);
    groupAcsr.setAttr<Group::AttrType::name>("Group " + juce::String(index), notification);
    
    auto layout = mAccessor.getAttr<AttrType::layout>();
    layout.push_back(identifier);
    mAccessor.setAttr<AttrType::layout>(layout, notification);
}

void Document::Director::removeGroup(AlertType const alertType, juce::String const identifier, NotificationType const notification)
{
    auto const groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
    auto const it = std::find_if(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](Group::Accessor const& acsr)
    {
        return acsr.getAttr<Group::AttrType::identifier>() == identifier;
    });
    anlWeakAssert(it != groupAcsrs.cend());
    if(it == groupAcsrs.cend())
    {
        return;
    }
    
    auto constexpr icon = juce::AlertWindow::AlertIconType::QuestionIcon;
    auto const title = juce::translate("Remove Group");
    auto const message = juce::translate("Are you sure you want to remove the \"ANLNAME\" group from the project? This will delete all the analysis and lose everything!!!!!").replace("ANLNAME", it->get().getAttr<Group::AttrType::name>());
    if(alertType == AlertType::window && !juce::AlertWindow::showOkCancelBox(icon, title, message))
    {
        return;
    }
    
    auto const layout = it->get().getAttr<Group::AttrType::layout>();
    for(auto const& tarckIdentifier : layout)
    {
        removeTrack(AlertType::silent, tarckIdentifier, notification);
    }
    
    auto const index = static_cast<size_t>(std::distance(groupAcsrs.cbegin(), it));
    mAccessor.eraseAcsr<AcsrType::groups>(index, notification);
}

void Document::Director::sanitize(NotificationType const notification)
{
    if(mAccessor.getNumAcsr<AcsrType::groups>() == 0_z)
    {
        addGroup(AlertType::silent, notification);
    }
    auto groupAcsrs = mAccessor.getAcsrs<AcsrType::groups>();
    anlWeakAssert(!groupAcsrs.empty());
    if(groupAcsrs.empty())
    {
        return;
    }
    
    auto& lastAcsr = groupAcsrs.back().get();
    auto const& trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
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
