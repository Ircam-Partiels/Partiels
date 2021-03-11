#include "AnlAnalyzerDirector.h"
#include "AnlAnalyzerPropertyPanel.h"
#include "AnlAnalyzerResults.h"

#include "../Plugin/AnlPluginProcessor.h"
#include "../Plugin/AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

Analyzer::Director::Director(Accessor& accessor, PluginList::Scanner& pluginListScanner, std::unique_ptr<juce::AudioFormatReader> audioFormatReader)
: mAccessor(accessor)
, mPluginListScanner(pluginListScanner)
, mAudioFormatReaderManager(std::move(audioFormatReader))
{
    accessor.onAttrUpdated = [&](AttrType anlAttr, NotificationType notification)
    {
        switch(anlAttr)
        {
            case AttrType::key:
            {
                try
                {
                    auto const& description = mPluginListScanner.getDescription(accessor.getAttr<AttrType::key>());
                    mAccessor.setAttr<AttrType::name>(description.name, NotificationType::synchronous);
                    mAccessor.setAttr<AttrType::description>(description, NotificationType::synchronous);
                    mAccessor.setAttr<AttrType::state>(description.defaultState, NotificationType::synchronous);
                }
                catch(...)
                {
                    
                }
            }
                break;
            case AttrType::state:
            {
                runAnalysis(notification);
            }
                break;
            case AttrType::description:
            {
                updateZoomAccessors(notification);
            }
                break;
            case AttrType::results:
            {
                updateZoomAccessors(notification);
            }
                break;
            case AttrType::name:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::colours:
            case AttrType::propertyState:
            case AttrType::time:
            case AttrType::warnings:
            case AttrType::processing:
                break;
        }
    };
    runAnalysis(NotificationType::synchronous);
}

Analyzer::Director::~Director()
{
    mAccessor.onAttrUpdated = nullptr;
    mAccessor.onAccessorInserted = nullptr;
    mAccessor.onAccessorErased = nullptr;
    
    std::unique_lock<std::mutex> lock(mAnalysisMutex);
    stopAnalysis(NotificationType::synchronous);
}

void Analyzer::Director::setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, NotificationType const notification)
{
    anlStrongAssert(audioFormatReader == nullptr || audioFormatReader != mAudioFormatReaderManager);
    if(audioFormatReader == mAudioFormatReaderManager)
    {
        return;
    }
    
    std::unique_lock<std::mutex> lock(mAnalysisMutex);
    stopAnalysis(notification);
    mAudioFormatReaderManager = std::move(audioFormatReader);
    lock.unlock();
    
    runAnalysis(notification);
}

void Analyzer::Director::stopAnalysis(NotificationType const notification)
{
    if(mAnalysisProcess.valid())
    {
        mAnalysisState = ProcessState::aborted;
        mAnalysisProcess.get();
        cancelPendingUpdate();
    }
    mAnalysisState = ProcessState::available;
    mAccessor.setAttr<AttrType::processing>(false, notification);
}

