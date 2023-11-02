#include "AnlTrackLoader.h"
#include "AnlTrackTools.h"
#include <TestResultsData.h>

#if JUCE_GCC
#define ANL_ATTR_UNUSED __attribute__((unused))
#else
#define ANL_ATTR_UNUSED
#endif

ANALYSE_FILE_BEGIN

namespace
{
    template <class T>
    class StopableStreamIterator
    {
    public:
        typedef char value_type ANL_ATTR_UNUSED;
        typedef ptrdiff_t difference_type ANL_ATTR_UNUSED;
        typedef const char* pointer ANL_ATTR_UNUSED;
        typedef const char& reference ANL_ATTR_UNUSED;
        typedef std::input_iterator_tag iterator_category ANL_ATTR_UNUSED;

        StopableStreamIterator(T const& s)
        : shouldAbort(s)
        {
        }

        StopableStreamIterator(std::istream& i, T const& s)
        : iterator(i)
        , shouldAbort(s)
        {
        }

        StopableStreamIterator(StopableStreamIterator const& x)
        : iterator(x.iterator)
        , shouldAbort(x.shouldAbort)
        {
        }

        ~StopableStreamIterator() = default;

        inline const char& operator*() const
        {
            return iterator.operator*();
        }

        inline const char* operator->() const
        {
            return iterator.operator->();
        }

        inline StopableStreamIterator& operator++()
        {
            iterator.operator++();
            return *this;
        }

        inline StopableStreamIterator& operator++(int v)
        {
            iterator.operator++(v);
            return *this;
        }

        inline bool operator==(StopableStreamIterator const& rhs) const
        {
            return shouldAbort ? true : iterator == rhs.iterator;
        }

        inline bool operator!=(StopableStreamIterator const& rhs) const
        {
            return shouldAbort ? false : iterator != rhs.iterator;
        }

    private:
        std::istream_iterator<char, char, std::char_traits<char>, ptrdiff_t> iterator;
        T const& shouldAbort;
    };
} // namespace

Track::Loader::~Loader()
{
    std::unique_lock<std::mutex> lock(mLoadingMutex);
    abortLoading();
}

void Track::Loader::loadAnalysis(FileInfo const& fileInfo)
{
    std::unique_lock<std::mutex> lock(mLoadingMutex, std::try_to_lock);
    anlStrongAssert(lock.owns_lock());
    if(!lock.owns_lock())
    {
        if(onLoadingFailed != nullptr)
        {
            onLoadingFailed("Invalid threaded access.");
        }
        return;
    }
    abortLoading();

    mChrono.start();

    mShouldAbort = false;
    mAdvancement.store(0.0f);
    mLoadingProcess = std::async([this, fileInfo = fileInfo]() mutable -> std::variant<Results, juce::String>
                                 {
                                     if(mShouldAbort)
                                     {
                                         return {};
                                     }
                                     juce::Thread::setCurrentThreadName("Track::Loader::Process");
                                     auto results = loadFromFile(fileInfo, mShouldAbort, mAdvancement);
                                     triggerAsyncUpdate();
                                     return results;
                                 });
}

void Track::Loader::handleAsyncUpdate()
{
    std::unique_lock<std::mutex> lock(mLoadingMutex);
    if(mLoadingProcess.valid())
    {
        if(mShouldAbort.exchange(false))
        {
#ifdef DEBUG
            auto const vResults = mLoadingProcess.get();
            auto* results = std::get_if<Results>(&vResults);
            anlWeakAssert(results != nullptr && results->isEmpty());
#endif
            mChrono.stop();
            if(onLoadingAborted != nullptr)
            {
                onLoadingAborted();
            }
        }
        else
        {
            auto const vResults = mLoadingProcess.get();
            mChrono.stop();
            if(auto* results = std::get_if<Results>(&vResults))
            {
                if(onLoadingSucceeded != nullptr)
                {
                    onLoadingSucceeded(*results);
                }
            }
            else if(auto* message = std::get_if<juce::String>(&vResults))
            {
                if(onLoadingFailed != nullptr)
                {
                    onLoadingFailed(*message);
                }
            }
            else
            {
                anlWeakAssert(false && "invalid state");
            }
        }
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
        anlWeakAssert(mShouldAbort.load() == false);
        mShouldAbort.store(true);
        mLoadingProcess.get();
        cancelPendingUpdate();

        if(onLoadingAborted != nullptr)
        {
            onLoadingAborted();
        }
    }
    mAdvancement.store(0.0f);
    mShouldAbort.store(false);
}

std::variant<Track::Results, juce::String> Track::Loader::loadFromFile(FileInfo const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement)
{
    try
    {
        if(fileInfo.file.hasFileExtension("dat"))
        {
            return loadFromBinary(fileInfo, shouldAbort, advancement);
        }
        if(fileInfo.file.hasFileExtension("json"))
        {
            return loadFromJson(fileInfo, shouldAbort, advancement);
        }
        else if(fileInfo.file.hasFileExtension("csv") || fileInfo.file.hasFileExtension("lab"))
        {
            return loadFromCsv(fileInfo, shouldAbort, advancement);
        }
        else if(fileInfo.file.hasFileExtension("cue"))
        {
            return loadFromCue(fileInfo, shouldAbort, advancement);
        }
        else if(fileInfo.file.hasFileExtension("sdif"))
        {
            return loadFromSdif(fileInfo, shouldAbort, advancement);
        }
    }
    catch(std::exception& e)
    {
        return juce::String(e.what());
    }
    catch(...)
    {
        return juce::String("Parsing error");
    }
    return {juce::translate("The file format is not supported")};
}

