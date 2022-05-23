#include "AnlTrackResultModel.h"

ANALYSE_FILE_BEGIN

namespace
{
    template <typename T>
    typename T::const_iterator findFirstIteratorAt(T const& results, double time)
    {
        if(results.empty())
        {
            return results.cend();
        }
        auto const it = std::upper_bound(results.cbegin(), results.cend(), time, [](auto const& t, auto const& result)
                                         {
                                             return t < std::get<0_z>(result);
                                         });
        if(it == results.cbegin())
        {
            return it;
        }
        return std::prev(it);
    }

    template <typename T>
    inline bool isvalid(T const& value)
    {
        return std::isfinite(value) && !std::isnan(value);
    }

    static std::optional<Zoom::Range> getValueRange(std::vector<Track::Result::Data::Points> const& results)
    {
        auto const cmpPoints = [](auto const& lhs, auto const& rhs)
        {
            if(!std::get<2>(rhs).has_value())
            {
                return false;
            }
            if(!std::get<2>(lhs).has_value())
            {
                return true;
            }
            return *std::get<2>(lhs) < *std::get<2>(rhs);
        };

        auto const accumChannel = [&](auto const& range, auto const& channel) -> std::optional<Zoom::Range>
        {
            auto const [min, max] = std::minmax_element(channel.cbegin(), channel.cend(), cmpPoints);
            if(min == channel.cend() || max == channel.cend() || !std::get<2>(*min).has_value() || !std::get<2>(*max).has_value())
            {
                return range;
            }
            anlWeakAssert(isvalid(*std::get<2>(*min)) && isvalid(*std::get<2>(*max)));
            if(!isvalid(*std::get<2>(*min)) || !isvalid(*std::get<2>(*max)))
            {
                return range;
            }
            Zoom::Range const newRange{*std::get<2>(*min), *std::get<2>(*max)};
            return range.has_value() ? range->getUnionWith(newRange) : newRange;
        };

        return std::accumulate(results.cbegin(), results.cend(), std::optional<Zoom::Range>{}, accumChannel);
    }

    static std::optional<Zoom::Range> getValueRange(std::vector<Track::Result::Data::Columns> const& results)
    {
        auto const accumColumns = [&](auto const& range, auto const& column) -> std::optional<Zoom::Range>
        {
            auto const& values = std::get<2>(column);
            auto const [min, max] = std::minmax_element(values.cbegin(), values.cend());
            if(min == values.cend() || max == values.cend())
            {
                return range;
            }
            anlWeakAssert(isvalid(*min) && isvalid(*max));
            if(!isvalid(*min) || !isvalid(*max))
            {
                return range;
            }
            Zoom::Range const newRange{*min, *max};
            return range.has_value() ? newRange.getUnionWith(*range) : newRange;
        };

        auto const accumChannels = [&](auto const& range, auto const& channel) -> std::optional<Zoom::Range>
        {
            auto const newRange = std::accumulate(channel.cbegin(), channel.cend(), range, accumColumns);
            return range.has_value() ? (newRange.has_value() ? newRange->getUnionWith(*range) : range) : newRange;
        };

        return std::accumulate(results.cbegin(), results.cend(), std::optional<Zoom::Range>{}, accumChannels);
    }

    static size_t getNumBins(std::vector<Track::Result::Data::Columns> const& results)
    {
        auto const accumChannel = [](auto const& val, auto const& channel)
        {
            return std::accumulate(channel.cbegin(), channel.cend(), val, [](auto const& rval, auto const& column)
                                   {
                                       return std::max(rval, std::get<2>(column).size());
                                   });
        };

        return std::accumulate(results.cbegin(), results.cend(), 0_z, accumChannel);
    }
} // namespace

Track::Result::Access::Access(Data const& data, Mode mode)
: mData(data)
, mMode(mode)
, mValid(mData.getAccess(mMode == Mode::read))
{
}

Track::Result::Access::Access(Access const& access)
: mData(access.mData)
, mMode(access.mMode)
, mValid(mData.getAccess(mMode == Mode::read))
{
}

