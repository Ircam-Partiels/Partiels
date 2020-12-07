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
                
                auto anlAcsrs = acsr.getAccessors<AttrType::analyzers>();
                for(auto& anlAcsr : anlAcsrs)
                {
                    anlAcsr.get().onUpdated(Analyzer::AttrType::key, notification);
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
            case Analyzer::AttrType::key:
            case Analyzer::AttrType::feature:
            case Analyzer::AttrType::parameters:
            {
                std::thread thd([&]() mutable
                {
                    auto reader = createAudioFormatReader(mAccessor, mAudioFormatManager, AlertType::window);
                    if(reader == nullptr)
                    {
                        triggerAsyncUpdate();
                        return;
                    }
                    
                    auto results = Analyzer::performAnalysis(acsr, *reader.get());
                    if(results.empty())
                    {
                        triggerAsyncUpdate();
                        return;
                    }
                    
                    auto getZoomState = [&]() -> std::pair<double, Zoom::Range>
                    {
                        auto const numDimension = results.front().values.size() + 1;
                        if(numDimension == 1)
                        {
                            return {1.0, {0.0, 1.0}};
                        }
                        else if(numDimension == 2)
                        {
                            auto pair = std::minmax_element(results.cbegin(), results.cend(), [](auto const& lhs, auto const& rhs)
                            {
                                return lhs.values[0] < rhs.values[0];
                            });
                            return {std::numeric_limits<double>::epsilon(), {static_cast<double>(pair.first->values[0]), static_cast<double>(pair.second->values[0])}};
                        }
                        else
                        {
                            auto rit = std::max_element(results.cbegin(), results.cend(), [](auto const& lhs, auto const& rhs)
                            {
                                return lhs.values.size() < rhs.values.size();
                            });
                            return {1.0, {0.0, static_cast<double>(rit->values.size())}};
                        }
                    };
                    
                    auto const zoomState = getZoomState();
                    
                    auto ret = std::make_shared<results_container>(zoomState.first, zoomState.second, std::move(results));
                    
                    std::unique_lock<std::mutex> lock(mMutex);
                    auto it = std::find_if(mProcesses.begin(), mProcesses.end(), [](auto const& process)
                    {
                        return std::get<0>(process).get_id() == std::this_thread::get_id();
                    });
                    anlWeakAssert(it != mProcesses.end());
                    std::get<2>(*it) = ret;

                    triggerAsyncUpdate();
                });
                
                std::unique_lock<std::mutex> lock(mMutex);
                mProcesses.push_back({std::move(thd), acsr, nullptr, notification});
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
    auto it = std::remove_if(mProcesses.begin(), mProcesses.end(), [](auto const& process)
    {
        return std::get<0>(process).joinable();
    });
    for(auto end = it; end != mProcesses.end(); ++end)
    {
        std::get<0>(*end).join();
        if(std::get<2>(*end) != nullptr)
        {
            auto const notification = std::get<3>(*end);
            auto& anlAcsr = std::get<1>(*end).get();
            auto& zoomAcsr = anlAcsr.getAccessor<Analyzer::AttrType::zoom>(0);
            zoomAcsr.setAttr<Zoom::AttrType::globalRange>(std::get<2>(*end)->range, notification);
            zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(std::get<2>(*end)->minimumLength, notification);
            
            anlAcsr.setAttr<Analyzer::AttrType::results>(std::get<2>(*end)->results, notification);
        }
    }
    std::unique_lock<std::mutex> lock(mMutex);
    mProcesses.erase(it, mProcesses.end());
}

ANALYSE_FILE_END