std::variant<Track::Results, juce::String> Track::Loader::loadFromJson(FileInfo const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement)
{
    auto stream = std::ifstream(fileInfo.file.getFullPathName().toStdString());
    if(!stream || !stream.is_open() || !stream.good())
    {
        return {juce::translate("The input stream of cannot be opened")};
    }
    return loadFromJson(stream, shouldAbort, advancement);
}

std::variant<Track::Results, juce::String> Track::Loader::loadFromJson(std::istream& stream, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement)
{
    StopableStreamIterator<std::atomic<bool>> const itStart(stream, shouldAbort);
    StopableStreamIterator<std::atomic<bool>> const itEnd(shouldAbort);
    nlohmann::basic_json container;
    try
    {
        container = nlohmann::json::parse(itStart, itEnd, nullptr);
    }
    catch(nlohmann::json::parse_error& e)
    {
        return {juce::translate(e.what())};
    }

    advancement.store(0.2f);
    if(container.is_discarded())
    {
        return {juce::translate("Parsing error")};
    }

    auto const& json = container.count("results") ? container.at("results") : container;
    if(shouldAbort)
    {
        return {};
    }

    std::vector<Results::Markers> markers;
    std::vector<Results::Points> points;
    std::vector<Results::Columns> columns;
    markers.reserve(json.size());
    points.reserve(json.size());
    columns.reserve(json.size());

    // clang-format off
    enum class Mode
    {
          undefined
        , markers
        , points
        , columns
    };
    // clang-format on

    auto mode = Mode::undefined;
    auto const check = [&](Mode const expected)
    {
        return mode == Mode::undefined || mode == expected;
    };

    auto const checkAndSet = [&](Mode const expected)
    {
        if(mode == Mode::undefined || mode == expected)
        {
            mode = expected;
            return true;
        }
        return false;
    };

    for(size_t channelIndex = 0_z; channelIndex < json.size(); ++channelIndex)
    {
        auto const& channelData = json.at(channelIndex);
        Results::Markers markerChannel;
        Results::Points pointChannel;
        Results::Columns columnChannel;
        markerChannel.reserve(check(Mode::markers) ? channelData.size() : 0_z);
        pointChannel.reserve(check(Mode::points) ? channelData.size() : 0_z);
        columnChannel.reserve(check(Mode::columns) ? channelData.size() : 0_z);

        auto const advRatio = 0.8f * static_cast<float>(channelIndex) / static_cast<float>(json.size());
        advancement.store(0.2f + advRatio);
        for(size_t frameIndex = 0_z; frameIndex < channelData.size(); ++frameIndex)
        {
            advancement.store(0.2f + advRatio + 0.1f * static_cast<float>(frameIndex) / static_cast<float>(channelData.size()));
            if(shouldAbort)
            {
                return {};
            }

            auto const& frameData = channelData.at(frameIndex);
            auto const timeIt = frameData.find("time");
            if(timeIt == frameData.cend())
            {
                return {juce::translate("Parsing error")};
            }
            auto const durationIt = frameData.find("duration");
            auto const extraIt = frameData.find("extra");
            auto const labelIt = frameData.find("label");
            if(labelIt != frameData.cend())
            {
                if(!checkAndSet(Mode::markers))
                {
                    return {juce::translate("Parsing error")};
                }
                markerChannel.push_back({});
                std::get<0_z>(markerChannel.back()) = timeIt->get<double>();
                std::get<1_z>(markerChannel.back()) = durationIt == frameData.cend() ? 0.0 : durationIt->get<double>();
                std::get<2_z>(markerChannel.back()) = labelIt->get<std::string>();
                if(extraIt != frameData.cend())
                {
                    std::get<3_z>(markerChannel.back()) = extraIt->get<std::vector<float>>();
                }
            }
            auto const valueIt = frameData.find("value");
            if(valueIt != frameData.cend())
            {
                if(!checkAndSet(Mode::points))
                {
                    return {juce::translate("Parsing error")};
                }
                pointChannel.push_back({});
                std::get<0_z>(pointChannel.back()) = timeIt->get<double>();
                std::get<1_z>(pointChannel.back()) = durationIt == frameData.cend() ? 0.0 : durationIt->get<double>();
                std::get<2_z>(pointChannel.back()) = valueIt->get<float>();
                if(extraIt != frameData.cend())
                {
                    std::get<3_z>(pointChannel.back()) = extraIt->get<std::vector<float>>();
                }
            }
            auto const valuesIt = frameData.find("values");
            if(valuesIt != frameData.cend())
            {
                if(!checkAndSet(Mode::columns))
                {
                    return {juce::translate("Parsing error")};
                }
                columnChannel.push_back({});
                std::get<0_z>(columnChannel.back()) = timeIt->get<double>();
                std::get<1_z>(columnChannel.back()) = durationIt == frameData.cend() ? 0.0 : durationIt->get<double>();
                std::get<2_z>(columnChannel.back()) = valuesIt->get<std::vector<float>>();
                if(extraIt != frameData.cend())
                {
                    std::get<3_z>(columnChannel.back()) = extraIt->get<std::vector<float>>();
                }
            }
        }
        if(check(Mode::markers))
        {
            markers.push_back(std::move(markerChannel));
        }
        else if(check(Mode::points))
        {
            points.push_back(std::move(pointChannel));
        }
        else if(check(Mode::columns))
        {
            columns.push_back(std::move(columnChannel));
        }
    }
    if(shouldAbort)
    {
        return {};
    }

    advancement.store(1.0f);
    if(check(Mode::markers))
    {
        return Track::Results(std::move(markers));
    }
    else if(check(Mode::points))
    {
        return Track::Results(std::move(points));
    }
    else if(check(Mode::columns))
    {
        return Track::Results(std::move(columns));
    }
    return {juce::translate("Parsing error")};
}