Track::Result::Access::Access(Access&& access)
: mData(access.mData)
, mMode(access.mMode)
, mValid(access.mValid)
{
    access.mValid = false;
}

Track::Result::Access::~Access()
{
    if(mValid)
    {
        mData.releaseAccess(mMode == Mode::read);
    }
}

class Track::Result::Data::Impl
{
public:
    Impl()
    : mInfo(std::make_shared<Info>())
    {
    }

    Impl(Impl const& rhs)
    : mInfo(rhs.mInfo)
    {
    }

    Impl(std::vector<Markers>&& markers)
    : mInfo(std::make_shared<Info>())
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo != nullptr)
        {
            mInfo->data = std::make_shared<std::vector<Markers>>(std::move(markers));
        }
        updated();
    }

    Impl(std::vector<Points>&& points)
    : mInfo(std::make_shared<Info>())
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo != nullptr)
        {
            mInfo->data = std::make_shared<std::vector<Points>>(std::move(points));
        }
        updated();
    }

    Impl(std::vector<Columns>&& columns)
    : mInfo(std::make_shared<Info>())
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo != nullptr)
        {
            mInfo->data = std::make_shared<std::vector<Columns>>(std::move(columns));
        }
        updated();
    }

    ~Impl()
    {
        MiscWeakAssert(mInfo != nullptr && (mInfo.use_count() > 1l || mInfo->accessCounter == 0_z));
    }

    std::shared_ptr<std::vector<Markers> const> getReadMarkers() const noexcept
    {
        MiscWeakAssert(hasAccess(true) && mInfo != nullptr);
        if(!hasAccess(true) || mInfo == nullptr)
        {
            return {};
        }
        if(auto const* markersPtr = std::get_if<std::shared_ptr<std::vector<Markers>>>(&mInfo->data))
        {
            return *markersPtr;
        }
        return {};
    }

    std::shared_ptr<std::vector<Points> const> getReadPoints() const noexcept
    {
        MiscWeakAssert(hasAccess(true) && mInfo != nullptr);
        if(!hasAccess(true) || mInfo == nullptr)
        {
            return {};
        }
        if(auto const* pointsPtr = std::get_if<std::shared_ptr<std::vector<Points>>>(&mInfo->data))
        {
            return *pointsPtr;
        }
        return {};
    }

    std::shared_ptr<std::vector<Columns> const> getReadColumns() const noexcept
    {
        MiscWeakAssert(hasAccess(true) && mInfo != nullptr);
        if(!hasAccess(true) || mInfo == nullptr)
        {
            return {};
        }
        if(auto const* columnsPtr = std::get_if<std::shared_ptr<std::vector<Columns>>>(&mInfo->data))
        {
            return *columnsPtr;
        }
        return {};
    }

    std::shared_ptr<std::vector<Markers>> getWriteMarkers() noexcept
    {
        MiscWeakAssert(hasAccess(false) && mInfo != nullptr);
        if(!hasAccess(false) || mInfo == nullptr)
        {
            return {};
        }
        if(auto* markersPtr = std::get_if<std::shared_ptr<std::vector<Markers>>>(&mInfo->data))
        {
            return *markersPtr;
        }
        return nullptr;
    }

    std::shared_ptr<std::vector<Points>> getWritePoints() noexcept
    {
        MiscWeakAssert(hasAccess(false) && mInfo != nullptr);
        if(!hasAccess(false) || mInfo == nullptr)
        {
            return {};
        }
        if(auto* pointsPtr = std::get_if<std::shared_ptr<std::vector<Points>>>(&mInfo->data))
        {
            return *pointsPtr;
        }
        return nullptr;
    }

    std::shared_ptr<std::vector<Columns>> getWriteColumns() noexcept
    {
        MiscWeakAssert(hasAccess(false) && mInfo != nullptr);
        if(!hasAccess(false) || mInfo == nullptr)
        {
            return {};
        }
        if(auto* columnsPtr = std::get_if<std::shared_ptr<std::vector<Columns>>>(&mInfo->data))
        {
            return *columnsPtr;
        }
        return nullptr;
    }

    std::optional<size_t> getNumChannels() const noexcept
    {
        MiscWeakAssert(hasAccess(true) && mInfo != nullptr);
        if(!hasAccess(true) || mInfo == nullptr)
        {
            return {};
        }
        return mInfo->numChannels;
    }

    std::optional<size_t> getNumBins() const noexcept
    {
        MiscWeakAssert(hasAccess(true) && mInfo != nullptr);
        if(!hasAccess(true) || mInfo == nullptr)
        {
            return {};
        }
        return mInfo->numBins;
    }

    std::optional<Zoom::Range> getValueRange() const noexcept
    {
        MiscWeakAssert(hasAccess(true) && mInfo != nullptr);
        if(!hasAccess(true) || mInfo == nullptr)
        {
            return {};
        }
        return mInfo->valueRange;
    }

    inline bool operator==(Impl const& rhd) const noexcept
    {
        return mInfo == rhd.mInfo;
    }

    inline bool operator!=(Impl const& rhd) const noexcept
    {
        return !(*this == rhd);
    }

    bool getAccess(bool readOnly) noexcept
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return false;
        }
        auto& counter = mInfo->accessCounter;
        if(counter == writeAccess)
        {
            return false;
        }
        else if(readOnly)
        {
            if(counter < writeAccess - 1_z)
            {
                ++counter;
                return true;
            }
            return false;
        }
        else
        {
            if(counter == 0_z)
            {
                counter = writeAccess;
                return true;
            }
            return false;
        }
    }

    void releaseAccess(bool readOnly) noexcept
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return;
        }
        MiscWeakAssert(hasAccess(readOnly));
        if(hasAccess(readOnly))
        {
            auto& counter = mInfo->accessCounter;
            if(readOnly)
            {
                MiscWeakAssert(counter < writeAccess);
                --counter;
            }
            else
            {
                counter = 0_z;
                updated();
            }
        }
    }

    bool hasAccess(bool readOnly) const noexcept
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return false;
        }
        auto const& counter = mInfo->accessCounter;
        return readOnly ? counter > 0_z : counter == writeAccess;
    }

