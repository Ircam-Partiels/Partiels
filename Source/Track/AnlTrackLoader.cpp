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

void Track::Loader::loadAnalysis(Accessor const& accessor, FileInfo const& fileInfo)
{
    auto const name = accessor.getAttr<AttrType::name>();

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
                                     juce::Thread::setCurrentThreadPriority(10);
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
        else if(fileInfo.file.hasFileExtension("csv"))
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
    class ThreadedStreamIterator
    {
    public:
        typedef char value_type ANL_ATTR_UNUSED;
        typedef ptrdiff_t difference_type ANL_ATTR_UNUSED;
        typedef const char* pointer ANL_ATTR_UNUSED;
        typedef const char& reference ANL_ATTR_UNUSED;
        typedef std::input_iterator_tag iterator_category ANL_ATTR_UNUSED;

        ThreadedStreamIterator(std::atomic<bool> const& s)
        : shouldAbort(s)
        {
        }

        ThreadedStreamIterator(std::istream& i, std::atomic<bool> const& s)
        : iterator(i)
        , shouldAbort(s)
        {
        }

        ThreadedStreamIterator(ThreadedStreamIterator const& x)
        : iterator(x.iterator)
        , shouldAbort(x.shouldAbort)
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
            return shouldAbort ? true : iterator == rhs.iterator;
        }

        inline bool operator!=(ThreadedStreamIterator const& rhs) const
        {
            return shouldAbort ? false : iterator != rhs.iterator;
        }

    private:
        std::istream_iterator<char, char, std::char_traits<char>, ptrdiff_t> iterator;
        std::atomic<bool> const& shouldAbort;
    };

    ThreadedStreamIterator const itStart(stream, shouldAbort);
    ThreadedStreamIterator const itEnd(shouldAbort);
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
            if(shouldAbort)
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

    if(shouldAbort)
    {
        return {};
    }

    Plugin::Output output;
    output.hasFixedBinCount = false;
    advancement.store(0.9f);
    auto results = Tools::getResults(output, pluginResults, shouldAbort);
    advancement.store(1.0f);
    return {std::move(results)};
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
                return {juce::translate("Parsing error")};
            }
            markers.resize(static_cast<size_t>(numChannels));

            for(auto& marker : markers)
            {
                if(shouldAbort)
                {
                    return {};
                }

                if(!stream.read(reinterpret_cast<char*>(&std::get<0>(marker)), sizeof(std::get<0>(marker))))
                {
                    return {juce::translate("Parsing error")};
                }
                if(!stream.read(reinterpret_cast<char*>(&std::get<1>(marker)), sizeof(std::get<1>(marker))))
                {
                    return {juce::translate("Parsing error")};
                }
                uint64_t length;
                if(!stream.read(reinterpret_cast<char*>(&length), sizeof(length)))
                {
                    return {juce::translate("Parsing error")};
                }
                std::get<2>(marker).resize(length);
                if(!stream.read(reinterpret_cast<char*>(std::get<2>(marker).data()), static_cast<long>(length * sizeof(char))))
                {
                    return {juce::translate("Parsing error")};
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
                return {juce::translate("Parsing error")};
            }
            points.resize(static_cast<size_t>(numChannels));
            for(auto& point : points)
            {
                if(shouldAbort)
                {
                    return {};
                }

                if(!stream.read(reinterpret_cast<char*>(&std::get<0>(point)), sizeof(std::get<0>(point))))
                {
                    return {juce::translate("Parsing error")};
                }
                if(!stream.read(reinterpret_cast<char*>(&std::get<1>(point)), sizeof(std::get<1>(point))))
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
                return {juce::translate("Parsing error")};
            }
            columns.resize(static_cast<size_t>(numChannels));
            for(auto& column : columns)
            {
                if(shouldAbort)
                {
                    return {};
                }

                if(!stream.read(reinterpret_cast<char*>(&std::get<0>(column)), sizeof(std::get<0>(column))))
                {
                    return {juce::translate("Parsing error")};
                }
                if(!stream.read(reinterpret_cast<char*>(&std::get<1>(column)), sizeof(std::get<1>(column))))
                {
                    return {juce::translate("Parsing error")};
                }
                uint64_t numBins;
                if(!stream.read(reinterpret_cast<char*>(&numBins), sizeof(numBins)))
                {
                    return {juce::translate("Parsing error")};
                }
                std::get<2>(column).resize(numBins);
                if(!stream.read(reinterpret_cast<char*>(std::get<2>(column).data()), static_cast<long>(sizeof(*std::get<2>(column).data()) * numBins)))
                {
                    return {juce::translate("Parsing error")};
                }
            }
            results.push_back(std::move(columns));
        }

        auto const numBins = Tools::getNumBins(results);
        auto const valueRange = Tools::getValueRange(results);
        res = Results(std::make_shared<const std::vector<Results::Columns>>(std::move(results)), numBins, valueRange);
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
    return Results(std::make_shared<const std::vector<Results::Markers>>(std::move(channelResults)));
}

std::variant<Track::Results, juce::String> Track::Loader::loadFromCsv(FileInfo const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement)
{
    auto stream = std::ifstream(fileInfo.file.getFullPathName().toStdString());
    if(!stream || !stream.is_open() || !stream.good())
    {
        return {juce::translate("The input stream of cannot be opened")};
    }
    return loadFromCsv(stream, shouldAbort, advancement);
}

std::variant<Track::Results, juce::String> Track::Loader::loadFromCsv(std::istream& stream, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement)
{
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
                pluginResults.push_back({});
            }
            std::string time, duration, value;
            std::istringstream linestream(line);
            getline(linestream, time, ',');
            getline(linestream, duration, ',');
            getline(linestream, value, ',');
            if(time.empty() || duration.empty())
            {
                return {juce::translate("Parsing error")};
            }
            else if(std::isdigit(static_cast<int>(time[0])) && std::isdigit(static_cast<int>(duration[0])))
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
    auto results = Tools::getResults(output, pluginResults, shouldAbort);
    advancement.store(1.0f);
    return {std::move(results)};
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
                    }
                    else
                    {
                        auto const& rowValues = values->at(rowIndex);
                        result.values.reserve(rowValues.size());
                        for(auto index = 0_z; index < rowValues.size(); ++index)
                        {
                            result.values.push_back(static_cast<float>(rowValues[index]));
                        }
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
    output.hasFixedBinCount = false;
    advancement.store(0.9f);
    auto results = Tools::getResults(output, pluginResults, shouldAbort);
    advancement.store(1.0f);
    return {std::move(results)};
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
            auto results = *std::get_if<Results>(&vResult);
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
            auto results = *std::get_if<Results>(&vResult);
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
            expectEquals(loadFromCsv(stream, shouldAbort, advancement).index(), 1_z);
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
            expectEquals(loadFromCsv(stream, shouldAbort, advancement).index(), 1_z);
        }

        beginTest("load csv markers");
        {
            auto const result = std::string(TestResultsData::Markers_csv);
            std::istringstream stream(result);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            checkMakers(loadFromCsv(stream, shouldAbort, advancement));
        }

        beginTest("load csv points");
        {
            auto const result = std::string(TestResultsData::Points_csv);
            std::istringstream stream(result);
            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            checkPoints(loadFromCsv(stream, shouldAbort, advancement));
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
    }
};

static Track::Loader::UnitTest trackLoaderUnitTest;

ANALYSE_FILE_END