std::variant<Track::Results, juce::String> Track::Loader::loadFromBinary(FileInfo const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement)
{
    auto stream = std::ifstream(fileInfo.file.getFullPathName().toStdString(), std::ios::in | std::ios::binary);
    if(!stream || !stream.is_open() || !stream.good())
    {
        return {juce::translate("The input stream of cannot be opened")};
    }
    return loadFromBinary(stream, shouldAbort, advancement);
}

std::variant<Track::Results, juce::String> Track::Loader::loadFromBinary(std::istream& stream, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement)
{
    if(shouldAbort)
    {
        return {};
    }

    Results res;
    char type[7] = {'\0'};
    if(!stream.read(type, sizeof(char) * 6))
    {
        return {juce::translate("Parsing error")};
    }

    if(stream.eof())
    {
        return {juce::translate("Parsing error")};
    }

    auto const readTimeAndDuration = [&](auto& frame)
    {
        if(!stream.read(reinterpret_cast<char*>(&std::get<0_z>(frame)), sizeof(std::get<0_z>(frame))))
        {
            return false;
        }
        if(!stream.read(reinterpret_cast<char*>(&std::get<1_z>(frame)), sizeof(std::get<1_z>(frame))))
        {
            return false;
        }
        return true;
    };

    auto const readExtra = [&](auto& frame)
    {
        uint64_t numExtra;
        if(!stream.read(reinterpret_cast<char*>(&numExtra), sizeof(numExtra)))
        {
            return false;
        }
        std::get<3_z>(frame).resize(numExtra);
        if(!stream.read(reinterpret_cast<char*>(std::get<3_z>(frame).data()), static_cast<long>(sizeof(*std::get<3_z>(frame).data()) * numExtra)))
        {
            return false;
        }
        return true;
    };

    if(std::string(type) == "PTLMKS" || std::string(type) == "PTLM01")
    {
        auto const hasExtra = std::string(type) == "PTLM01";
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
                return {juce::translate("Parsing error")};
            }
            markers.resize(static_cast<size_t>(numChannels));

            for(auto& marker : markers)
            {
                if(shouldAbort)
                {
                    return {};
                }

                if(!readTimeAndDuration(marker))
                {
                    return {juce::translate("Parsing error")};
                }
                uint64_t length;
                if(!stream.read(reinterpret_cast<char*>(&length), sizeof(length)))
                {
                    return {juce::translate("Parsing error")};
                }
                std::get<2_z>(marker).resize(length);
                if(!stream.read(reinterpret_cast<char*>(std::get<2_z>(marker).data()), static_cast<long>(length * sizeof(char))))
                {
                    return {juce::translate("Parsing error")};
                }
                if(hasExtra)
                {
                    readExtra(marker);
                }
            }
            results.push_back(std::move(markers));
        }

        res = Results(std::move(results));
    }
    else if(std::string(type) == "PTLPTS" || std::string(type) == "PTLP01")
    {
        auto const hasExtra = std::string(type) == "PTLP01";
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
                return {juce::translate("Parsing error")};
            }
            points.resize(static_cast<size_t>(numChannels));
            for(auto& point : points)
            {
                if(shouldAbort)
                {
                    return {};
                }

                if(!readTimeAndDuration(point))
                {
                    return {juce::translate("Parsing error")};
                }
                bool hasValue;
                if(!stream.read(reinterpret_cast<char*>(&hasValue), sizeof(hasValue)))
                {
                    return {juce::translate("Parsing error")};
                }
                if(hasValue)
                {
                    float value;
                    if(!stream.read(reinterpret_cast<char*>(&value), sizeof(value)))
                    {
                        return {juce::translate("Parsing error")};
                    }
                    std::get<2_z>(point) = value;
                }
                if(hasExtra)
                {
                    readExtra(point);
                }
            }
            results.push_back(std::move(points));
        }

        res = Results(std::move(results));
    }
    else if(std::string(type) == "PTLCLS" || std::string(type) == "PTLC01")
    {
        auto const hasExtra = std::string(type) == "PTLC01";
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
                return {juce::translate("Parsing error")};
            }
            columns.resize(static_cast<size_t>(numChannels));
            for(auto& column : columns)
            {
                if(shouldAbort)
                {
                    return {};
                }

                if(!readTimeAndDuration(column))
                {
                    return {juce::translate("Parsing error")};
                }
                uint64_t numBins;
                if(!stream.read(reinterpret_cast<char*>(&numBins), sizeof(numBins)))
                {
                    return {juce::translate("Parsing error")};
                }
                std::get<2_z>(column).resize(numBins);
                if(!stream.read(reinterpret_cast<char*>(std::get<2_z>(column).data()), static_cast<long>(sizeof(*std::get<2>(column).data()) * numBins)))
                {
                    return {juce::translate("Parsing error")};
                }
                if(hasExtra)
                {
                    readExtra(column);
                }
            }
            results.push_back(std::move(columns));
        }
        res = Results(std::move(results));
    }
    else
    {
        return {juce::translate("Parsing error")};
    }

    advancement.store(1.0f);
    return {std::move(res)};
}

