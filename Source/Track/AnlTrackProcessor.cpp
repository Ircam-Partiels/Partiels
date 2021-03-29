#include "AnlTrackProcessor.h"
#include "../Plugin/AnlPluginProcessor.h"

ANALYSE_FILE_BEGIN

Track::Processor::~Processor()
{
    std::unique_lock<std::mutex> lock(mAnalysisMutex);
    abortAnalysis();
}

void Track::Processor::stopAnalysis()
{
    std::unique_lock<std::mutex> lock(mAnalysisMutex);
    abortAnalysis();
}

Track::Processor::Result Track::Processor::runAnalysis(Accessor const& accessor, juce::AudioFormatReader& reader)
{
    auto const& description = accessor.getAttr<AttrType::description>();
    auto const& state = accessor.getAttr<AttrType::state>();
    
    std::unique_lock<std::mutex> lock(mAnalysisMutex, std::try_to_lock);
    anlStrongAssert(lock.owns_lock());
    if(!lock.owns_lock())
    {
        return {};
    }
    abortAnalysis();
    
    auto const key = accessor.getAttr<AttrType::key>();
    if(key.identifier.empty() || key.feature.empty())
    {
        return {};
    }
    
    anlStrongAssert(state.blockSize > 0 && state.stepSize > 0);
    if(state.blockSize == 0 || state.stepSize == 0)
    {
        return {};
    }
    
    std::unique_ptr<Plugin::Processor> processor;
    try
    {
        processor = Plugin::Processor::create(key, state, reader);
    }
    catch(std::exception& e)
    {
        return std::make_tuple(WarningType::plugin, e.what(), description, state);
    }
    catch(...)
    {
        return std::make_tuple(WarningType::plugin, "unknown error", description, state);
    }
    
    if(processor == nullptr)
    {
        return std::make_tuple(WarningType::state, "invalid state", description, state);
    }
    
    auto const pluginDescription = processor->getDescription();
    anlStrongAssert(pluginDescription != Plugin::Description{});
    if(pluginDescription == Plugin::Description{})
    {
        return std::make_tuple(WarningType::plugin, "invalid description", pluginDescription, state);
    }
    
    mChrono.start();
    mAnalysisProcess = std::async([this, processor = std::move(processor)]() -> std::shared_ptr<std::vector<Plugin::Result>>
    {
        juce::Thread::setCurrentThreadName("Track::Director::runAnalysis");
        juce::Thread::setCurrentThreadPriority(10);
        
        auto expected = ProcessState::available;
        if(!mAnalysisState.compare_exchange_weak(expected, ProcessState::running))
        {
            triggerAsyncUpdate();
            return {};
        }
        expected = ProcessState::running;

        auto results = std::make_shared<std::vector<Plugin::Result>>();
        if(results == nullptr)
        {
            mAnalysisState.compare_exchange_weak(expected, ProcessState::aborted);
            triggerAsyncUpdate();
            return {};
        }
        
        if(!processor->prepareToAnalyze(*results))
        {
            mAnalysisState.compare_exchange_weak(expected, ProcessState::aborted);
            triggerAsyncUpdate();
            return {};
        }
        
        while(mAnalysisState.load() != ProcessState::aborted && processor->performNextAudioBlock(*results))
        {
            mAdvancement.store(processor->getAdvancement());
        }
        mAdvancement.store(1.0f);
        if(mAnalysisState.compare_exchange_weak(expected, ProcessState::ended))
        {
            triggerAsyncUpdate();
            return results;
        }
        
        triggerAsyncUpdate();
        return {};
    });
    
    return std::make_tuple(WarningType::none, "", pluginDescription, state);
}

void Track::Processor::handleAsyncUpdate()
{
    std::unique_lock<std::mutex> lock(mAnalysisMutex);
    if(mAnalysisProcess.valid())
    {
        anlWeakAssert(mAnalysisState != ProcessState::available);
        anlWeakAssert(mAnalysisState != ProcessState::running);
        
        auto const results = mAnalysisProcess.get();
        auto expected = ProcessState::ended;
        if(mAnalysisState.compare_exchange_weak(expected, ProcessState::available))
        {
            mChrono.stop();
            if(onAnalysisEnded != nullptr)
            {
                onAnalysisEnded(results);
            }
        }
        else if(expected == ProcessState::aborted)
        {
            if(onAnalysisAborted != nullptr)
            {
                onAnalysisAborted();
            }
        }
        abortAnalysis();
    }
}

float Track::Processor::getAdvancement() const
{
    return mAdvancement.load();
}

void Track::Processor::abortAnalysis()
{
    mAdvancement.store(0.0f);
    if(mAnalysisProcess.valid())
    {
        mAnalysisState = ProcessState::aborted;
        mAnalysisProcess.get();
        cancelPendingUpdate();
        
        if(onAnalysisAborted != nullptr)
        {
            onAnalysisAborted();
        }
    }
    mAnalysisState = ProcessState::available;
}

ANALYSE_FILE_END
