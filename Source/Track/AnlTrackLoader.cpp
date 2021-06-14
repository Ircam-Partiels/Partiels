#include "AnlTrackLoader.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Loader::~Loader()
{
    std::unique_lock<std::mutex> lock(mLoadingMutex);
    abortLoading();
}

juce::Result Track::Loader::loadAnalysis(Accessor const& accessor, juce::File const& file)
{
    auto const name = accessor.getAttr<AttrType::name>();

    std::unique_lock<std::mutex> lock(mLoadingMutex, std::try_to_lock);
    anlStrongAssert(lock.owns_lock());
    if(!lock.owns_lock())
    {
        return juce::Result::fail(juce::translate("The track ANLNAME cannot import analysis results from JSON file FLNAME due to concurrency access.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }
    abortLoading();

    nlohmann::json json;
    std::ifstream stream(file.getFullPathName().toStdString());
    if(!stream.is_open() || !stream.good())
    {
        return juce::Result::fail(juce::translate("The track ANLNAME cannot import analysis results from the JSON file FLNAME because the input stream cannot be opened.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }
    stream >> json;
    stream.close();

    if(json.empty())
    {
        return juce::Result::fail(juce::translate("The track ANLNAME cannot import analysis results from the JSON file FLNAME because the file doesn't contain datas.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }

    mChrono.start();
    mLoadingProcess = std::async([this, file = file, json = std::move(json)]() -> Results
                                 {
                                     juce::Thread::setCurrentThreadName("Track::Loader::Process");
                                     juce::Thread::setCurrentThreadPriority(10);

                                     auto expected = ProcessState::available;
                                     if(!mLoadingState.compare_exchange_weak(expected, ProcessState::running))
                                     {
                                         triggerAsyncUpdate();
                                         return {};
                                     }
                                     expected = ProcessState::running;

                                     std::vector<std::vector<Plugin::Result>> pluginResults;
                                     pluginResults.resize(json.size());
                                     for(size_t channelIndex = 0_z; channelIndex < json.size(); ++channelIndex)
                                     {
                                         auto const& channelData = json[channelIndex];
                                         auto& channelResults = pluginResults[channelIndex];
                                         channelResults.resize(channelData.size());
                                         for(size_t frameIndex = 0_z; frameIndex < channelData.size(); ++frameIndex)
                                         {
                                             if(mLoadingState.load() == ProcessState::aborted)
                                             {
                                                 triggerAsyncUpdate();
                                                 return {};
                                             }

                                             auto const& frameData = channelData[frameIndex];
                                             auto& frameResults = channelResults[frameIndex];
                                             auto const timeIt = frameData.find("time");
                                             if(timeIt != frameData.cend())
                                             {
                                                 frameResults.hasTimestamp = true;
                                                 frameResults.timestamp = Vamp::RealTime::fromSeconds(timeIt.value());
                                             }
                                             auto const durationIt = frameData.find("duration");
                                             if(durationIt != frameData.cend())
                                             {
                                                 frameResults.hasDuration = true;
                                                 frameResults.duration = Vamp::RealTime::fromSeconds(durationIt.value());
                                             }
                                             auto const labelIt = frameData.find("label");
                                             if(labelIt != frameData.cend())
                                             {
                                                 frameResults.label = labelIt.value();
                                             }
                                             auto const valueIt = frameData.find("value");
                                             if(valueIt != frameData.cend())
                                             {
                                                 frameResults.values.push_back(valueIt.value());
                                             }
                                             auto const valuesIt = frameData.find("values");
                                             if(valuesIt != frameData.cend())
                                             {
                                                 frameResults.values.resize(valuesIt->size());
                                                 for(size_t binIndex = 0_z; binIndex < valuesIt->size(); ++binIndex)
                                                 {
                                                     frameResults.values[binIndex] = valuesIt->at(binIndex);
                                                 }
                                             }
                                         }
                                     }

                                     if(mLoadingState.load() == ProcessState::aborted)
                                     {
                                         triggerAsyncUpdate();
                                         return {};
                                     }

                                     Plugin::Output output;
                                     output.hasFixedBinCount = false;
                                     auto results = Tools::getResults(output, pluginResults);
        results.file = file;
                                     if(mLoadingState.compare_exchange_weak(expected, ProcessState::ended))
                                     {
                                         triggerAsyncUpdate();
                                         return results;
                                     }

                                     triggerAsyncUpdate();
                                     return {};
                                 });

    return juce::Result::ok();
}

void Track::Loader::handleAsyncUpdate()
{
    std::unique_lock<std::mutex> lock(mLoadingMutex);
    if(mLoadingProcess.valid())
    {
        anlWeakAssert(mLoadingState != ProcessState::available);
        anlWeakAssert(mLoadingState != ProcessState::running);

        auto const results = mLoadingProcess.get();
        auto expected = ProcessState::ended;
        if(mLoadingState.compare_exchange_weak(expected, ProcessState::available))
        {
            mChrono.stop();
            if(onLoadingEnded != nullptr)
            {
                onLoadingEnded(results);
            }
        }
        else if(expected == ProcessState::aborted)
        {
            if(onLoadingAborted != nullptr)
            {
                onLoadingAborted();
            }
        }
        abortLoading();
    }
}

bool Track::Loader::isRunning() const
{
    return mLoadingProcess.valid();
}

void Track::Loader::abortLoading()
{
    if(mLoadingProcess.valid())
    {
        mLoadingState = ProcessState::aborted;
        mLoadingProcess.get();
        cancelPendingUpdate();

        if(onLoadingAborted != nullptr)
        {
            onLoadingAborted();
        }
    }
    mLoadingState = ProcessState::available;
}

ANALYSE_FILE_END