std::variant<Track::Results, juce::String> Track::Loader::loadFromCue(FileInfo const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement)
{
    auto stream = std::ifstream(fileInfo.file.getFullPathName().toStdString());
    if(!stream || !stream.is_open() || !stream.good())
    {
        return {juce::translate("The input stream of cannot be opened")};
    }
    return loadFromCue(stream, shouldAbort, advancement);
}

std::variant<Track::Results, juce::String> Track::Loader::loadFromCue(std::istream& stream, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement)
{
    using namespace std::string_literals;
    std::vector<Track::Results::Markers> channelResults;

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

    auto unescapeString = [](std::string const& s)
    {
        return juce::String(s).replace("\\\"", "\"").replace("\\\'", "\'").replace("\\t", "\t").replace("\\r", "\r").replace("\\n", "\n").unquoted().toStdString();
    };

    auto hasKey = [](std::string const& line, std::string const& key, size_t indentation)
    {
        auto generateKey = [&](std::string const& strIndentation, bool upperCase)
        {
            std::string fkey;
            while(indentation > 0_z)
            {
                fkey += strIndentation;
                --indentation;
            }
            fkey += key;
            if(upperCase)
            {
                std::transform(fkey.begin(), fkey.end(), fkey.begin(), [](auto c)
                               {
                                   return std::toupper(c);
                               });
            }
            else
            {
                std::transform(fkey.begin(), fkey.end(), fkey.begin(), [](auto c)
                               {
                                   return std::tolower(c);
                               });
            }
            return fkey;
        };
        return line.find(generateKey("  ", true)) == 0_z || line.find(generateKey("  ", false)) == 0_z || line.find(generateKey("\t", true)) == 0_z || line.find(generateKey("\t", false)) == 0_z;
    };

    auto addNewChannel = true;
    std::string line;
    while(getline(stream, line, '\n'))
    {
        if(shouldAbort)
        {
            return {};
        }

        if(hasKey(line, "TITLE"s, 0_z))
        {
            addNewChannel = true;
        }
        else if(hasKey(line, "TRACK"s, 1_z))
        {
            if(std::exchange(addNewChannel, false))
            {
                channelResults.push_back({});
            }
            if(channelResults.empty())
            {
                return {juce::translate("Parsing error")};
            }
            channelResults.back().push_back({});
        }
        else if(hasKey(line, "TITLE"s, 2_z))
        {
            if(channelResults.empty() || channelResults.back().empty())
            {
                return {juce::translate("Parsing error")};
            }
            auto& frameResult = channelResults.back().back();
            auto const label = unescapeString(trimString(trimString(line).substr("TITLE"s.length())));
            std::get<2>(frameResult) = label;
        }
        else if(hasKey(line, "INDEX 01"s, 2_z))
        {
            if(channelResults.empty() || channelResults.back().empty())
            {
                return {juce::translate("Parsing error")};
            }
            auto& frameResult = channelResults.back().back();
            auto const timeString = trimString(trimString(line).substr("INDEX 01"s.length()));
            std::string minutes, secondes, frames;
            std::istringstream timeStream(timeString);
            getline(timeStream, minutes, ':');
            getline(timeStream, secondes, ':');
            getline(timeStream, frames, ':');
            auto const time = std::stod(minutes) * 60.0 + std::stod(secondes) + std::stod(frames) / 75.0;
            std::get<0>(frameResult) = time;
        }
    }

    advancement.store(1.0f);
    return Results(std::move(channelResults));
}

std::variant<Track::Results, juce::String> Track::Loader::loadFromCsv(FileInfo const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement)
{
    auto stream = std::ifstream(fileInfo.file.getFullPathName().toStdString());
    if(!stream || !stream.is_open() || !stream.good())
    {
        return {juce::translate("The input stream of cannot be opened")};
    }
    auto const separator = fileInfo.args.getValue("separator", ",").toStdString();
    auto const useEndTime = fileInfo.args.getValue("useendtime", "false") == "true";
    return loadFromCsv(stream, separator.empty() ? ',' : separator.at(0_z), useEndTime, shouldAbort, advancement);
}

