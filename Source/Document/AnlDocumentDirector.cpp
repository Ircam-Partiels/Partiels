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
                auto const title = juce::translate("Analysis cannot be created");
                auto const message = juce::translate("The analysis ANNM cannot be inserted into the document.").replace("ANNM", name);
                juce::AlertWindow::showMessageBox(icon, title, message);
            }
            return;
        }

        auto& anlAcsr = mAccessor.getAccessors<Document::AttrType::analyzers>().back().get();
        anlAcsr.setAttr<Analyzer::AttrType::key>(key, NotificationType::synchronous);
        anlAcsr.setAttr<Analyzer::AttrType::name>(name, NotificationType::synchronous);
        anlAcsr.setAttr<Analyzer::AttrType::colour>(juce::Colours::blue, NotificationType::synchronous);
        anlAcsr.setAttr<Analyzer::AttrType::colourMap>(Analyzer::ColorMap::Heat, NotificationType::synchronous);
        
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
            case AttrType::file:
            {
                auto reader = createAudioFormatReader(mAccessor, mAudioFormatManager, AlertType::window);
                if(reader == nullptr)
                {
                    return;
                }
                auto const sampleRate = reader->sampleRate;
                auto const duration = sampleRate > 0.0 ? static_cast<double>(reader->lengthInSamples) / sampleRate : 0.0;
                
                auto& zoomAcsr = mAccessor.getAccessor<AttrType::timeZoom>(0);
                zoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{0.0, duration}, notification);
                
                auto anlAcsrs = mAccessor.getAccessors<AttrType::analyzers>();
                for(auto& anlAcsr : anlAcsrs)
                {
                    Analyzer::performAnalysis(anlAcsr, *reader.get(), 512, notification);
                }
            }
                break;
            case isLooping:
            case gain:
            case isPlaybackStarted:
            case playheadPosition:
            case timeZoom:
            case layout:
                break;
            case analyzers:
            {
                auto anlAcsrs = acsr.getAccessors<AttrType::analyzers>();
                for(auto& anlAcsr : anlAcsrs)
                {
                    JUCE_COMPILER_WARNING("Remove onUpdate");
                    setupAnalyzer(anlAcsr);
                }
            }
                break;
        }
    };
}

void Document::Director::setupAnalyzer(Analyzer::Accessor& acsr)
{
    acsr.onUpdated = [&](Analyzer::AttrType anlAttr, NotificationType notification)
    {
        switch (anlAttr)
        {
            case Analyzer::key:
            case Analyzer::AttrType::feature:
            case Analyzer::AttrType::parameters:
            case Analyzer::blockSize:
            {
                std::thread thd([&]()
                {
                    auto reader = createAudioFormatReader(mAccessor, mAudioFormatManager, AlertType::window);
                    if(reader == nullptr)
                    {
                        return;
                    }
                    
                    Analyzer::performAnalysis(acsr, *reader.get(), 512, NotificationType::asynchronous);
                    triggerAsyncUpdate();
                });
                mThreads.emplace_back(std::move(thd));
            }
                break;
            case Analyzer::AttrType::zoom:
            case Analyzer::AttrType::name:
            case Analyzer::AttrType::colour:
            case Analyzer::AttrType::colourMap:
            case Analyzer::AttrType::results:
                break;
        }
    };
}

void Document::Director::handleAsyncUpdate()
{
    JUCE_COMPILER_WARNING("Improve")
    auto it = std::remove_if(mThreads.begin(), mThreads.end(), [](auto const& thd)
    {
        return thd.joinable();
    });
    for(auto end = it; end != mThreads.end(); ++end)
    {
        end->join();
    }
    mThreads.erase(it, mThreads.end());
}

ANALYSE_FILE_END
