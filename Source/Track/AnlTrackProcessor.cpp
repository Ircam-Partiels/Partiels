#include "AnlTrackProcessor.h"
#include "../Plugin/AnlPluginProcessor.h"
#include "AnlTrackTools.h"

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

bool Track::Processor::runAnalysis(Accessor const& accessor, juce::AudioFormatReader& reader)
{
    auto state = accessor.getAttr<AttrType::state>();

    std::unique_lock<std::mutex> lock(mAnalysisMutex, std::try_to_lock);
    anlWeakAssert(lock.owns_lock());
    if(!lock.owns_lock())
    {
        anlError("Track", "Concurrent thread access!");
        return false;
    }
    abortAnalysis();

    auto const key = accessor.getAttr<AttrType::key>();
    if(key.identifier.empty() || key.feature.empty())
    {
        return false;
    }

    if(state.blockSize == 0_z)
    {
        state.blockSize = 64_z;
    }
    if(state.stepSize == 0_z)
    {
        state.stepSize = state.blockSize;
    }

    auto processor = Plugin::Processor::create(key, state, reader);
    if(processor == nullptr)
    {
        throw std::runtime_error("allocation failed");
    }

    mChrono.start();
    mAnalysisProcess = std::async([this, processor = std::move(processor)]() -> Results
                                  {
                                      anlDebug("Track", "Processor thread launched");
                                      juce::Thread::setCurrentThreadName("Track::Processor::Process");

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
                                      anlDebug("Track", "Processor prepared");

                                      while(mAnalysisState.load() != ProcessState::aborted && processor->performNextAudioBlock(pluginResults))
                                      {
                                          mAdvancement.store(processor->getAdvancement());
                                      }

                                      anlDebug("Track", "Processor performed");

                                      auto const results = Tools::getResults(processor->getOutput(), pluginResults, mShouldAbort);
                                      mAdvancement.store(1.0f);

                                      anlDebug("Track", "Results performed");

                                      if(mAnalysisState.compare_exchange_weak(expected, ProcessState::ended))
                                      {
                                          triggerAsyncUpdate();
                                          return results;
                                      }

                                      triggerAsyncUpdate();
                                      return {};
                                  });
    return true;
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
        mShouldAbort = true;
        mAnalysisProcess.get();
        cancelPendingUpdate();

        if(onAnalysisAborted != nullptr)
        {
            onAnalysisAborted();
        }
    }
    mShouldAbort = false;
    mAnalysisState = ProcessState::available;
}

ANALYSE_FILE_END