std::variant<Track::Results, juce::String> Track::Loader::loadFromCsv(std::istream& stream, char const separator, bool useEndTime, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement)
{
    auto const trimString = [](std::string const& s)
    {
        auto const ltrim = [](std::string const& sr)
        {
            auto const start = sr.find_first_not_of(" \n\r\t\f\v");
            return (start == std::string::npos) ? "" : sr.substr(start);
        };

        auto const rtrim = [](std::string const& sr)
        {
            auto const end = sr.find_last_not_of(" \n\r\t\f\v");
            return (end == std::string::npos) ? "" : sr.substr(0, end + 1);
        };

        return rtrim(ltrim(s));
    };

    auto const unescapeString = [](juce::String const& s)
    {
        return s.replace("\\\"", "\"").replace("\\\'", "\'").replace("\\t", "\t").replace("\\r", "\r").replace("\\n", "\n").unquoted();
    };

    // clang-format off
    enum class Mode
    {
          undefined
        , markers
        , points
    };
    // clang-format on

    auto mode = Mode::undefined;
    auto const check = [&](Mode const expected)
    {
        return mode == Mode::undefined || mode == expected;
    };

    auto const getFloatValue = [](auto const& v) -> std::optional<float>
    {
        try
        {
            auto value = std::stof(v);
            if(std::isnan(value) || !std::isfinite(value))
            {
                return {};
            }
            return value;
        }
        catch(...)
        {
            return {};
        }
    };

    std::vector<Results::Markers> markers;
    std::vector<Results::Points> points;

    auto addNewChannel = true;
    std::string line;
    while(std::getline(stream, line, '\n'))
    {
        if(shouldAbort)
        {
            return {};
        }

        line = trimString(line);
        if(line.empty() || line == "\n")
        {
            addNewChannel = true;
        }
        else
        {
            if(std::exchange(addNewChannel, false))
            {
                markers.push_back({});
                points.push_back({});
            }
            std::string time, duration, value;
            std::istringstream linestream(line);
            std::getline(linestream, time, separator);
            std::getline(linestream, duration, separator);
            std::getline(linestream, value, separator);
            if(time.empty() || duration.empty())
            {
                return {juce::translate("Parsing error")};
            }
            else if(std::isdigit(static_cast<int>(time[0])) && std::isdigit(static_cast<int>(duration[0])))
            {
                auto& markerChannel = markers.back();
                auto& pointChannel = points.back();
                auto const timevalue = std::stod(time);
                auto const durationvalue = std::max(useEndTime ? std::stod(duration) - timevalue : std::stod(duration), 0.0);
                auto pointvalue = mode == Mode::markers ? std::optional<float>{} : getFloatValue(value);
                if(pointvalue.has_value())
                {
                    mode = Mode::points;
                    pointChannel.push_back({});
                    std::get<0_z>(pointChannel.back()) = timevalue;
                    std::get<1_z>(pointChannel.back()) = durationvalue;
                    std::get<2_z>(pointChannel.back()) = pointvalue.value();
                    while(getline(linestream, value, separator))
                    {
                        pointvalue = getFloatValue(value);
                        if(pointvalue.has_value())
                        {
                            std::get<3_z>(pointChannel.back()).push_back(pointvalue.value());
                        }
                    }
                }
                else
                {
                    if(mode == Mode::points)
                    {
                        return {juce::translate("Parsing error")};
                    }
                    mode = Mode::markers;
                    markerChannel.push_back({});
                    std::get<0_z>(markerChannel.back()) = timevalue;
                    std::get<1_z>(markerChannel.back()) = durationvalue;
                    std::get<2_z>(markerChannel.back()) = unescapeString(value).toStdString();
                    while(getline(linestream, value, separator))
                    {
                        pointvalue = getFloatValue(value);
                        if(pointvalue.has_value())
                        {
                            std::get<3_z>(markerChannel.back()).push_back(pointvalue.value());
                        }
                    }
                }
            }
        }
    }

    advancement.store(1.0f);
    if(check(Mode::markers))
    {
        return Track::Results(std::move(markers));
    }
    else if(check(Mode::points))
    {
        return Track::Results(std::move(points));
    }
    return {juce::translate("Parsing error")};
}

std::variant<Track::Results, juce::String> Track::Loader::loadFromSdif(FileInfo const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement)
{
    if(!fileInfo.file.existsAsFile())
    {
        return {juce::translate("The input stream of cannot be opened")};
    }
    auto const& args = fileInfo.args;
    auto const frame = SdifConverter::getSignature(args.getValue("frame", ""));
    auto const matrix = SdifConverter::getSignature(args.getValue("matrix", ""));
    auto const row = args.containsKey("row") ? std::optional<size_t>(static_cast<size_t>(args.getValue("row", "").getIntValue())) : std::optional<size_t>();
    auto const column = args.containsKey("column") ? std::optional<size_t>(static_cast<size_t>(args.getValue("column", "").getIntValue())) : std::optional<size_t>();
    return loadFromSdif(fileInfo.file, frame, matrix, row, column, shouldAbort, advancement);
}