private:
    void updated()
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return;
        }
        auto const access = getAccess(true);
        if(!access)
        {
            return;
        }
        if(auto const markers = getReadMarkers())
        {
            mInfo->numChannels = markers->size();
            mInfo->numBins = 0_z;
            mInfo->valueRange = Zoom::Range::emptyRange(0.0);
        }
        else if(auto const points = getReadPoints())
        {
            mInfo->numChannels = points->size();
            mInfo->numBins = 1_z;
            mInfo->valueRange = ::Misc::getValueRange(*points);
        }
        else if(auto const columns = getReadColumns())
        {
            mInfo->numChannels = columns->size();
            mInfo->numBins = ::Misc::getNumBins(*columns);
            mInfo->valueRange = ::Misc::getValueRange(*columns);
        }
        else
        {
            mInfo->numChannels.reset();
            mInfo->numBins.reset();
            mInfo->valueRange.reset();
        }
        releaseAccess(true);
    }

    using DataType = std::variant<std::shared_ptr<std::vector<Markers>>, std::shared_ptr<std::vector<Points>>, std::shared_ptr<std::vector<Columns>>>;
    static constexpr size_t writeAccess = std::numeric_limits<size_t>::max();

    struct Info
    {
        DataType data{};
        std::optional<size_t> numChannels{};
        std::optional<size_t> numBins{};
        std::optional<Zoom::Range> valueRange{};
        size_t accessCounter{0_z};

        JUCE_LEAK_DETECTOR(Info)
    };
    std::shared_ptr<Info> mInfo;

    JUCE_LEAK_DETECTOR(Impl)
};

Track::Result::Data::Data()
: mImpl(std::make_unique<Impl>())
{
}

Track::Result::Data::Data(Data const& rhs)
: mImpl(std::make_unique<Impl>(*rhs.mImpl))
{
}

Track::Result::Data::Data(std::vector<Markers>&& markers)
: mImpl(std::make_unique<Impl>(std::move(markers)))
{
}

