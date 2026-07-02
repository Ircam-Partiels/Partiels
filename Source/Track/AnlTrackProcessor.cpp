#include "AnlTrackProcessor.h"
#include "../Plugin/AnlPluginListScanner.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Processor::~Processor()
{
    std::unique_lock<std::mutex> lock(mAnalysisMutex);
    abortAnalysis(lock);
}

void Track::Processor::stopAnalysis()
{
    std::unique_lock<std::mutex> lock(mAnalysisMutex);
    abortAnalysis(lock);
}

bool Track::Processor::runAnalysis(Accessor const& accessor, juce::AudioFormatReader& reader, InputStates inputStates)
{
    std::unique_lock<std::mutex> lock(mAnalysisMutex, std::try_to_lock);
    MiscWeakAssert(lock.owns_lock());
    if(!lock.owns_lock())
    {
        MiscError("Track", "Concurrent thread access!");
        return false;
    }
    abortAnalysis(lock);

    auto const key = accessor.getAttr<AttrType::key>();
    if(key.identifier.empty() || key.feature.empty())
    {
        return false;
    }
    // This is a specific case that speed up the waveform analysis
    if(key.identifier == "partiels-vamp-plugins:partielswaveform" && key.feature == "peaks")
    {
        mChrono.start();
        auto description = PluginList::Scanner::loadDescription(key, reader.sampleRate);
        mAnalysisProcess = std::async(std::launch::async, [this, desc = std::move(description), &reader]()
                                      {
                                          MiscDebug("Track", "Processor thread launched");
                                          juce::Thread::setCurrentThreadName("Track::Processor::Process");
                                          auto result = runWaveformAnalysis(reader, [this](float advancement)
                                                                            {
                                                                                mAdvancement.store(advancement);
                                                                                return !mShouldAbort.load();
                                                                            });
                                          auto fresult = std::make_tuple(std::move(std::get<0>(result)), std::move(std::get<1>(result)), std::move(desc));
                                          if(!mShouldAbort.load())
                                          {
                                              triggerAsyncUpdate();
                                          }
                                          return fresult;
                                      });
        return true;
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
    MiscWeakAssert(processor != nullptr);
    if(processor == nullptr)
    {
        MiscError("Track", "Processor allocation failed!");
        return false;
    }

    mChrono.start();
    mAnalysisProcess = std::async(std::launch::async, [this, proc = std::move(processor), inputs = std::move(inputStates)]()
                                  {
                                      MiscDebug("Track", "Processor thread launched");
                                      juce::Thread::setCurrentThreadName("Track::Processor::Process");
                                      auto result = runPluginAnalysis(*proc, inputs, [&, this](float advancement)
                                                                      {
                                                                          mAdvancement.store(advancement);
                                                                          return !mShouldAbort.load();
                                                                      });
                                      auto fresult = std::make_tuple(std::move(std::get<0>(result)), std::move(std::get<1>(result)), proc->getDescription());
                                      if(!mShouldAbort.load())
                                      {
                                          triggerAsyncUpdate();
                                      }
                                      return fresult;
                                  });
    return true;
}

void Track::Processor::handleAsyncUpdate()
{
    std::unique_lock<std::mutex> lock(mAnalysisMutex);
    if(mAnalysisProcess.valid())
    {
        auto state = mAnalysisProcess.wait_for(std::chrono::milliseconds(0));
        while(state != std::future_status::ready)
        {
            state = mAnalysisProcess.wait_for(std::chrono::milliseconds(5));
        }

        auto const results = mAnalysisProcess.get();
        if(onAnalysisEnded != nullptr)
        {
            mChrono.stop("Processor analysis ended");
            lock.unlock();
            onAnalysisEnded(std::get<0>(results), std::get<1>(results), std::get<2>(results));
            lock.lock();
        }
        mAdvancement.store(0.0f);
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

void Track::Processor::abortAnalysis(std::unique_lock<std::mutex>& lock)
{
    if(!lock.owns_lock())
    {
        MiscError("Track", "The mutex should already be locked!");
        return;
    }

    mAdvancement.store(0.0f);
    if(mAnalysisProcess.valid())
    {
        mShouldAbort.store(true);
        auto state = mAnalysisProcess.wait_for(std::chrono::milliseconds(0));
        while(state != std::future_status::ready)
        {
            state = mAnalysisProcess.wait_for(std::chrono::milliseconds(5));
        }
        [[maybe_unused]] auto result = mAnalysisProcess.get();
        cancelPendingUpdate();
        mShouldAbort.store(false);
        if(onAnalysisAborted != nullptr)
        {
            lock.unlock();
            onAnalysisAborted();
            lock.lock();
        }
    }
}

Track::Processor::ProcessResult Track::Processor::runWaveformAnalysis(juce::AudioFormatReader& reader, std::function<bool(float)> callback)
{
    if(callback != nullptr && !callback(0.0f))
    {
        return createEmptyProcessResult(createError(juce::translate("Aborted")));
    }

    if(reader.lengthInSamples == 0)
    {
        return std::make_tuple(juce::Result::ok(), Track::Results{});
    }

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
    if(callback != nullptr && !callback(0.1f))
    {
        return createEmptyProcessResult(createError(juce::translate("Aborted")));
    }

    juce::int64 index = 0;
    while(index < reader.lengthInSamples)
    {
        auto const adv = static_cast<float>(index) / static_cast<float>(reader.lengthInSamples) * 0.9f + 0.1f;
        if(callback != nullptr && !callback(adv))
        {
            return createEmptyProcessResult(createError(juce::translate("Aborted")));
        }

        auto const remainingSize = reader.lengthInSamples - index;
        auto const blockSize = std::min(remainingSize, static_cast<juce::int64>(buffer.getNumSamples()));
        if(!reader.read(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), index, static_cast<int>(blockSize)))
        {

            return createEmptyProcessResult(createError(juce::translate("Failed to read audio data")));
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
        index += blockSize;
    }

    if(callback != nullptr && !callback(1.0f))
    {
        return createEmptyProcessResult(createError(juce::translate("Aborted")));
    }
    return std::make_tuple(juce::Result::ok(), Track::Results(std::move(points)));
}

Track::Processor::ProcessResult Track::Processor::runPluginAnalysis(Plugin::Processor& processor, InputStates const& inputStates, std::function<bool(float)> callback)
{
    auto const tryProcess = [&](std::function<juce::Result(void)> fn) -> juce::Result
    {
        try
        {
            return fn();
        }
        catch(std::exception const& e)
        {
            return createError(e.what());
        }
        catch(...)
        {
            return createError(juce::translate("Unknown"));
        }
    };

    if(callback != nullptr && !callback(0.0f))
    {
        return createEmptyProcessResult(createError(juce::translate("Aborted")));
    }

    MiscDebug("Track::Processor", "Preparing analysis...");
    std::vector<std::vector<Plugin::Result>> results;
    auto result = tryProcess([&]()
                             {
                                 return processor.prepareToAnalyze(results);
                             });
    if(result.failed())
    {
        return createEmptyProcessResult(std::move(result));
    }

    MiscDebug("Track::Processor", "Setting inputs...");
    std::vector<std::vector<std::vector<Plugin::Result>>> inputData;
    auto const inputs = processor.getInputs();
    for(auto const& inputState : inputStates)
    {
        auto const it = std::find_if(inputs.cbegin(), inputs.cend(), [&](auto const& input)
                                     {
                                         return input.identifier == inputState.first;
                                     });
        if(it != inputs.cend())
        {
            auto const featureIndex = static_cast<size_t>(std::distance(inputs.cbegin(), it));
            auto allData = Tools::convert(*it, inputState.second.first, inputState.second.second);
            for(auto channelIndex = 0_z; channelIndex < allData.size(); ++channelIndex)
            {
                if(channelIndex >= inputData.size())
                {
                    inputData.resize(channelIndex + 1_z);
                }
                if(featureIndex >= inputData.at(channelIndex).size())
                {
                    inputData[channelIndex].resize(featureIndex + 1_z);
                }
                inputData[channelIndex][featureIndex] = std::move(allData[channelIndex]);
            }
        }
    }

    result = tryProcess([&]()
                        {
                            return processor.setPrecomputingResults(inputData);
                        });
    if(result.failed())
    {
        return createEmptyProcessResult(std::move(result));
    }
    MiscDebug("Track::Processor", "Performing analysis...");

    auto performResult = processor.performNextAudioBlock(results);
    while(std::get<1>(performResult))
    {
        if(callback != nullptr && !callback(processor.getAdvancement()))
        {
            return std::make_tuple(juce::Result::fail(juce::translate("Aborted")), Track::Results{});
        }
        performResult = processor.performNextAudioBlock(results);
    }
    if(std::get<0>(performResult).failed())
    {
        return createEmptyProcessResult(std::get<0>(performResult));
    }

    MiscDebug("Track::Processor", "Converting results...");
    auto processed = false;
    auto const cresults = Tools::convert(processor.getOutput(), results, [&]()
                                         {
                                             processed = callback == nullptr || callback(1.0f);
                                             return processed;
                                         });
    if(!processed)
    {
        return createEmptyProcessResult(createError(juce::translate("Aborted")));
    }
    MiscDebug("Track::Processor", "Analysis completed");
    return std::make_tuple(juce::Result::ok(), std::move(cresults));
}

Track::Processor::ProcessResult Track::Processor::createEmptyProcessResult(juce::Result result)
{
    return std::make_tuple(std::move(result), Track::Results{});
}

juce::Result Track::Processor::createError(juce::String const& reason)
{
    return juce::Result::fail(juce::translate("Analysis failed with error: CERROR").replace("CERROR", reason));
}

ANALYSE_FILE_END