std::variant<Track::Results, juce::String> Track::Loader::loadFromSdif(juce::File const& file, uint32_t frameId, uint32_t matrixId, std::optional<size_t> row, std::optional<size_t> column, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement)
{
    advancement.store(0.0f);
    // clang-format off
    enum class BinMode
    {
          undefined
        , defined
        , variable
    };
    // clang-format on
    auto binMode = BinMode::undefined;
    auto binCount = 0_z;
    auto const setBinCount = [&](auto const numBins)
    {
        if(binMode == BinMode::undefined)
        {
            binMode = BinMode::defined;
            binCount = numBins;
        }
        else if(binMode == BinMode::defined && binCount != numBins)
        {
            binMode = BinMode::variable;
            binCount = numBins;
        }
    };

    juce::String message;
    std::vector<std::vector<Plugin::Result>> pluginResults;
    auto const readResult = SdifConverter::read(
        file, [&](uint32_t frameSignature, size_t frameIndex, double time, size_t numMarix)
        {
            juce::ignoreUnused(frameIndex, time, numMarix);
            return !shouldAbort && message.isEmpty() && frameSignature == frameId;
        },
        [&](uint32_t frameSignature, size_t frameIndex, uint32_t matrixSignature, size_t numRows, size_t numColumns, std::vector<std::string> names)
        {
            juce::ignoreUnused(frameSignature, frameIndex, numRows, numColumns, names);
            return !shouldAbort && message.isEmpty() && matrixSignature == matrixId;
        },
        [&](uint32_t frameSignature, size_t frameIndex, uint32_t matrixSignature, double time, size_t numRows, size_t numColumns, std::variant<std::vector<std::string>, std::vector<std::vector<double>>> data)
        {
            juce::ignoreUnused(frameSignature, matrixSignature);
            if(shouldAbort || !message.isEmpty())
            {
                return false;
            }
            if(data.index() == 0_z && column.has_value() && *column != 0_z)
            {
                message = "Column index cannot be specified with text data type";
                return false;
            }
            if(numRows != 1_z && !row.has_value() && numColumns != 1_z && !column.has_value())
            {
                message = "Can't convert all rows and all columns (either one row or one column must be selected)";
                return false;
            }

            pluginResults.resize(std::max(pluginResults.size(), frameIndex + 1_z));
            if(pluginResults.size() < frameIndex)
            {
                message = "Channels cannot be alocated";
                return false;
            }
            auto& channelResults = pluginResults[frameIndex];

            auto const* labels = std::get_if<0_z>(&data);
            auto const* values = std::get_if<1_z>(&data);

            Plugin::Result result;
            result.hasTimestamp = true;
            result.timestamp = Vamp::RealTime::fromSeconds(time);

            if(row.has_value() || numRows == 1_z)
            {
                auto const rowIndex = row.has_value() ? *row : 0_z;
                if(labels != nullptr && labels->size() > rowIndex)
                {
                    result.label = labels->at(rowIndex);
                    setBinCount(0_z);
                }
                else if(values != nullptr && values->size() > rowIndex)
                {
                    if(column.has_value() || numColumns == 1)
                    {
                        auto const columnIndex = column.has_value() ? *column : 0_z;
                        if(values->at(rowIndex).size() > columnIndex)
                        {
                            result.values = {static_cast<float>(values->at(rowIndex).at(columnIndex))};
                        }
                        setBinCount(1_z);
                    }
                    else
                    {
                        auto const& rowValues = values->at(rowIndex);
                        result.values.reserve(rowValues.size());
                        for(auto index = 0_z; index < rowValues.size(); ++index)
                        {
                            result.values.push_back(static_cast<float>(rowValues[index]));
                        }
                        setBinCount(rowValues.size());
                    }
                }
            }
            else if(values != nullptr)
            {
                auto const columnIndex = column.has_value() ? *column : 0_z;
                result.values.reserve(values->size());
                for(auto rowIndex = 0_z; rowIndex < values->size(); ++rowIndex)
                {
                    if(columnIndex < values->at(rowIndex).size())
                    {
                        result.values.push_back(static_cast<float>(values->at(rowIndex).at(columnIndex)));
                    }
                }
                setBinCount(result.values.size());
            }
            channelResults.push_back(std::move(result));
            advancement.store(std::min(advancement.load() + 0.001f, 1.0f));
            return true;
        });

    if(readResult.failed())
    {
        return {readResult.getErrorMessage()};
    }

    if(!message.isEmpty())
    {
        return {message};
    }

    Plugin::Output output;
    switch(static_cast<uint32_t>(matrixId))
    {
        case SdifConverter::SignatureIds::i1FQ0:
            output.hasFixedBinCount = true;
            output.binCount = 1_z;
            break;
        default:
            output.hasFixedBinCount = binMode == BinMode::defined;
            output.binCount = binCount;
            break;
    }
    advancement.store(0.9f);
    auto results = Tools::convert(output, pluginResults, shouldAbort);
    advancement.store(1.0f);
    return {std::move(results)};
}

juce::String Track::Loader::getWildCardForAllFormats()
{
    return "*.json;*.csv;*.lab;*.cue;*.sdif;*.dat";
}