Track::Result::Data::Data(std::vector<Points>&& points)
: mImpl(std::make_unique<Impl>(std::move(points)))
{
}

Track::Result::Data::Data(std::vector<Columns>&& columns)
: mImpl(std::make_unique<Impl>(std::move(columns)))
{
}

Track::Result::Data::~Data()
{
    // Necessary for the private implementation deletion
}

Track::Result::Data& Track::Result::Data::operator=(Data const& rhs)
{
    mImpl = std::make_unique<Impl>(*rhs.mImpl);
    return *this;
}

bool Track::Result::Data::getAccess(bool readOnly) const
{
    MiscWeakAssert(mImpl != nullptr);
    if(mImpl == nullptr)
    {
        return false;
    }
    return mImpl->getAccess(readOnly);
}

void Track::Result::Data::releaseAccess(bool readOnly) const
{
    MiscWeakAssert(mImpl != nullptr);
    if(mImpl == nullptr)
    {
        return;
    }
    mImpl->releaseAccess(readOnly);
}

Track::Result::Access Track::Result::Data::getReadAccess() const
{
    return Access(*this, Access::Mode::read);
}

Track::Result::Access Track::Result::Data::getWriteAccess() const
{
    return Access(*this, Access::Mode::write);
}

bool Track::Result::Data::isEmpty() const noexcept
{
    return getMarkers() == nullptr && getPoints() == nullptr && getColumns() == nullptr;
}

std::shared_ptr<std::vector<Track::Result::Data::Markers> const> Track::Result::Data::getMarkers() const noexcept
{
    MiscWeakAssert(mImpl != nullptr);
    if(mImpl == nullptr)
    {
        return {};
    }
    return mImpl->getReadMarkers();
}

std::shared_ptr<std::vector<Track::Result::Data::Points> const> Track::Result::Data::getPoints() const noexcept
{
    MiscWeakAssert(mImpl != nullptr);
    if(mImpl == nullptr)
    {
        return {};
    }
    return mImpl->getReadPoints();
}

std::shared_ptr<std::vector<Track::Result::Data::Columns> const> Track::Result::Data::getColumns() const noexcept
{
    MiscWeakAssert(mImpl != nullptr);
    if(mImpl == nullptr)
    {
        return {};
    }
    return mImpl->getReadColumns();
}

std::shared_ptr<std::vector<Track::Result::Data::Markers>> Track::Result::Data::getMarkers() noexcept
{
    MiscWeakAssert(mImpl != nullptr);
    if(mImpl == nullptr)
    {
        return {};
    }
    return mImpl->getWriteMarkers();
}

std::shared_ptr<std::vector<Track::Result::Data::Points>> Track::Result::Data::getPoints() noexcept
{
    MiscWeakAssert(mImpl != nullptr);
    if(mImpl == nullptr)
    {
        return {};
    }
    return mImpl->getWritePoints();
}

std::shared_ptr<std::vector<Track::Result::Data::Columns>> Track::Result::Data::getColumns() noexcept
{
    MiscWeakAssert(mImpl != nullptr);
    if(mImpl == nullptr)
    {
        return {};
    }
    return mImpl->getWriteColumns();
}

std::optional<size_t> Track::Result::Data::getNumChannels() const noexcept
{
    MiscWeakAssert(mImpl != nullptr);
    if(mImpl == nullptr)
    {
        return {};
    }
    return mImpl->getNumChannels();
}

std::optional<size_t> Track::Result::Data::getNumBins() const noexcept
{
    MiscWeakAssert(mImpl != nullptr);
    if(mImpl == nullptr)
    {
        return {};
    }
    return mImpl->getNumBins();
}

std::optional<Zoom::Range> Track::Result::Data::getValueRange() const noexcept
{
    MiscWeakAssert(mImpl != nullptr);
    if(mImpl == nullptr)
    {
        return {};
    }
    return mImpl->getValueRange();
}

bool Track::Result::Data::operator==(Data const& rhd) const noexcept
{
    return mImpl == rhd.mImpl;
}

