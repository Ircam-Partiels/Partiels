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
    mAccessor.onAttrUpdated = nullptr;
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
        if(!mAccessor.insertAccessor<Document::AcsrType::analyzers>(-1, NotificationType::synchronous))
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

        auto& anlAcsr = mAccessor.getAccessors<Document::AcsrType::analyzers>().back().get();
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
    acsr.onAttrUpdated = [&](AttrType attribute, NotificationType notification)
    {
        switch (attribute)
        {
            case AttrType::file:
            {
                auto reader = createAudioFormatReader(acsr, mAudioFormatManager, AlertType::window);
                if(reader == nullptr)
                {
                    return;
                }
                auto const sampleRate = reader->sampleRate;
                auto const duration = sampleRate > 0.0 ? static_cast<double>(reader->lengthInSamples) / sampleRate : 0.0;
                
                auto& zoomAcsr = acsr.getAccessor<AcsrType::timeZoom>(0);
                zoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{0.0, duration}, notification);
                zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(duration / 100.0, notification);
                zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(Zoom::Range{0.0, duration}, notification);
                
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
                auto const time = acsr.getAttr<AttrType::playheadPosition>();
                auto& zoomAcsr = acsr.getAccessor<AcsrType::timeZoom>(0);
                auto const range = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
                if(!range.contains(time))
                {
                    zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range.movedToStartAt(time), notification);
                }
            }
                break;
            case AttrType::layoutHorizontal:
                break;
        }
    };
    
    acsr.onAcsrInserted = [&](AcsrType attribute, size_t index, NotificationType notification)
    {
        switch (attribute)
        {
            case AcsrType::analyzers:
            {
                anlStrongAssert(index <= mAnalyzers.size());
                auto& anlAcsr = acsr.getAccessor<AcsrType::analyzers>(index);
                auto director = std::make_unique<Analyzer::Director>(anlAcsr);
                anlStrongAssert(director != nullptr);
                if(director != nullptr)
                {
                    director->setAudioFormatReader(createAudioFormatReader(mAccessor, mAudioFormatManager, AlertType::silent), notification);
                }
                mAnalyzers.insert(mAnalyzers.begin() + static_cast<long>(index), std::move(director));
                
                auto& layoutAcsr = acsr.getAccessor<AcsrType::layout>(0);
                auto sizes = layoutAcsr.getAttr<Layout::StrechableContainer::AttrType::sizes>();
                if(sizes.size() < mAnalyzers.size())
                {
                    sizes.insert(sizes.begin() + static_cast<long>(index), 100);
                    layoutAcsr.setAttr<Layout::StrechableContainer::AttrType::sizes>(sizes, notification);
                }
            }
                break;
            case AcsrType::timeZoom:
            case AcsrType::layout:
                break;
        }
    };
    
    acsr.onAcsrErased = [&](AcsrType attribute, size_t index, NotificationType notification)
    {
        switch (attribute)
        {
            case AcsrType::analyzers:
            {
                anlStrongAssert(index < mAnalyzers.size());
                mAnalyzers.erase(mAnalyzers.begin() + static_cast<long>(index));
                
                auto& layoutAcsr = acsr.getAccessor<AcsrType::layout>(0);
                auto sizes = layoutAcsr.getAttr<Layout::StrechableContainer::AttrType::sizes>();
                sizes.erase(sizes.begin() + static_cast<long>(index));
                layoutAcsr.setAttr<Layout::StrechableContainer::AttrType::sizes>(sizes, notification);
            }
                break;
            case AcsrType::timeZoom:
            case AcsrType::layout:
                break;
        }
    };
}

ANALYSE_FILE_END