Track::Loader::ArgumentSelector::ArgumentSelector()
: mPropertyName("File", "The file to import", nullptr)
, mPropertyColumnSeparator("Column Separator", "The seperatror character between colummns", "", std::vector<std::string>{"Comma", "Space", "Tab", "Pipe", "Slash", "Colon"}, nullptr)
, mLoadButton("Load", "Load the file with the arguments", nullptr)
{
    mPropertyName.entry.setEnabled(false);
    addAndMakeVisible(mPropertyName);
    addAndMakeVisible(mPropertyColumnSeparator);
    mPropertyColumnSeparator.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
    mSdifPanel.onUpdated = [this]()
    {
        auto const format = mSdifPanel.getFromSdifFormat();
        mLoadButton.setEnabled(mSdifPanel.isVisible() && std::get<0_z>(format) != uint32_t(0) && std::get<1_z>(format) != uint32_t(0));
    };
    addAndMakeVisible(mSdifPanel);
    mLoadButton.entry.addShortcut(juce::KeyPress(juce::KeyPress::returnKey));
    addAndMakeVisible(mLoadButton);
    setSize(300, 100);
}

void Track::Loader::ArgumentSelector::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto const setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyName);
    setBounds(mPropertyColumnSeparator);
    setBounds(mSdifPanel);
    setBounds(mLoadButton);
    setSize(getWidth(), bounds.getY() + 2);
}

