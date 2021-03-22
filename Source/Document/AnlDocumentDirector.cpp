#include "AnlDocumentDirector.h"
#include "AnlDocumentAudioReader.h"
#include "../Plugin/AnlPluginListScanner.h"
#include "../Track/AnlTrackPropertyPanel.h"

ANALYSE_FILE_BEGIN

Document::Director::Director(Accessor& accessor, PluginList::Accessor& pluginListAccessor, PluginList::Scanner& pluginListScanner, juce::AudioFormatManager& audioFormatManager)
: mAccessor(accessor)
, mPluginListScanner(pluginListScanner)
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
                auto& zoomAcsr = mAccessor.getAccessor<AcsrType::timeZoom>(0);
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
                
                for(auto const& anl : mAnalyzers)
                {
                    if(anl != nullptr)
                    {
                        anl->setAudioFormatReader(createAudioFormatReader(mAccessor, mAudioFormatManager, AlertType::silent), notification);
                    }
                }
            }
                break;
            case AttrType::isLooping:
            case AttrType::gain:
            case AttrType::isPlaybackStarted:
            case AttrType::playheadPosition:
            {
                auto const time = mAccessor.getAttr<AttrType::playheadPosition>();
                auto const numAnlAcsrs = mAccessor.getNumAccessors<AcsrType::analyzers>();
                for(size_t i = 0; i < numAnlAcsrs; ++i)
                {
                    auto& anlAcsr  = mAccessor.getAccessor<AcsrType::analyzers>(i);
                    anlAcsr.setAttr<Analyzer::AttrType::time>(time, notification);
                }
                auto& zoomAcsr = mAccessor.getAccessor<AcsrType::timeZoom>(0);
                auto const range = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
                if(!range.contains(time))
                {
                    zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range.movedToStartAt(time), notification);
                }
            }
                break;
            case AttrType::layoutHorizontal:
            case AttrType::layoutVertical:
            case AttrType::layout:
                break;
        }
    };
    
    mAccessor.onAccessorInserted = [&](AcsrType type, size_t index, NotificationType notification)
    {
        juce::ignoreUnused(notification);
        switch(type)
        {
            case AcsrType::analyzers:
            {
                anlStrongAssert(index <= mAnalyzers.size());
                if(index > mAnalyzers.size())
                {
                    return;
                }
                auto& anlAcsr = mAccessor.getAccessor<AcsrType::analyzers>(index);
                auto audioFormatReader = createAudioFormatReader(mAccessor, mAudioFormatManager, AlertType::silent);
                auto director = std::make_unique<Analyzer::Director>(anlAcsr, mPluginListScanner, std::move(audioFormatReader));
                anlStrongAssert(director != nullptr);
                mAnalyzers.insert(mAnalyzers.begin() + static_cast<long>(index), std::move(director));
            }
                break;
            case AcsrType::timeZoom:
                break;
        }
    };
    
    mAccessor.onAccessorErased = [&](AcsrType type, size_t index, NotificationType notification)
    {
        juce::ignoreUnused(notification);
        switch(type)
        {
            case AcsrType::analyzers:
            {
                anlStrongAssert(index < mAnalyzers.size());
                if(index >= mAnalyzers.size())
                {
                    return;
                }
                mAnalyzers.erase(mAnalyzers.begin() + static_cast<long>(index));
            }
                break;
            case AcsrType::timeZoom:
                break;
        }
    };
    
    auto& zoomAcsr = mAccessor.getAccessor<AcsrType::timeZoom>(0);
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
                break;
        }
    };
}

Document::Director::~Director()
{
    auto& zoomAcsr = mAccessor.getAccessor<AcsrType::timeZoom>(0);
    zoomAcsr.onAttrUpdated = nullptr;
    mAccessor.onAttrUpdated = nullptr;
    mAccessor.onAccessorInserted = nullptr;
    mAccessor.onAccessorErased = nullptr;
}

void Document::Director::addAnalysis(AlertType const alertType, NotificationType const notification)
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
        
        auto const index = mAccessor.getNumAccessors<AcsrType::analyzers>();
        if(!mAccessor.insertAccessor<AcsrType::analyzers>(index, notification))
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

        auto& anlAcsr = mAccessor.getAccessor<Document::AcsrType::analyzers>(index);
        anlAcsr.setAttr<Analyzer::AttrType::identifier>(identifier, notification);
        anlAcsr.setAttr<Analyzer::AttrType::key>(key, notification);
        
        auto layout = mAccessor.getAttr<AttrType::layout>();
        layout.push_back(identifier);
        mAccessor.setAttr<AttrType::layout>(layout, notification);
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

void Document::Director::removeAnalysis(juce::String const identifier, NotificationType const notification)
{
    auto const anlAcsrs = mAccessor.getAccessors<AcsrType::analyzers>();
    auto const it = std::find_if(anlAcsrs.cbegin(), anlAcsrs.cend(), [&](Analyzer::Accessor const& acsr)
    {
        return acsr.getAttr<Analyzer::AttrType::identifier>() == identifier;
    });
    anlWeakAssert(it != anlAcsrs.cend());
    if(it == anlAcsrs.cend())
    {
        return;
    }
    
    
    auto constexpr icon = juce::AlertWindow::AlertIconType::QuestionIcon;
    auto const title = juce::translate("Remove Analysis");
    auto const message = juce::translate("Are you sure you want to remove the \"ANLNAME\" analysis from the project? If you edited the results of the analysis, the changes will be lost!").replace("ANLNAME", it->get().getAttr<Analyzer::AttrType::name>());
    if(!juce::AlertWindow::showOkCancelBox(icon, title, message))
    {
        return;
    }
        
    auto layout = mAccessor.getAttr<AttrType::layout>();
    std::erase(layout, identifier);
    mAccessor.setAttr<AttrType::layout>(layout, notification);
    auto const index = static_cast<size_t>(std::distance(anlAcsrs.cbegin(), it));
    mAccessor.eraseAccessor<AcsrType::analyzers>(index, notification);
}

ANALYSE_FILE_END