bool Track::Result::Data::operator!=(Data const& rhd) const noexcept
{
    return !(*this == rhd);
}

bool Track::Result::Data::matchWithEpsilon(Data const& other, double timeEpsilon, float valueEpsilon) const
{
    if(*this == other)
    {
        return true;
    }

    auto const match = [&](auto const& lhs, auto const rhs, auto const cmp)
    {
        MiscWeakAssert(lhs != nullptr);
        if((lhs == nullptr && rhs != nullptr) || (lhs != nullptr && rhs == nullptr))
        {
            return false;
        }
        return lhs->size() == rhs->size() && std::equal(lhs->cbegin(), lhs->cend(), rhs->cbegin(), [&](auto const& lchannel, auto const& rchannel)
                                                        {
                                                            return lchannel.size() == rchannel.size() && std::equal(lchannel.cbegin(), lchannel.cend(), rchannel.cbegin(), [&](auto const& ldata, auto const& rdata)
                                                                                                                    {
                                                                                                                        return std::abs(std::get<0_z>(ldata) - std::get<0_z>(rdata)) < timeEpsilon && std::abs(std::get<1_z>(ldata) - std::get<1_z>(rdata)) < timeEpsilon && cmp(std::get<2_z>(ldata), std::get<2_z>(rdata));
                                                                                                                    });
                                                        });
    };

    if(auto const markers = getMarkers())
    {
        return match(markers, other.getMarkers(), [](auto const& lhs, auto const& rhs)
                     {
                         return lhs == rhs;
                     });
    }
    else if(auto const points = getPoints())
    {
        return match(points, other.getPoints(), [&](auto const& lhs, auto const& rhs)
                     {
                         return lhs.has_value() == rhs.has_value() && (!lhs.has_value() || std::abs(*lhs - *rhs) < valueEpsilon);
                     });
    }
    else if(auto const columns = getColumns())
    {
        return match(columns, other.getColumns(), [&](auto const& lhs, auto const& rhs)
                     {
                         return lhs.size() == rhs.size() && std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin(), [&](auto const& lhsv, auto const& rhsv)
                                                                       {
                                                                           return std::abs(lhsv - rhsv) < valueEpsilon;
                                                                       });
                     });
    }
    else
    {
        return other.getMarkers() == nullptr && other.getPoints() == nullptr && other.getColumns() == nullptr;
    }
}

std::optional<std::string> Track::Result::Data::getValue(std::shared_ptr<std::vector<Markers> const> results, size_t channel, double time)
{
    if(results == nullptr || results->size() <= channel || results->at(channel).empty())
    {
        return {};
    }
    auto const& channelResults = results->at(channel);
    auto const it = findFirstAt(channelResults, time);
    if(it != channelResults.cend())
    {
        return std::get<2>(*it);
    }
    return {};
}

std::optional<float> Track::Result::Data::getValue(std::shared_ptr<std::vector<Points> const> results, size_t channel, double time)
{
    if(results == nullptr || results->size() <= channel || results->at(channel).empty())
    {
        return {};
    }
    auto const& channelResults = results->at(channel);
    auto const first = findFirstAt(channelResults, time);
    if(first == channelResults.cend())
    {
        return {};
    }
    auto const second = std::next(first);
    auto const end = std::get<0>(*first) + std::get<1>(*first);
    if(second == channelResults.cend() || time < end || !std::get<2>(*second).has_value())
    {
        return std::get<2>(*first);
    }
    auto const next = std::get<0>(*second);
    if((next - end) < std::numeric_limits<double>::epsilon() || !std::get<2>(*first).has_value())
    {
        return std::get<2>(*second);
    }
    auto const ratio = std::max(std::min((time - end) / (next - end), 1.0), 0.0);
    if(std::isnan(ratio) || !std::isfinite(ratio)) // Extra check in case (next - end) < std::numeric_limits<double>::epsilon()
    {
        return std::get<2>(*second);
    }
    return (1.0 - ratio) * *std::get<2>(*first) + ratio * *std::get<2>(*second);
}

