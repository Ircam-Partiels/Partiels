#include "AnlDocumentDirector.h"
#include "AnlDocumentAudioReader.h"
#include "../Plugin/AnlPluginListScanner.h"
#include "../Analyzer/AnlAnalyzerProcessor.h"
#include "../Analyzer/AnlAnalyzerPropertyPanel.h"

ANALYSE_FILE_BEGIN

Document::Director::Director(Accessor& accessor, PluginList::Accessor& pluginAccessor, juce::AudioFormatManager const& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
, mPluginListTable(pluginAccessor)
{
    setupDocument(mAccessor);
}

Document::Director::~Director()
{
    mAccessor.onUpdated = nullptr;
}

void Document::Director::addAnalysis(AlertType alertType)
{
    if(mModalWindow != nullptr)
    {
        mModalWindow->exitModalState(0);
        mModalWindow = nullptr;
    }
    
    mPluginListTable.onPluginSelected = [this, alertType](juce::String key)
    {
        if(mModalWindow != nullptr)
        {
            mModalWindow->exitModalState(0);
            mModalWindow = nullptr;
        }
        
        auto const name = PluginList::Scanner::getPluginDescriptions()[key].name;
        if(!mAccessor.insertAccessor<Document::AttrType::analyzers>(-1, NotificationType::synchronous))
        {
            if(alertType == AlertType::window)
            {
                auto constexpr icon = juce::AlertWindow::AlertIconType::WarningIcon;
                auto const title = juce::translate("Analysis cannot be created!");
                auto const message = juce::translate("The analysis \"ANLNAME\" cannot be inserted into the document.").replace("ANLNAME", name);
                juce::AlertWindow::showMessageBox(icon, title, message);
            }
            return;
        }

        auto& anlAcsr = mAccessor.getAccessors<Document::AttrType::analyzers>().back().get();
        anlAcsr.setAttr<Analyzer::AttrType::key>(key, NotificationType::synchronous);
        anlAcsr.setAttr<Analyzer::AttrType::name>(name, NotificationType::synchronous);
        anlAcsr.setAttr<Analyzer::AttrType::colour>(juce::Colours::blue, NotificationType::synchronous);
        anlAcsr.setAttr<Analyzer::AttrType::colourMap>(Analyzer::ColorMap::Inferno, NotificationType::synchronous);
        
        Analyzer::PropertyPanel panel(anlAcsr);
        auto const title = juce::translate("Analyzer Properties");
        auto const& lookAndFeel = juce::Desktop::getInstance().getDefaultLookAndFeel();
        auto const bgColor = lookAndFeel.findColour(juce::ResizableWindow::backgroundColourId);
        juce::DialogWindow::showModalDialog(title, &panel, nullptr, bgColor, true, false, false);
    };
    
    auto const& lookAndFeel = juce::Desktop::getInstance().getDefaultLookAndFeel();
    auto const bgColor = lookAndFeel.findColour(juce::ResizableWindow::backgroundColourId);
    
    juce::DialogWindow::LaunchOptions o;
    o.dialogTitle = juce::translate("New Analyzer...");
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

void Document::Director::setupDocument(Document::Accessor& acsr)
{
    acsr.onUpdated = [&](AttrType attribute, NotificationType notification)
    {
        switch (attribute)
        {
            case file:
            {
                auto reader = createAudioFormatReader(acsr, mAudioFormatManager, AlertType::window);
                if(reader == nullptr)
                {
                    return;
                }
                auto const sampleRate = reader->sampleRate;
                auto const duration = sampleRate > 0.0 ? static_cast<double>(reader->lengthInSamples) / sampleRate : 0.0;
                
                auto& zoomAcsr = acsr.getAccessor<AttrType::timeZoom>(0);
                zoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{0.0, duration}, notification);
                zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(duration / 100.0, notification);
                zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(Zoom::Range{0.0, duration}, notification);
                
                for(size_t i = 0; i < mAnalyzers.size(); ++i)
                {
                    mAnalyzers[i]->setAudioFormatReader(createAudioFormatReader(mAccessor, mAudioFormatManager, AlertType::silent), notification);
                }
            }
                break;
            case isLooping:
            case gain:
            case isPlaybackStarted:
            case playheadPosition:
            {
                auto const time = acsr.getAttr<AttrType::playheadPosition>();
                auto& zoomAcsr = acsr.getAccessor<AttrType::timeZoom>(0);
                auto const range = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
                if(!range.contains(time))
                {
                    zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range.movedToStartAt(time), notification);
                }
            }
                break;
            case timeZoom:
            case layout:
            case layoutHorizontal:
                break;
            case analyzers:
            {
                auto anlAcsrs = acsr.getAccessors<AttrType::analyzers>();
                for(size_t i = mAnalyzers.size(); i < anlAcsrs.size(); ++i)
                {
                    mAnalyzers.push_back(std::make_unique<Analyzer::Director>(anlAcsrs[i]));
                    mAnalyzers[i]->setAudioFormatReader(createAudioFormatReader(mAccessor, mAudioFormatManager, AlertType::silent), notification);
                }
            }
                break;
        }
    };
}

ANALYSE_FILE_END
