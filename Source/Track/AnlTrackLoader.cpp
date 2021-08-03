#include "AnlTrackLoader.h"
#include "AnlTrackTools.h"

#if JUCE_GCC
#define ANL_ATTR_UNUSED __attribute__((unused))
#else
#define ANL_ATTR_UNUSED
#endif

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
        return juce::Result::fail(juce::translate("The track ANLNAME cannot import analysis results from file FLNAME due to concurrency access.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }
    abortLoading();

    auto openStream = [&]() -> std::ifstream
    {
        auto const path = file.getFullPathName().toStdString();
        if(file.hasFileExtension("dat"))
        {
            return std::ifstream(path, std::ios::in | std::ios::binary);
        }
        else if(file.hasFileExtension("json"))
        {
            return std::ifstream(path);
        }
        return std::ifstream();
    };

    auto stream = openStream();
    if(!stream || !stream.is_open() || !stream.good())
    {
        return juce::Result::fail(juce::translate("The track ANLNAME cannot import analysis results from the file FLNAME because the input stream cannot be opened.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }

    mAdvancement.store(0.0f);
    mChrono.start();

    mLoadingProcess = std::async([this, file = file, stream = std::move(stream)]() mutable -> Results
                                 {
                                     juce::Thread::setCurrentThreadName("Track::Loader::Process");
                                     juce::Thread::setCurrentThreadPriority(10);
                                     if(file.hasFileExtension("json"))
                                     {
                                         return loadFromJson(std::move(stream));
                                     }
                                     return loadFromBinary(std::move(stream));
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

float Track::Loader::getAdvancement() const
{
    return mAdvancement.load();
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
    mAdvancement.store(0.0f);
    mLoadingState = ProcessState::available;
}

Track::Results Track::Loader::loadFromJson(std::ifstream stream)
{
    auto expected = ProcessState::available;
    if(!mLoadingState.compare_exchange_weak(expected, ProcessState::running))
    {
        triggerAsyncUpdate();
        stream.close();
        return {};
    }

    class ThreadedStreamIterator
    {
    public:
        typedef char value_type ANL_ATTR_UNUSED;
        typedef ptrdiff_t difference_type ANL_ATTR_UNUSED;
        typedef const char* pointer ANL_ATTR_UNUSED;
        typedef const char& reference ANL_ATTR_UNUSED;
        typedef std::input_iterator_tag iterator_category ANL_ATTR_UNUSED;

        ThreadedStreamIterator(std::atomic<ProcessState>& s)
        : state(s)
        {
        }

        ThreadedStreamIterator(std::istream& i, std::atomic<ProcessState>& s)
        : iterator(i)
        , state(s)
        {
        }

        ThreadedStreamIterator(ThreadedStreamIterator const& x)
        : iterator(x.iterator)
        , state(x.state)
        {
        }

        ~ThreadedStreamIterator() = default;

        inline const char& operator*() const
        {
            return iterator.operator*();
        }

        inline const char* operator->() const
        {
            return iterator.operator->();
        }

        inline ThreadedStreamIterator& operator++()
        {
            iterator.operator++();
            return *this;
        }

        inline ThreadedStreamIterator& operator++(int v)
        {
            iterator.operator++(v);
            return *this;
        }

        inline bool operator==(ThreadedStreamIterator const& rhs) const
        {
            return state.load() == ProcessState::aborted ? true : iterator == rhs.iterator;
        }

        inline bool operator!=(ThreadedStreamIterator const& rhs) const
        {
            return state.load() == ProcessState::aborted ? false : iterator != rhs.iterator;
        }

    private:
        std::istream_iterator<char, char, std::char_traits<char>, ptrdiff_t> iterator;
        std::atomic<ProcessState> const& state;
    };

    expected = ProcessState::running;
    ThreadedStreamIterator const itStart(stream, mLoadingState);
    ThreadedStreamIterator const itEnd(mLoadingState);
    auto const json = nlohmann::json::parse(itStart, itEnd, nullptr, false);
    stream.close();
    mAdvancement.store(0.2f);
    if(json.is_discarded())
    {
        mLoadingState.store(ProcessState::aborted);
        triggerAsyncUpdate();
        return {};
    }

    if(mLoadingState.load() == ProcessState::aborted)
    {
        triggerAsyncUpdate();
        return {};
    }
    std::vector<std::vector<Plugin::Result>> pluginResults;
    pluginResults.resize(json.size());
    for(size_t channelIndex = 0_z; channelIndex < json.size(); ++channelIndex)
    {
        auto const& channelData = json[channelIndex];
        auto& channelResults = pluginResults[channelIndex];
        channelResults.resize(channelData.size());
        auto const advRatio = 0.7f * static_cast<float>(channelIndex) / static_cast<float>(json.size());
        mAdvancement.store(0.2f + advRatio);
        for(size_t frameIndex = 0_z; frameIndex < channelData.size(); ++frameIndex)
        {
            mAdvancement.store(0.2f + advRatio + 0.1f * static_cast<float>(frameIndex) / static_cast<float>(channelData.size()));
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
    mAdvancement.store(0.9f);
    auto results = Tools::getResults(output, pluginResults);
    mAdvancement.store(1.0f);
    if(mLoadingState.compare_exchange_weak(expected, ProcessState::ended))
    {
        triggerAsyncUpdate();
        return results;
    }

    triggerAsyncUpdate();
    return {};
}

Track::Results Track::Loader::loadFromBinary(std::ifstream stream)
{
    auto expected = ProcessState::available;
    if(!mLoadingState.compare_exchange_weak(expected, ProcessState::running))
    {
        triggerAsyncUpdate();
        stream.close();
        return {};
    }
    expected = ProcessState::running;

    Results res;
    char type[7] = {'\0'};
    if(!stream.read(type, sizeof(char) * 6))
    {
        mLoadingState.store(ProcessState::aborted);
        triggerAsyncUpdate();
        return {};
    }

    if(std::string(type) == "PTLMKS")
    {
        std::vector<Results::Markers> results;
        while(!stream.eof())
        {
            Results::Markers markers;
            uint64_t numChannels;
            if(!stream.read(reinterpret_cast<char*>(&numChannels), sizeof(numChannels)))
            {
                if(stream.eof())
                {
                    break;
                }
                mLoadingState.store(ProcessState::aborted);
                triggerAsyncUpdate();
                return {};
            }
            markers.resize(static_cast<size_t>(numChannels));
            for(auto& marker : markers)
            {
                if(mLoadingState.load() == ProcessState::aborted)
                {
                    triggerAsyncUpdate();
                    return {};
                }

                if(!stream.read(reinterpret_cast<char*>(&std::get<0>(marker)), sizeof(std::get<0>(marker))))
                {
                    mLoadingState.store(ProcessState::aborted);
                    triggerAsyncUpdate();
                    return {};
                }
                if(!stream.read(reinterpret_cast<char*>(&std::get<1>(marker)), sizeof(std::get<1>(marker))))
                {
                    mLoadingState.store(ProcessState::aborted);
                    triggerAsyncUpdate();
                    return {};
                }
                uint64_t length;
                if(!stream.read(reinterpret_cast<char*>(&length), sizeof(length)))
                {
                    mLoadingState.store(ProcessState::aborted);
                    triggerAsyncUpdate();
                    return {};
                }
                std::get<2>(marker).resize(length);
                if(!stream.read(reinterpret_cast<char*>(std::get<2>(marker).data()), static_cast<long>(sizeof(length) * sizeof(char))))
                {
                    mLoadingState.store(ProcessState::aborted);
                    triggerAsyncUpdate();
                    return {};
                }
            }
            results.push_back(std::move(markers));
        }

        res = Results(std::make_shared<const std::vector<Results::Markers>>(std::move(results)));
    }
    else if(std::string(type) == "PTLPTS")
    {
        std::vector<Results::Points> results;
        while(!stream.eof())
        {
            Results::Points points;
            uint64_t numChannels;
            if(!stream.read(reinterpret_cast<char*>(&numChannels), sizeof(numChannels)))
            {
                if(stream.eof())
                {
                    break;
                }
                mLoadingState.store(ProcessState::aborted);
                triggerAsyncUpdate();
                return {};
            }
            points.resize(static_cast<size_t>(numChannels));
            for(auto& point : points)
            {
                if(mLoadingState.load() == ProcessState::aborted)
                {
                    triggerAsyncUpdate();
                    return {};
                }

                if(!stream.read(reinterpret_cast<char*>(&std::get<0>(point)), sizeof(std::get<0>(point))))
                {
                    mLoadingState.store(ProcessState::aborted);
                    triggerAsyncUpdate();
                    return {};
                }
                if(!stream.read(reinterpret_cast<char*>(&std::get<1>(point)), sizeof(std::get<1>(point))))
                {
                    mLoadingState.store(ProcessState::aborted);
                    triggerAsyncUpdate();
                    return {};
                }
                bool hasValue;
                if(!stream.read(reinterpret_cast<char*>(&hasValue), sizeof(hasValue)))
                {
                    mLoadingState.store(ProcessState::aborted);
                    triggerAsyncUpdate();
                    return {};
                }
                if(hasValue)
                {
                    float value;
                    if(!stream.read(reinterpret_cast<char*>(&value), sizeof(value)))
                    {
                        mLoadingState.store(ProcessState::aborted);
                        triggerAsyncUpdate();
                        return {};
                    }
                    std::get<2>(point) = value;
                }
            }
            results.push_back(std::move(points));
        }

        auto const valueRange = Tools::getValueRange(results);
        res = Results(std::make_shared<const std::vector<Results::Points>>(std::move(results)), valueRange);
    }
    else if(std::string(type) == "PTLCLS")
    {
        std::vector<Results::Columns> results;
        while(!stream.eof())
        {
            Results::Columns columns;
            uint64_t numChannels;
            if(!stream.read(reinterpret_cast<char*>(&numChannels), sizeof(numChannels)))
            {
                if(stream.eof())
                {
                    break;
                }
                mLoadingState.store(ProcessState::aborted);
                triggerAsyncUpdate();
                return {};
            }
            columns.resize(static_cast<size_t>(numChannels));
            for(auto& column : columns)
            {
                if(mLoadingState.load() == ProcessState::aborted)
                {
                    triggerAsyncUpdate();
                    return {};
                }

                if(!stream.read(reinterpret_cast<char*>(&std::get<0>(column)), sizeof(std::get<0>(column))))
                {
                    mLoadingState.store(ProcessState::aborted);
                    triggerAsyncUpdate();
                    return {};
                }
                if(!stream.read(reinterpret_cast<char*>(&std::get<1>(column)), sizeof(std::get<1>(column))))
                {
                    mLoadingState.store(ProcessState::aborted);
                    triggerAsyncUpdate();
                    return {};
                }
                uint64_t numBins;
                if(!stream.read(reinterpret_cast<char*>(&numBins), sizeof(numBins)))
                {
                    mLoadingState.store(ProcessState::aborted);
                    triggerAsyncUpdate();
                    return {};
                }
                std::get<2>(column).resize(numBins);
                if(!stream.read(reinterpret_cast<char*>(std::get<2>(column).data()), static_cast<long>(sizeof(*std::get<2>(column).data()) * numBins)))
                {
                    mLoadingState.store(ProcessState::aborted);
                    triggerAsyncUpdate();
                    return {};
                }
            }
            results.push_back(std::move(columns));
        }

        auto const numBins = Tools::getNumBins(results);
        auto const valueRange = Tools::getValueRange(results);
        res = Results(std::make_shared<const std::vector<Results::Columns>>(std::move(results)), numBins, valueRange);
    }

    if(mLoadingState.compare_exchange_weak(expected, ProcessState::ended))
    {
        triggerAsyncUpdate();
        return res;
    }

    triggerAsyncUpdate();
    return {};
}

ANALYSE_FILE_END