void Analyzer::Director::runAnalysis(NotificationType const notification)
{
    std::unique_lock<std::mutex> lock(mAnalysisMutex, std::try_to_lock);
    anlStrongAssert(lock.owns_lock());
    if(!lock.owns_lock())
    {
        return;
    }
    stopAnalysis(notification);
    
    auto* reader = mAudioFormatReaderManager.get();
    if(reader == nullptr)
    {
        return;
    }
    
    auto const key = mAccessor.getAttr<AttrType::key>();
    if(key.identifier.empty() || key.feature.empty())
    {
        return;
    }
    
    auto const state = mAccessor.getAttr<AttrType::state>();
    anlStrongAssert(state.blockSize > 0 && state.stepSize > 0);
    if(state.blockSize == 0 || state.stepSize == 0)
    {
        return;
    }
    
    auto showWarningWindow = [&](juce::String const& reason)
    {
        MessageWindow::show(MessageWindow::MessageType::warning,
                            "Plugin cannot be loaded",
                            "The plugin \"KEYID - KEYFEATURE\" of the track \"TRACKNAME\" cannot be loaded due to: REASON.",
                            {
                                  {"KEYID", key.identifier}
                                , {"KEYFEATURE", key.feature}
                                , {"TRACKNAME", mAccessor.getAttr<AttrType::name>()}
                                , {"REASON", reason}
                            });
    };
    
    std::unique_ptr<Plugin::Processor> processor;
    try
    {
        processor = Plugin::Processor::create(key, state, *reader);
    }
    catch(std::exception& e)
    {
        showWarningWindow(e.what());
        mAccessor.setAttr<AttrType::warnings>(WarningType::plugin, notification);
        return;
    }
    catch(...)
    {
        showWarningWindow("unknown error");
        mAccessor.setAttr<AttrType::warnings>(WarningType::plugin, notification);
        return;
    }
    
    if(processor == nullptr)
    {
        mAccessor.setAttr<AttrType::warnings>(WarningType::state, notification);
        auto const& description = mAccessor.getAttr<AttrType::description>();
        if(state.blockSize == description.defaultState.blockSize && state.stepSize == description.defaultState.stepSize)
        {
            showWarningWindow("invalid state");
            return;
        }
        if(juce::AlertWindow::showOkCancelBox(juce::AlertWindow::AlertIconType::WarningIcon,
                                              juce::translate("Plugin cannot be loaded"),
                                              juce::translate("The plugin \"KEYID - KEYFEATURE\" of the track \"TRACKNAME\" cannot be initialized because the step size or the block size might not be supported. Would you like to use the plugin default value for the block size and the step size?")))
        {
            auto newState = state;
            newState.blockSize = description.defaultState.blockSize;
            newState.stepSize = description.defaultState.stepSize;
            lock.unlock();
            mAccessor.setAttr<AttrType::state>(newState, notification);
        }
        return;
    }
    auto description = processor->getDescription();
    anlStrongAssert(description != Plugin::Description{});
    if(description == Plugin::Description{})
    {
        showWarningWindow("invalid description");
        mAccessor.setAttr<AttrType::warnings>(WarningType::plugin, notification);
        return;
    }
    
    mAccessor.setAttr<AttrType::description>(description, notification);
    mAccessor.setAttr<AttrType::warnings>(WarningType::none, notification);
    mAccessor.setAttr<AttrType::processing>(true, notification);
    
    mAnalysisProcess = std::async([this, notification, processor = std::move(processor)]() -> std::tuple<std::vector<Plugin::Result>, NotificationType>
    {
        juce::Thread::setCurrentThreadName("Analyzer::Director::runAnalysis");
        
        auto expected = ProcessState::available;
        if(!mAnalysisState.compare_exchange_weak(expected, ProcessState::running))
        {
            triggerAsyncUpdate();
            return std::make_tuple(std::vector<Plugin::Result>{}, notification);
        }

        std::vector<Plugin::Result> results;
        while(mAnalysisState.load() != ProcessState::aborted && processor->performNextAudioBlock(results))
        {
        }
        expected = ProcessState::running;
        if(mAnalysisState.compare_exchange_weak(expected, ProcessState::ended))
        {
            triggerAsyncUpdate();
            return std::make_tuple(std::move(results), notification);
        }
        triggerAsyncUpdate();
        return std::make_tuple(std::vector<Plugin::Result>{}, notification);
    });
}

void Analyzer::Director::updateZoomAccessors(NotificationType const notification)
{
    auto const& results = mAccessor.getAttr<AttrType::results>();
    auto const& output = mAccessor.getAttr<AttrType::description>().output;
    
    auto& valueZoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
    if(valueZoomAcsr.getAttr<Zoom::AttrType::globalRange>() == Zoom::Range{0.0, 0.0})
    {
        if(output.hasKnownExtents)
        {
            valueZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{static_cast<double>(output.minValue), static_cast<double>(output.maxValue)}, notification);
        }
        else if(!results.empty())
        {
            valueZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Results::getValueRange(results), notification);
        }
        valueZoomAcsr.setAttr<Zoom::AttrType::minimumLength>(output.isQuantized ? static_cast<double>(output.quantizeStep) : Zoom::epsilon(), notification);
        valueZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(valueZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), notification);
    }
    
    auto& binZoomAcsr = mAccessor.getAccessor<AcsrType::binZoom>(0);
    if(output.hasFixedBinCount)
    {
        binZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{0.0, static_cast<double>(output.binCount)}, notification);
    }
    else if(!results.empty())
    {
        binZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Results::getBinRange(results), notification);
    }
    if(binZoomAcsr.getAttr<Zoom::AttrType::visibleRange>() == Zoom::Range{0.0, 0.0})
    {
        binZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(binZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), notification);
    }
    binZoomAcsr.setAttr<Zoom::AttrType::minimumLength>(1.0, notification);
}

void Analyzer::Director::handleAsyncUpdate()
{
    if(mAnalysisProcess.valid())
    {
        anlWeakAssert(mAnalysisState != ProcessState::available);
        anlWeakAssert(mAnalysisState != ProcessState::running);
        
        auto const result = mAnalysisProcess.get();
        auto expected = ProcessState::ended;
        if(mAnalysisState.compare_exchange_weak(expected, ProcessState::available))
        {
            mAccessor.setAttr<AttrType::results>(std::get<0>(result), std::get<1>(result));
        }
        
        std::unique_lock<std::mutex> lock(mAnalysisMutex);
        stopAnalysis(std::get<1>(result));
    }
}

ANALYSE_FILE_END
