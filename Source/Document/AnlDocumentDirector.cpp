#include "AnlDocumentDirector.h"
#include "AnlDocumentAudioReader.h"
#include "../Plugin/AnlPluginListScanner.h"
#include "../Track/AnlTrackPropertyPanel.h"

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
            case AttrType::layoutVertical:
            case AttrType::layout:
            case AttrType::expanded:
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
                anlStrongAssert(index <= mTracks.size());
                if(index > mTracks.size())
                {
                    return;
                }
                auto& trackAcsr = mAccessor.getAcsr<AcsrType::tracks>(index);
                auto audioFormatReader = createAudioFormatReader(mAccessor, mAudioFormatManager, AlertType::silent);
                auto director = std::make_unique<Track::Director>(trackAcsr, std::move(audioFormatReader));
                anlStrongAssert(director != nullptr);
                if(director != nullptr)
                {
                    director->onLinkedZoomChanged = [this](Zoom::Accessor const& zoomAcsr, NotificationType notification)
                    {
                        for(auto& track : mTracks)
                        {
                            if(track != nullptr)
                            {
                                track->setLinkedZoom(zoomAcsr, notification);
                            }
                        }
                    };
                }
                mTracks.insert(mTracks.begin() + static_cast<long>(index), std::move(director));
            }
                break;
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
            }
                break;
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
        
        auto layout = mAccessor.getAttr<AttrType::layout>();
        layout.push_back(identifier);
        mAccessor.setAttr<AttrType::layout>(layout, notification);
        anlStrongAssert(layout.size() == mAccessor.getNumAcsr<AcsrType::tracks>());
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

void Document::Director::removeTrack(juce::String const identifier, NotificationType const notification)
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
    if(!juce::AlertWindow::showOkCancelBox(icon, title, message))
    {
        return;
    }
        
    auto layout = mAccessor.getAttr<AttrType::layout>();
    std::erase(layout, identifier);
    mAccessor.setAttr<AttrType::layout>(layout, notification);
    auto const index = static_cast<size_t>(std::distance(trackAcsrs.cbegin(), it));
    mAccessor.eraseAcsr<AcsrType::tracks>(index, notification);
    anlStrongAssert(layout.size() == mAccessor.getNumAcsr<AcsrType::tracks>());
}

ANALYSE_FILE_END