bool Track::Loader::ArgumentSelector::setFile(juce::File const& file, double sampleRate, std::function<void(FileInfo)> callback)
{
    if(callback == nullptr)
    {
        return false;
    }

    juce::WildcardFileFilter wildcardFilter(getWildCardForAllFormats(), "*", "");
    if(!wildcardFilter.isFileSuitable(file))
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("Invalid file!"))
                                 .withMessage(juce::translate("The format of the file 'FLNAME' is not supported.").replace("FLNAME", file.getFullPathName()))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
        return false;
    }

    auto const streamFlag = file.hasFileExtension("dat") ? std::ios::in | std::ios::binary : std::ios::in;
    auto stream = std::ifstream(file.getFullPathName().toStdString(), streamFlag);
    if(!stream)
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("Invalid file!"))
                                 .withMessage(juce::translate("The input stream of the file 'FLNAME' cannot be opened.").replace("FLNAME", file.getFullPathName()))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
        return false;
    }

    mPropertyName.entry.setText(file.getFileName(), juce::NotificationType::dontSendNotification);

    if(file.hasFileExtension("sdif"))
    {
        mPropertyColumnSeparator.setVisible(false);
        mSdifPanel.setVisible(true);
        mSdifPanel.setFile(file);
        juce::WeakReference<juce::Component> weakReference(this);
        auto const doLoad = [=, this]()
        {
            if(weakReference.get() == nullptr)
            {
                return;
            }
            FileInfo fileInfo;
            fileInfo.file = file;
            auto const format = mSdifPanel.getFromSdifFormat();
            fileInfo.args.set("frame", SdifConverter::getString(std::get<0_z>(format)));
            fileInfo.args.set("matrix", SdifConverter::getString(std::get<1_z>(format)));
            if(std::get<2_z>(format).has_value())
            {
                fileInfo.args.set("row", juce::String(*std::get<2_z>(format)));
            }
            if(std::get<3_z>(format).has_value())
            {
                fileInfo.args.set("column", juce::String(*std::get<3_z>(format)));
            }
            fileInfo.extra = mSdifPanel.getExtraInfo(sampleRate);
            callback(fileInfo);
        };
        if(!mSdifPanel.hasAnyChangeableOption())
        {
            doLoad();
            return false;
        }
        mLoadButton.entry.onClick = doLoad;
        resized();
        return true;
    }

    if(file.hasFileExtension("csv"))
    {
        mPropertyColumnSeparator.setVisible(true);
        mSdifPanel.setVisible(false);
        mLoadButton.setEnabled(true);
        juce::WeakReference<juce::Component> weakReference(this);
        mLoadButton.entry.onClick = [=, this]()
        {
            if(weakReference.get() == nullptr)
            {
                return;
            }
            FileInfo fileInfo;
            fileInfo.file = file;
            static const std::vector<std::string> separators{",", " ", "\t", "|", "/", ":"};
            auto const index = static_cast<size_t>(std::max(mPropertyColumnSeparator.entry.getSelectedItemIndex(), 0));
            auto const separator = index < separators.size() ? separators.at(index) : separators.at(0_z);
            fileInfo.args.set("separator", juce::String(separator));
            fileInfo.args.set("useendtime", "false");
            callback(fileInfo);
        };
        resized();
        return true;
    }

    if(file.hasFileExtension("lab"))
    {
        FileInfo fileInfo;
        fileInfo.file = file;
        fileInfo.args.set("separator", "\t");
        fileInfo.args.set("useendtime", "true");
        callback(fileInfo);
        return false;
    }

    if(file.hasFileExtension("json"))
    {
        FileInfo fileInfo;
        fileInfo.file = file;
        auto json = nlohmann::sax_parse_json_object(stream, "track", 1_z);
        if(json.count("track") > 0_z)
        {
            json["track"].erase("identifier");
            json["track"].erase("file");
            fileInfo.extra = std::move(json["track"]);
        }
        callback(fileInfo);
        return false;
    }

    FileInfo fileInfo;
    fileInfo.file = file;
    callback(fileInfo);
    return false;
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
        auto checkMakers = [this](std::variant<Results, juce::String> vResult, double timeEpsilon = 1e-9)
        {
            expectEquals(vResult.index(), 0_z);
            if(vResult.index() == 1_z)
            {
                expectNotEquals(vResult.index(), 1_z, *std::get_if<juce::String>(&vResult));
                return;
            }
            auto const results = *std::get_if<Results>(&vResult);
            auto const access = results.getReadAccess();
            expect(static_cast<bool>(access));
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
            auto expectMarker = [this, timeEpsilon](Results::Marker const& marker, double time, std::string const& label)
            {
                expectWithinAbsoluteError(std::get<0>(marker), time, timeEpsilon);
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

            expectChannel(markers->at(0_z), {{0.023219955, "N"s}, {1.023582767, "B7/D#"s}, {78.026122449, "A"s}});
            expectChannel(markers->at(1_z), {{0.023219955, "Z"s}, {10.023582767, "A"s}});
        };

        auto checkPoints = [this](std::variant<Results, juce::String> vResult)
        {
            expectEquals(vResult.index(), 0_z);
            if(vResult.index() == 1_z)
            {
                expectNotEquals(vResult.index(), 1_z, *std::get_if<juce::String>(&vResult));
                return;
            }
            auto const results = *std::get_if<Results>(&vResult);
            auto const access = results.getReadAccess();
            expect(static_cast<bool>(access));
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

        beginTest("load cue error");
        {
            auto const result = std::string(TestResultsData::Error_cue);
            std::istringstream stream(result);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            expectEquals(loadFromCue(stream, shouldAbort, advancement).index(), 1_z);
        }

        beginTest("load cue markers");
        {
            auto const result = std::string(TestResultsData::Markers_cue);
            std::istringstream stream(result);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            checkMakers(loadFromCue(stream, shouldAbort, advancement), 1.0 / 75.0);
        }

        beginTest("load csv error");
        {
            auto const result = std::string(TestResultsData::Error_csv);
            std::istringstream stream(result);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            expectEquals(loadFromCsv(stream, ',', false, shouldAbort, advancement).index(), 1_z);
        }

        beginTest("load csv markers with comma separator");
        {
            auto const result = std::string(TestResultsData::Markers_csv);
            std::istringstream stream(result);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            checkMakers(loadFromCsv(stream, ',', false, shouldAbort, advancement));
        }

        beginTest("load csv markers with tab separator");
        {
            auto const result = std::string(TestResultsData::MarkersTab_csv);
            std::istringstream stream(result);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            checkMakers(loadFromCsv(stream, '\t', false, shouldAbort, advancement));
        }

        beginTest("load csv markers with tab separator");
        {
            auto const result = std::string(TestResultsData::MarkersSpace_csv);
            std::istringstream stream(result);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            checkMakers(loadFromCsv(stream, ' ', false, shouldAbort, advancement));
        }

        beginTest("load csv points with comma separator");
        {
            auto const result = std::string(TestResultsData::Points_csv);
            std::istringstream stream(result);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            checkPoints(loadFromCsv(stream, ',', false, shouldAbort, advancement));
        }

        beginTest("load csv points with tab separator");
        {
            auto const result = std::string(TestResultsData::PointsTab_csv);
            std::istringstream stream(result);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            checkPoints(loadFromCsv(stream, '\t', false, shouldAbort, advancement));
        }

        beginTest("load csv points with space separator");
        {
            auto const result = std::string(TestResultsData::PointsSpace_csv);
            std::istringstream stream(result);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            checkPoints(loadFromCsv(stream, ' ', false, shouldAbort, advancement));
        }

        beginTest("load json error");
        {
            auto const result = std::string(TestResultsData::Error_json);
            std::istringstream stream(result);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            expectEquals(loadFromJson(stream, shouldAbort, advancement).index(), 1_z);
        }

        beginTest("load json markers");
        {
            auto const result = std::string(TestResultsData::Markers_json);
            std::istringstream stream(result);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            checkMakers(loadFromJson(stream, shouldAbort, advancement));
        }

        beginTest("load json points");
        {
            auto const result = std::string(TestResultsData::Points_json);
            std::istringstream stream(result);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            checkPoints(loadFromJson(stream, shouldAbort, advancement));
        }

        beginTest("load binary error");
        {
            std::stringstream stream;
            stream.write(TestResultsData::Error_dat, TestResultsData::Error_datSize);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            expectEquals(loadFromBinary(stream, shouldAbort, advancement).index(), 1_z);
        }

        beginTest("load binary Markers");
        {
            std::stringstream stream;
            stream.write(TestResultsData::Markers_dat, TestResultsData::Markers_datSize);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            checkMakers(loadFromBinary(stream, shouldAbort, advancement));
        }

        beginTest("load binary Points");
        {
            std::stringstream stream;
            stream.write(TestResultsData::Points_dat, TestResultsData::Points_datSize);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            checkPoints(loadFromBinary(stream, shouldAbort, advancement));
        }

        beginTest("load lab Markers");
        {
            std::stringstream stream;
            stream.write(TestResultsData::Markers_lab, TestResultsData::Markers_labSize);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            checkMakers(loadFromCsv(stream, '\t', true, shouldAbort, advancement));
        }
    }
};

static Track::Loader::UnitTest trackLoaderUnitTest;

ANALYSE_FILE_END
