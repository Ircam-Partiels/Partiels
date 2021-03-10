#include "AnlAnalyzerDirector.h"
#include "AnlAnalyzerPropertyPanel.h"
#include "AnlAnalyzerRenderer.h"

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
            case AttrType::state:
            {
                runAnalysis(notification);
            }
                break;
            case AttrType::results:
            case AttrType::name:
            case AttrType::description:
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
    
    auto createProcessor = [&]() -> std::unique_ptr<Plugin::Processor>
    {
        try
        {
            return Plugin::Processor::create(key, state, *reader);
        }
        catch(std::exception& e)
        {
            MessageWindow::show(MessageWindow::MessageType::warning,
                                "Plugin cannot be loaded",
                                "The plugin cannot be loaded due to: REASON.",
                                {{"REASON", e.what()}});
        }
        catch(...)
        {
            MessageWindow::show(MessageWindow::MessageType::warning,
                                "Plugin cannot be loaded",
                                "The plugin cannot be loaded due to: REASON.",
                                {{"REASON", "unknwon error"}});
        }
        return nullptr;
    };
    
    auto processor = createProcessor();
    if(processor == nullptr)
    {
        std::map<WarningType, juce::String> warning;
        warning[WarningType::state] = juce::translate("The step size or the block size might not be supported");
        mAccessor.setAttr<AttrType::warnings>(warning, notification);
        
        lock.unlock();
        if(juce::AlertWindow::showOkCancelBox(juce::AlertWindow::AlertIconType::WarningIcon, juce::translate("Plugin cannot be loaded"), juce::translate("Initialization failed, the step size or the block size might not be supported. Would you like to use the plugin default value for the block size and the step size") + "?"))
        {
            auto newState = state;
            auto const& description = mAccessor.getAttr<AttrType::description>();
            newState.blockSize = description.defaultState.blockSize;
            newState.stepSize = description.defaultState.stepSize;
            mAccessor.setAttr<AttrType::state>(newState, notification);
        }
        return;
    }
    mAccessor.setAttr<AttrType::warnings>(std::map<WarningType, juce::String>{}, notification);
    

    auto description = processor->getDescription();
    anlWeakAssert(description != Plugin::Description{});
    if(description == Plugin::Description{})
    {
        return;
    }
    mAccessor.setAttr<AttrType::description>(description, notification);
    updateZooms(notification);
    
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

void Analyzer::Director::updateZooms(NotificationType const notification)
{
    JUCE_COMPILER_WARNING("If zoom extent is defined, the zoom can be updated before the analysis")
    
    mUpdateZoom = std::make_tuple(false, notification);
    auto getZoomInfo = [&]() -> std::tuple<Zoom::Range, double>
    {
        Zoom::Range range;
        bool initialized = false;
        auto const& results = mAccessor.getAttr<AttrType::results>();
        for(auto const& result : results)
        {
            auto const& values = result.values;
            auto const pair = std::minmax_element(values.cbegin(), values.cend());
            if(pair.first != values.cend() && pair.second != values.cend())
            {
                auto const start = static_cast<double>(*pair.first);
                auto const end = static_cast<double>(*pair.second);
                range = !initialized ? Zoom::Range{start, end} : range.getUnionWith({start, end});
                initialized = true;
            }
        }
        auto constexpr epsilon = std::numeric_limits<double>::epsilon() * 100.0;
        return !initialized ? std::make_tuple(Zoom::Range{0.0, 1.0}, 1.0) : std::make_tuple(range, epsilon);
    };
    
    auto& valueZoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
    auto const& output = mAccessor.getAttr<AttrType::description>().output;
    if(output.hasKnownExtents)
    {
        valueZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{static_cast<double>(output.minValue), static_cast<double>(output.maxValue)}, notification);
        valueZoomAcsr.setAttr<Zoom::AttrType::minimumLength>((output.isQuantized ? static_cast<double>(output.quantizeStep) : std::numeric_limits<double>::epsilon()), notification);
    }
    else
    {
        auto const& results = mAccessor.getAttr<AttrType::results>();
        if(results.empty())
        {
            mUpdateZoom = std::make_tuple(true, notification);
            return;
        }
        
        auto const info = getZoomInfo();
        valueZoomAcsr.setAttr<Zoom::AttrType::globalRange>(std::get<0>(info), notification);
        valueZoomAcsr.setAttr<Zoom::AttrType::minimumLength>(std::get<1>(info), notification);
    }
    
    auto const& results = mAccessor.getAttr<AttrType::results>();
    if(results.empty())
    {
        mUpdateZoom = std::make_tuple(true, notification);
        return;
    }
    
    auto& binZoomAcsr = mAccessor.getAccessor<AcsrType::binZoom>(0);
    binZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{0.0, static_cast<double>(results[0].values.size())}, notification);
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
            if(std::get<0>(mUpdateZoom))
            {
                updateZooms(std::get<1>(mUpdateZoom));
            }
        }
        
        std::unique_lock<std::mutex> lock(mAnalysisMutex);
        stopAnalysis(std::get<1>(result));
    }
}

ANALYSE_FILE_END
