#include "AnlTrackProcessor.h"
#include "../Plugin/AnlPluginListScanner.h"
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

bool Track::Processor::runAnalysis(Accessor const& accessor, juce::AudioFormatReader& reader, Results input)
{
    std::unique_lock<std::mutex> lock(mAnalysisMutex, std::try_to_lock);
    MiscWeakAssert(lock.owns_lock());
    if(!lock.owns_lock())
    {
        MiscError("Track", "Concurrent thread access!");
        return false;
    }
    abortAnalysis();

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
                                          triggerAsyncUpdate();
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
    mAnalysisProcess = std::async(std::launch::async, [this, proc = std::move(processor), in = std::move(input)]()
                                  {
                                      MiscDebug("Track", "Processor thread launched");
                                      juce::Thread::setCurrentThreadName("Track::Processor::Process");
                                      auto result = runPluginAnalysis(*proc, in, [&, this](float advancement)
                                                                      {
                                                                          mAdvancement.store(advancement);
                                                                          return !mShouldAbort.load();
                                                                      });
                                      auto fresult = std::make_tuple(std::move(std::get<0>(result)), std::move(std::get<1>(result)), proc->getDescription());
                                      triggerAsyncUpdate();
                                      return fresult;
                                  });
    return true;
}

void Track::Processor::handleAsyncUpdate()
{
    std::unique_lock<std::mutex> lock(mAnalysisMutex);
    if(mAnalysisProcess.valid())
    {
        auto const state = mAnalysisProcess.wait_for(std::chrono::milliseconds(0));
        MiscWeakAssert(state == std::future_status::ready);
        if(state != std::future_status::ready)
        {
            triggerAsyncUpdate();
            return;
        }

        auto const results = mAnalysisProcess.get();
        if(onAnalysisEnded != nullptr)
        {
            mChrono.stop("Processor analysis ended");
            onAnalysisEnded(std::get<0>(results), std::get<1>(results), std::get<2>(results));
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
    std::unique_lock<std::mutex> lock(mAnalysisMutex, std::try_to_lock);
    MiscWeakAssert(!lock.owns_lock());
    if(lock.owns_lock())
    {
        MiscError("Track", "The mutex should already be locked!");
        return;
    }
    mAdvancement.store(0.0f);
    if(mAnalysisProcess.valid())
    {
        mShouldAbort.store(true);
        mAnalysisProcess.get();
        cancelPendingUpdate();
        if(onAnalysisAborted != nullptr)
        {
            onAnalysisAborted();
        }
    }
    mShouldAbort.store(false);
}

Track::Processor::ProcessResult Track::Processor::runWaveformAnalysis(juce::AudioFormatReader& reader, std::function<bool(float)> callback)
{
    if(callback != nullptr && !callback(0.0f))
    {
        return std::make_tuple(juce::Result::fail(juce::translate("Analysis aborted")), Track::Results{});
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
        return std::make_tuple(juce::Result::fail(juce::translate("Analysis aborted")), Track::Results{});
    }

    juce::int64 index = 0;
    while(index < reader.lengthInSamples)
    {
        auto const adv = static_cast<float>(index) / static_cast<float>(reader.lengthInSamples) * 0.9f + 0.1f;
        if(callback != nullptr && !callback(adv))
        {
            return std::make_tuple(juce::Result::fail(juce::translate("Analysis aborted")), Track::Results{});
        }

        auto const remainingSize = reader.lengthInSamples - index;
        auto const blockSize = std::min(remainingSize, static_cast<juce::int64>(buffer.getNumSamples()));
        if(!reader.read(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), index, static_cast<int>(blockSize)))
        {

            return std::make_tuple(juce::Result::fail(juce::translate("Analysis failed to read audio data")), Track::Results{});
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
        return std::make_tuple(juce::Result::fail(juce::translate("Analysis aborted")), Track::Results{});
    }
    return std::make_tuple(juce::Result::ok(), Track::Results(std::move(points)));
}

Track::Processor::ProcessResult Track::Processor::runPluginAnalysis(Plugin::Processor& processor, Results const& input, std::function<bool(float)> callback)
{
    if(callback != nullptr && !callback(0.0f))
    {
        return std::make_tuple(juce::Result::fail(juce::translate("Analysis aborted")), Track::Results{});
    }

    std::vector<std::vector<Plugin::Result>> results;
    auto prepareResult = processor.prepareToAnalyze(results);
    if(prepareResult.failed())
    {
        return std::make_tuple(prepareResult, Track::Results{});
    }
    auto const inputs = Tools::convert(processor.getInput(), input);
    auto precomputingResults = processor.setPrecomputingResults(inputs);
    if(precomputingResults.failed())
    {
        return std::make_tuple(precomputingResults, Track::Results{});
    }
    MiscDebug("Track", "Processor prepared");

    auto performResult = processor.performNextAudioBlock(results);
    while(std::get<1>(performResult))
    {
        if(callback != nullptr && !callback(processor.getAdvancement()))
        {
            return std::make_tuple(juce::Result::fail(juce::translate("Analysis aborted")), Track::Results{});
        }
        performResult = processor.performNextAudioBlock(results);
    }
    if(std::get<0>(performResult).failed())
    {
        return std::make_tuple(std::get<0>(performResult), Track::Results{});
    }

    MiscDebug("Track", "Processor performed");

    auto processed = false;
    auto const cresults = Tools::convert(processor.getOutput(), results, [&]()
                                         {
                                             processed = callback == nullptr || callback(1.0f);
                                             return processed;
                                         });
    if(!processed)
    {
        return std::make_tuple(juce::Result::fail(juce::translate("Analysis aborted")), Track::Results{});
    }
    MiscDebug("Track", "Results performed");
    return std::make_tuple(juce::Result::ok(), std::move(cresults));
}

ANALYSE_FILE_END
