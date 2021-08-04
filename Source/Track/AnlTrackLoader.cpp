#include "AnlTrackLoader.h"
#include "AnlTrackTools.h"
#include <TestResultsData.h>

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
                                     Track::Results results;
                                     try
                                     {
                                         results = file.hasFileExtension("json") ? loadFromJson(stream, mLoadingState, mAdvancement) : loadFromBinary(stream, mLoadingState, mAdvancement);
                                     }
                                     catch(...)
                                     {
                                         mLoadingState.store(ProcessState::aborted);
                                     }
                                     triggerAsyncUpdate();
                                     return results;
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

Track::Results Track::Loader::loadFromJson(std::istream& stream, std::atomic<ProcessState>& loadingState, std::atomic<float>& advancement)
{
    auto expected = ProcessState::available;
    if(!loadingState.compare_exchange_weak(expected, ProcessState::running))
    {
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
    ThreadedStreamIterator const itStart(stream, loadingState);
    ThreadedStreamIterator const itEnd(loadingState);
    auto const json = nlohmann::json::parse(itStart, itEnd, nullptr, false);

    advancement.store(0.2f);
    if(json.is_discarded())
    {
        loadingState.store(ProcessState::aborted);
        return {};
    }

    if(loadingState.load() == ProcessState::aborted)
    {
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
        advancement.store(0.2f + advRatio);
        for(size_t frameIndex = 0_z; frameIndex < channelData.size(); ++frameIndex)
        {
            advancement.store(0.2f + advRatio + 0.1f * static_cast<float>(frameIndex) / static_cast<float>(channelData.size()));
            if(loadingState.load() == ProcessState::aborted)
            {
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

    if(loadingState.load() == ProcessState::aborted)
    {
        return {};
    }

    Plugin::Output output;
    output.hasFixedBinCount = false;
    advancement.store(0.9f);
    auto results = Tools::getResults(output, pluginResults);
    advancement.store(1.0f);
    if(loadingState.compare_exchange_weak(expected, ProcessState::ended))
    {
        return results;
    }

    return {};
}

Track::Results Track::Loader::loadFromBinary(std::istream& stream, std::atomic<ProcessState>& loadingState, std::atomic<float>& advancement)
{
    auto expected = ProcessState::available;
    if(!loadingState.compare_exchange_weak(expected, ProcessState::running))
    {
        return {};
    }
    expected = ProcessState::running;
    
    Results res;
    char type[7] = {'\0'};
    if(!stream.read(type, sizeof(char) * 6))
    {
        loadingState.store(ProcessState::aborted);
        return {};
    }

    if(stream.eof())
    {
        loadingState.store(ProcessState::aborted);
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
                loadingState.store(ProcessState::aborted);
                return {};
            }
            markers.resize(static_cast<size_t>(numChannels));

            for(auto& marker : markers)
            {
                if(loadingState.load() == ProcessState::aborted)
                {
                    return {};
                }

                if(!stream.read(reinterpret_cast<char*>(&std::get<0>(marker)), sizeof(std::get<0>(marker))))
                {
                    loadingState.store(ProcessState::aborted);
                    return {};
                }
                if(!stream.read(reinterpret_cast<char*>(&std::get<1>(marker)), sizeof(std::get<1>(marker))))
                {
                    loadingState.store(ProcessState::aborted);
                    return {};
                }
                uint64_t length;
                if(!stream.read(reinterpret_cast<char*>(&length), sizeof(length)))
                {
                    loadingState.store(ProcessState::aborted);
                    return {};
                }
                std::get<2>(marker).resize(length);
                if(!stream.read(reinterpret_cast<char*>(std::get<2>(marker).data()), static_cast<long>(length * sizeof(char))))
                {
                    loadingState.store(ProcessState::aborted);
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
                loadingState.store(ProcessState::aborted);
                return {};
            }
            points.resize(static_cast<size_t>(numChannels));
            for(auto& point : points)
            {
                if(loadingState.load() == ProcessState::aborted)
                {
                    return {};
                }

                if(!stream.read(reinterpret_cast<char*>(&std::get<0>(point)), sizeof(std::get<0>(point))))
                {
                    loadingState.store(ProcessState::aborted);
                    return {};
                }
                if(!stream.read(reinterpret_cast<char*>(&std::get<1>(point)), sizeof(std::get<1>(point))))
                {
                    loadingState.store(ProcessState::aborted);
                    return {};
                }
                bool hasValue;
                if(!stream.read(reinterpret_cast<char*>(&hasValue), sizeof(hasValue)))
                {
                    loadingState.store(ProcessState::aborted);
                    return {};
                }
                if(hasValue)
                {
                    float value;
                    if(!stream.read(reinterpret_cast<char*>(&value), sizeof(value)))
                    {
                        loadingState.store(ProcessState::aborted);
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
                loadingState.store(ProcessState::aborted);
                return {};
            }
            columns.resize(static_cast<size_t>(numChannels));
            for(auto& column : columns)
            {
                if(loadingState.load() == ProcessState::aborted)
                {
                    return {};
                }

                if(!stream.read(reinterpret_cast<char*>(&std::get<0>(column)), sizeof(std::get<0>(column))))
                {
                    loadingState.store(ProcessState::aborted);
                    return {};
                }
                if(!stream.read(reinterpret_cast<char*>(&std::get<1>(column)), sizeof(std::get<1>(column))))
                {
                    loadingState.store(ProcessState::aborted);
                    return {};
                }
                uint64_t numBins;
                if(!stream.read(reinterpret_cast<char*>(&numBins), sizeof(numBins)))
                {
                    loadingState.store(ProcessState::aborted);
                    return {};
                }
                std::get<2>(column).resize(numBins);
                if(!stream.read(reinterpret_cast<char*>(std::get<2>(column).data()), static_cast<long>(sizeof(*std::get<2>(column).data()) * numBins)))
                {
                    loadingState.store(ProcessState::aborted);
                    return {};
                }
            }
            results.push_back(std::move(columns));
        }

        auto const numBins = Tools::getNumBins(results);
        auto const valueRange = Tools::getValueRange(results);
        res = Results(std::make_shared<const std::vector<Results::Columns>>(std::move(results)), numBins, valueRange);
    }

    advancement.store(1.0f);
    if(loadingState.compare_exchange_weak(expected, ProcessState::ended))
    {
        return res;
    }
    return {};
}

Track::Results Track::Loader::loadFromCsv(std::istream& stream, std::atomic<ProcessState>& loadingState, std::atomic<float>& advancement)
{
    auto expected = ProcessState::available;
    if(!loadingState.compare_exchange_weak(expected, ProcessState::running))
    {
        return {};
    }
    expected = ProcessState::running;

    std::vector<std::vector<Plugin::Result>> pluginResults;

    auto trimString = [](std::string const& s)
    {
        auto ltrim = [](std::string const& sr)
        {
            auto const start = sr.find_first_not_of(" \n\r\t\f\v");
            return (start == std::string::npos) ? "" : sr.substr(start);
        };

        auto rtrim = [](std::string const& sr)
        {
            auto const end = sr.find_last_not_of(" \n\r\t\f\v");
            return (end == std::string::npos) ? "" : sr.substr(0, end + 1);
        };

        return rtrim(ltrim(s));
    };

    auto unescapeString = [](juce::String const& s)
    {
        return s.replace("\\\"", "\"").replace("\\\'", "\'").replace("\\t", "\t").replace("\\r", "\r").replace("\\n", "\n").unquoted();
    };

    auto addNewChannel = true;
    std::string line;
    while(getline(stream, line, '\n'))
    {
        line = trimString(line);
        if(line.empty() || line == "\n")
        {
            addNewChannel = true;
        }
        else
        {
            if(std::exchange(addNewChannel, false))
            {
                pluginResults.push_back({});
            }
            std::string time, duration, value;
            std::istringstream linestream(line);
            getline(linestream, time, ',');
            getline(linestream, duration, ',');
            getline(linestream, value, ',');
            if(!time.empty() && !duration.empty() && std::isdigit(static_cast<int>(time[0])) && std::isdigit(static_cast<int>(duration[0])))
            {
                auto& channelResults = pluginResults.back();
                Plugin::Result result;
                result.hasTimestamp = true;
                result.timestamp = Vamp::RealTime::fromSeconds(std::stod(time));
                result.hasDuration = true;
                result.duration = Vamp::RealTime::fromSeconds(std::stod(duration));
                if(!value.empty() && !std::isdigit(static_cast<int>(value[0])))
                {
                    result.label = unescapeString(value).toStdString();
                }
                else if(!value.empty())
                {
                    result.values.push_back(std::stof(value));
                    while(getline(linestream, value, ','))
                    {
                        result.values.push_back(std::stof(value));
                    }
                }
                channelResults.push_back(std::move(result));
            }
        }
    }

    Plugin::Output output;
    output.hasFixedBinCount = false;
    advancement.store(0.9f);
    auto results = Tools::getResults(output, pluginResults);
    advancement.store(1.0f);
    if(loadingState.compare_exchange_weak(expected, ProcessState::ended))
    {
        return results;
    }
    return {};
}

class Track::Loader::UnitTest
: public juce::UnitTest
{
public:
    UnitTest()
    : juce::UnitTest("Track", "Loader")
    {
    }

    ~UnitTest() override = default;

    void runTest() override
    {
        auto checkMakers = [this](Results results)
        {
            auto const markers = results.getMarkers();
            expect(markers != nullptr);
            if(markers == nullptr)
            {
                return;
            }
            expectEquals(markers->size(), 2_z);
            if(markers->size() != 2_z)
            {
                return;
            }

            using namespace std::string_literals;
            auto expectMarker = [this](Results::Marker const& marker, double time, std::string const& label)
            {
                expectWithinAbsoluteError(std::get<0>(marker), time, 1e-9);
                expectWithinAbsoluteError(std::get<1>(marker), 0.0, 1e-9);
                expectEquals(std::get<2>(marker), label);
            };

            auto expectChannel = [&](Results::Markers const& channelMarkers, std::vector<std::tuple<double, std::string>> const& expectedResults)
            {
                expectEquals(channelMarkers.size(), expectedResults.size());
                if(channelMarkers.size() != expectedResults.size())
                {
                    return;
                }

                for(auto index = 0_z; index < expectedResults.size(); ++index)
                {
                    expectMarker(channelMarkers[index], std::get<0>(expectedResults[index]), std::get<1>(expectedResults[index]));
                }
            };

            expectChannel(markers->at(0_z), {{0.023219955, "N"s}, {0.023582767, "B7/D#"s}, {0.026122449, "A"s}});
            expectChannel(markers->at(1_z), {{0.023219955, "Z"s}, {0.023582767, "A"s}});
        };

        auto checkPoints = [this](Results results)
        {
            auto const points = results.getPoints();
            expect(points != nullptr);
            if(points == nullptr)
            {
                return;
            }
            expectEquals(points->size(), 2_z);
            if(points->size() != 2_z)
            {
                return;
            }

            using namespace std::string_literals;
            auto expectPoint = [this](Results::Point const& point, double time, float value)
            {
                expectWithinAbsoluteError(std::get<0>(point), time, 1e-9);
                expectWithinAbsoluteError(std::get<1>(point), 0.0, 1e-9);
                expect(std::get<2>(point).has_value());
                if(std::get<2>(point).has_value())
                {
                    expectWithinAbsoluteError(*std::get<2>(point), value, 1e-9f);
                }
            };

            auto expectChannel = [&](Results::Points const& channelPoints, std::vector<std::tuple<double, float>> const& expectedResults)
            {
                expectEquals(channelPoints.size(), expectedResults.size());
                if(channelPoints.size() != expectedResults.size())
                {
                    return;
                }

                for(auto index = 0_z; index < expectedResults.size(); ++index)
                {
                    expectPoint(channelPoints[index], std::get<0>(expectedResults[index]), std::get<1>(expectedResults[index]));
                }
            };
            expectChannel(points->at(0_z), {{0.0, 2361.65478515625f}, {0.023219955, 3899.171630859375f}, {0.046439909, 3270.79541015625f}, {0.069659864, 2604.403564453125f}});
            expectChannel(points->at(1_z), {{0.0, 2503.146484375f}, {0.023219955, 3616.829833984375f}, {0.046439909, 3045.656005859375f}});
        };

        beginTest("JSON Markers");
        {
            auto const result = std::string(TestResultsData::Markers_json);
            std::istringstream stream(result);
            std::atomic<ProcessState> loadingState;
            loadingState.store(ProcessState::available);
            std::atomic<float> advancement;
            advancement.store(0.0f);
            checkMakers(loadFromJson(stream, loadingState, advancement));
        }

        beginTest("JSON Points");
        {
            auto const result = std::string(TestResultsData::Points_json);
            std::istringstream stream(result);
            std::atomic<ProcessState> loadingState;
            loadingState.store(ProcessState::available);
            std::atomic<float> advancement;
            advancement.store(0.0f);
            checkPoints(loadFromJson(stream, loadingState, advancement));
        }

        beginTest("Binary Markers");
        {
            std::stringstream stream;
            stream.write(TestResultsData::Markers_dat, TestResultsData::Markers_datSize);
            std::atomic<ProcessState> loadingState;
            loadingState.store(ProcessState::available);
            std::atomic<float> advancement;
            advancement.store(0.0f);
            checkMakers(loadFromBinary(stream, loadingState, advancement));
        }

        beginTest("Binary Points");
        {
            std::stringstream stream;
            stream.write(TestResultsData::Points_dat, TestResultsData::Points_datSize);
            std::atomic<ProcessState> loadingState;
            loadingState.store(ProcessState::available);
            std::atomic<float> advancement;
            advancement.store(0.0f);
            checkPoints(loadFromBinary(stream, loadingState, advancement));
        }
    }
};

static Track::Loader::UnitTest trackLoaderUnitTest;

ANALYSE_FILE_END
