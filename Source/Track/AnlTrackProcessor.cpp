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
    auto const numChannels = reader.numChannels;

    mChrono.start();
    mAnalysisProcess = std::async([this, numChannels, processor = std::move(processor)]() -> Results
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

                                      std::vector<Plugin::Result> pluginResults;
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

                                      auto rtToS = [](Vamp::RealTime const& rt)
                                      {
                                          return static_cast<double>(rt.sec) + static_cast<double>(rt.nsec) / 1000000000.0;
                                      };

                                      auto getResults = [&]() -> Results
                                      {
                                          auto getBinCount = [&]() -> size_t
                                          {
                                              if(processor->getOutput().hasFixedBinCount)
                                              {
                                                  return processor->getOutput().binCount;
                                              }
                                              auto it = std::max_element(pluginResults.cbegin(), pluginResults.cend(), [](auto const& lhs, auto const& rhs)
                                                                         {
                                                                             return lhs.values.size() < rhs.values.size();
                                                                         });
                                              if(it != pluginResults.cend())
                                              {
                                                  return it->values.size();
                                              }
                                              return 0_z;
                                          };
                                          switch(getBinCount())
                                          {
                                              case 0_z:
                                              {
                                                  std::vector<Results::Marker> markers;
                                                  markers.reserve(pluginResults.size());
                                                  for(auto const& result : pluginResults)
                                                  {
                                                      anlWeakAssert(result.hasTimestamp);
                                                      if(result.hasTimestamp)
                                                      {
                                                          markers.push_back(std::make_tuple(rtToS(result.timestamp), result.hasDuration ? rtToS(result.duration) : 0.0, result.label));
                                                      }
                                                  }
                                                  std::vector<Results::Markers> results;
                                                  for(size_t i = 0; i < numChannels; ++i)
                                                  {
                                                      results.push_back(markers);
                                                  }
                                                  return Results(std::make_shared<const std::vector<Results::Markers>>(std::move(results)));
                                              }
                                              break;
                                              case 1_z:
                                              {
                                                  std::vector<Results::Point> points;
                                                  points.reserve(pluginResults.size());
                                                  for(auto const& result : pluginResults)
                                                  {
                                                      anlWeakAssert(result.hasTimestamp);
                                                      if(result.hasTimestamp)
                                                      {
                                                          auto const valid = !result.values.empty() && std::isfinite(result.values[0]) && !std::isnan(result.values[0]);
                                                          points.push_back(std::make_tuple(rtToS(result.timestamp), result.hasDuration ? rtToS(result.duration) : 0.0, valid ? result.values[0] : std::optional<float>()));
                                                      }
                                                  }
                                                  std::vector<Results::Points> results;
                                                  for(size_t i = 0; i < numChannels; ++i)
                                                  {
                                                      results.push_back(points);
                                                  }
                                                  return Results(std::make_shared<const std::vector<Results::Points>>(std::move(results)));
                                              }
                                              break;
                                              default:
                                              {
                                                  std::vector<Results::Column> columns;
                                                  columns.reserve(pluginResults.size());
                                                  for(auto& result : pluginResults)
                                                  {
                                                      anlWeakAssert(result.hasTimestamp);
                                                      if(result.hasTimestamp)
                                                      {
                                                          columns.push_back(std::make_tuple(rtToS(result.timestamp), result.hasDuration ? rtToS(result.duration) : 0.0, std::move(result.values)));
                                                      }
                                                  }
                                                  std::vector<Results::Columns> results;
                                                  for(size_t i = 0; i < numChannels; ++i)
                                                  {
                                                      results.push_back(columns);
                                                  }
                                                  return Results(std::make_shared<const std::vector<Results::Columns>>(std::move(results)));
                                              }
                                              break;
                                          }
                                      };

                                      auto const results = getResults();
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
