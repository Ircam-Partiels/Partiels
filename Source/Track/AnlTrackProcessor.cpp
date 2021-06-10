#include "AnlTrackProcessor.h"
#include "AnlTrackTools.h"
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

    anlWeakAssert(state.blockSize > 0 && state.stepSize > 0);
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
        return std::make_tuple(WarningType::plugin, e.what(), Plugin::Output{});
    }
    catch(...)
    {
        return std::make_tuple(WarningType::plugin, "unknown error", Plugin::Output{});
    }

    if(processor == nullptr)
    {
        return std::make_tuple(WarningType::state, "invalid state", Plugin::Output{});
    }

    auto const output = processor->getOutput();

    mChrono.start();
    mAnalysisProcess = std::async([this, processor = std::move(processor)]() -> Results
                                  {
                                      juce::Thread::setCurrentThreadName("Track::Processor::Process");
                                      juce::Thread::setCurrentThreadPriority(10);

                                      auto expected = ProcessState::available;
                                      if(!mAnalysisState.compare_exchange_weak(expected, ProcessState::running))
                                      {
                                          triggerAsyncUpdate();
                                          return {};
                                      }
                                      expected = ProcessState::running;

                                      std::vector<std::vector<Plugin::Result>> pluginResults;
                                      if(!processor->prepareToAnalyze(pluginResults))
                                      {
                                          mAnalysisState.compare_exchange_weak(expected, ProcessState::aborted);
                                          triggerAsyncUpdate();
                                          return {};
                                      }

                                      while(mAnalysisState.load() != ProcessState::aborted && processor->performNextAudioBlock(pluginResults))
                                      {
                                          mAdvancement.store(processor->getAdvancement());
                                      }

                                      auto const results = Tools::getResults(processor->getOutput(), pluginResults);
                                      mAdvancement.store(1.0f);

                                      if(mAnalysisState.compare_exchange_weak(expected, ProcessState::ended))
                                      {
                                          triggerAsyncUpdate();
                                          return results;
                                      }

                                      triggerAsyncUpdate();
                                      return {};
                                  });

    return std::make_tuple(WarningType::none, "", std::move(output));
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

bool Track::Processor::isRunning() const
{
    return mAnalysisProcess.valid();
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