std::optional<float> Track::Result::Data::getValue(std::shared_ptr<std::vector<Columns> const> results, size_t channel, double time, size_t bin)
{
    if(results == nullptr || results->size() <= channel || results->at(channel).empty())
    {
        return {};
    }
    auto const& channelResults = results->at(channel);
    auto const it = findFirstAt(channelResults, time);
    if(it == channelResults.cend() || std::get<2>(*it).empty())
    {
        return {};
    }
    auto const& column = std::get<2>(*it);
    if(bin >= column.size())
    {
        return {};
    }
    return column[bin];
}

Track::Result::Data::Markers::const_iterator Track::Result::Data::findFirstAt(Markers const& results, double time)
{
    return findFirstIteratorAt(results, time);
}

Track::Result::Data::Points::const_iterator Track::Result::Data::findFirstAt(Points const& results, double time)
{
    return findFirstIteratorAt(results, time);
}

Track::Result::Data::Columns::const_iterator Track::Result::Data::findFirstAt(Columns const& results, double time)
{
    return findFirstIteratorAt(results, time);
}

Track::Result::File::File(juce::File const& f, juce::StringPairArray const& a, nlohmann::json const& e, juce::String const& c)
: file(f)
, args(a)
, extra(e)
, commit(c)
{
}

bool Track::Result::File::operator==(File const& rhd) const noexcept
{
    return file == rhd.file && args == rhd.args && commit == rhd.commit;
}

bool Track::Result::File::operator!=(File const& rhd) const noexcept
{
    return !(*this == rhd);
}

bool Track::Result::File::isEmpty() const noexcept
{
    return file == juce::File{} && commit.isEmpty();
}

template <>
void XmlParser::toXml<Track::Result::File>(juce::XmlElement& xml, juce::Identifier const& attributeName, Track::Result::File const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "path", value.file);
        toXml(*child, "args", value.args);
        toXml(*child, "commit", value.commit);
        xml.addChildElement(child.release());
    }
}

template <>
auto XmlParser::fromXml<Track::Result::File>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Track::Result::File const& defaultValue)
    -> Track::Result::File
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Track::Result::File value;
    value.file = fromXml(*child, "path", defaultValue.file);
    value.args = fromXml(*child, "args", defaultValue.args);
    value.commit = fromXml(*child, "commit", defaultValue.commit);
    return value;
}

void Track::Result::to_json(nlohmann::json& j, File const& file)
{
    j["path"] = file.file;
    j["args"] = file.args;
    j["commit"] = file.commit;
}

void Track::Result::from_json(nlohmann::json const& j, File& file)
{
    file.file = j.value("path", file.file);
    file.args = j.value("args", file.args);
    file.commit = j.value("commit", file.commit);
}

class TrackResultUnitTest
: public juce::UnitTest
{
public:
    TrackResultUnitTest()
    : juce::UnitTest("Track", "Result")
    {
    }

    ~TrackResultUnitTest() override = default;

    void runTest() override
    {
        using namespace Track::Result;
        beginTest("findFirstAt");
        {
            // clang-format off
            Data::Markers markers
            {
                  {8.015293423, 0.0, "John"}
                , {8.421642629, 0.0, "Jimi"}
                , {8.816381858, 0.0, "James"}
            };
            // clang-format on
            expect(Data::findFirstAt(markers, 0.0) == markers.cbegin());
            expect(Data::findFirstAt(markers, 8.015293423) == markers.cbegin());
            expect(Data::findFirstAt(markers, 8.1) == markers.cbegin());
            expect(Data::findFirstAt(markers, 8.421642629) == std::next(markers.cbegin(), 1l));
            expect(Data::findFirstAt(markers, 8.5) == std::next(markers.cbegin(), 1l));
            expect(Data::findFirstAt(markers, 8.816381858) == std::next(markers.cbegin(), 2l));
            expect(Data::findFirstAt(markers, 8.9) == std::next(markers.cbegin(), 2l));
        }
    }
};

static TrackResultUnitTest trackResultUnitTest;

ANALYSE_FILE_END
