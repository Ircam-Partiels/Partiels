#include "AnlTrackProcessor.h"
#include "../Plugin/AnlPluginListScanner.h"
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

std::optional<Plugin::Description> Track::Processor::runAnalysis(Accessor const& accessor, juce::AudioFormatReader& reader, Results input)
{
    std::unique_lock<std::mutex> lock(mAnalysisMutex, std::try_to_lock);
    anlWeakAssert(lock.owns_lock());
    if(!lock.owns_lock())
    {
        anlError("Track", "Concurrent thread access!");
        return {};
    }
    abortAnalysis();

    auto const key = accessor.getAttr<AttrType::key>();
    if(key.identifier.empty() || key.feature.empty())
    {
        return {};
    }
    // This is a specific case that speed up the waveform analysis
    if(key.identifier == "partiels-vamp-plugins:partielswaveform" && key.feature == "peaks")
    {
        mChrono.start();
        mAnalysisProcess = std::async([&, this]() -> Results
                                      {
                                          anlDebug("Track", "Processor thread launched");
                                          juce::Thread::setCurrentThreadName("Track::Processor::Process");

                                          mAdvancement.store(0.0f);
                                          auto expected = ProcessState::available;
                                          if(!mAnalysisState.compare_exchange_weak(expected, ProcessState::running))
                                          {
                                              triggerAsyncUpdate();
                                              return {};
                                          }
                                          expected = ProcessState::running;

                                          static size_t constexpr maxSize = 10 * 48000; // 10 seconds at 48000
                                          static size_t constexpr expectedBlock = 8192;

                                          auto const lengthInSamples = static_cast<size_t>(reader.lengthInSamples);
                                          auto const duration = static_cast<double>(reader.lengthInSamples) / reader.sampleRate;
                                          auto const timeRatio = duration / static_cast<double>(reader.lengthInSamples);
                                          auto const sizeRatio = static_cast<double>(lengthInSamples) / static_cast<double>(maxSize);
                                          auto const stepSize = static_cast<size_t>(std::floor(std::max(sizeRatio, 1.0)));

                                          auto const outputSize = lengthInSamples / stepSize + ((lengthInSamples % stepSize > 0_z) ? 1_z : 0_z);
                                          std::vector<Track::Results::Points> points;
                                          points.resize(static_cast<size_t>(reader.numChannels));
                                          for(auto& pointChannel : points)
                                          {
                                              pointChannel.resize(outputSize);
                                          }

                                          auto const numSamples = static_cast<size_t>(expectedBlock / stepSize) * stepSize;
                                          juce::AudioBuffer<float> buffer(static_cast<int>(reader.numChannels), static_cast<int>(numSamples));
                                          mAdvancement.store(0.1f);

                                          juce::int64 index = 0;
                                          while(mAnalysisState.load() != ProcessState::aborted && index < reader.lengthInSamples)
                                          {
                                              auto const remainingSize = reader.lengthInSamples - index;
                                              auto const blockSize = std::min(remainingSize, static_cast<juce::int64>(buffer.getNumSamples()));
                                              if(!reader.read(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), index, static_cast<int>(blockSize)))
                                              {
                                                  mAnalysisState.compare_exchange_weak(expected, ProcessState::aborted);
                                                  triggerAsyncUpdate();
                                                  return {};
                                              }

                                              if(stepSize == 1_z)
                                              {
                                                  for(auto channel = 0; channel < buffer.getNumChannels(); ++channel)
                                                  {
                                                      auto& pointChannel = points[static_cast<size_t>(channel)];
                                                      auto const* bufferChannel = buffer.getReadPointer(channel);
                                                      auto position = static_cast<size_t>(index);
                                                      for(auto sample = 0_z; sample < static_cast<size_t>(blockSize); ++sample)
                                                      {
                                                          MiscWeakAssert(position < pointChannel.size());
                                                          auto& point = pointChannel[position];
                                                          std::get<0_z>(point) = static_cast<double>(position) * timeRatio;
                                                          std::get<1_z>(point) = 0.0;
                                                          std::get<2_z>(point) = *bufferChannel++;
                                                          ++position;
                                                      }
                                                  }
                                              }
                                              else
                                              {
                                                  for(auto channel = 0; channel < buffer.getNumChannels(); ++channel)
                                                  {
                                                      auto& pointChannel = points[static_cast<size_t>(channel)];
                                                      auto const* bufferChannel = buffer.getReadPointer(channel);

                                                      auto sample = 0_z;
                                                      auto outputPosition = static_cast<size_t>(index) / stepSize;
                                                      auto position = static_cast<size_t>(index);
                                                      while(sample < static_cast<size_t>(blockSize))
                                                      {
                                                          auto const maxStep = std::min(static_cast<size_t>(blockSize) - sample, stepSize);
                                                          auto const it = std::max_element(bufferChannel, bufferChannel + maxStep, [](auto const& lhs, auto const& rhs)
                                                                                           {
                                                                                               return std::abs(lhs) < std::abs(rhs);
                                                                                           });

                                                          MiscWeakAssert(outputPosition < pointChannel.size());
                                                          auto& point = pointChannel[outputPosition];
                                                          std::get<0_z>(point) = static_cast<double>(position) / static_cast<double>(reader.lengthInSamples) * duration;
                                                          std::get<1_z>(point) = 0.0;
                                                          std::get<2_z>(point) = *it;

                                                          bufferChannel = std::next(bufferChannel, static_cast<long>(maxStep));
                                                          sample += stepSize;
                                                          position += stepSize;
                                                          ++outputPosition;
                                                      }
                                                  }
                                              }

                                              mAdvancement.store(static_cast<float>(index) / static_cast<float>(reader.lengthInSamples) * 0.9f + 0.1f);
                                              index += blockSize;
                                          }

                                          mAdvancement.store(1.0f);
                                          if(mAnalysisState.compare_exchange_weak(expected, ProcessState::ended))
                                          {
                                              triggerAsyncUpdate();
                                              return Track::Results(std::move(points));
                                          }

                                          triggerAsyncUpdate();
                                          return {};
                                      });
        return PluginList::Scanner::loadDescription(key, reader.sampleRate);
    }

    auto state = accessor.getAttr<AttrType::state>();
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

    auto description = processor->getDescription();
    mChrono.start();
    mAnalysisProcess = std::async([this, processor = std::move(processor), input = std::move(input)]() -> Results
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
                                      auto const inputs = Tools::convert(processor->getInput(), input);
                                      if(!processor->setPrecomputingResults(inputs))
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

                                      auto const results = Tools::convert(processor->getOutput(), pluginResults, mShouldAbort);
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
    return description;
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
