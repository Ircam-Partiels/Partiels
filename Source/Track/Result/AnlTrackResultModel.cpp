#include "AnlTrackResultModel.h"

ANALYSE_FILE_BEGIN

namespace
{
    static std::optional<Zoom::Range> getValueRange(std::vector<Track::Result::Data::Points> const& results)
    {
        auto const accumChannel = [](auto const& range, auto const& channel) -> std::optional<Zoom::Range>
        {
            auto const accumPoint = [](auto const& channelRange, auto const& point) -> std::optional<Zoom::Range>
            {
                return std::get<2>(point).has_value() ? (channelRange.has_value() ? channelRange->getUnionWith(std::get<2>(point).value()) : Zoom::Range::emptyRange(std::get<2>(point).value())) : channelRange;
            };
            auto const channelRange = std::accumulate(channel.cbegin(), channel.cend(), std::optional<Zoom::Range>{}, accumPoint);
            return channelRange.has_value() ? (range.has_value() ? range->getUnionWith(channelRange.value()) : channelRange) : range;
        };
        return std::accumulate(results.cbegin(), results.cend(), std::optional<Zoom::Range>{}, accumChannel);
    }

    static std::optional<Zoom::Range> getValueRange(std::vector<Track::Result::Data::Columns> const& results)
    {
        auto const accumChannels = [](auto const& range, auto const& channel) -> std::optional<Zoom::Range>
        {
            auto const accumColumns = [](auto const& channelRange, auto const& column) -> std::optional<Zoom::Range>
            {
                if(std::get<2>(column).empty())
                {
                    return channelRange;
                }
                auto const [min, max] = std::minmax_element(std::get<2>(column).cbegin(), std::get<2>(column).cend());
                return channelRange.has_value() ? channelRange->getUnionWith({*min, *max}) : Zoom::Range{*min, *max};
            };
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

    static std::vector<Zoom::Range> getExtraRange(auto const& results)
    {
        std::vector<Zoom::Range> ranges;
        for(auto const& channel : results)
        {
            for(auto const& frame : channel)
            {
                for(auto index = 0_z; index < std::get<3>(frame).size(); ++index)
                {
                    if(index >= ranges.size())
                    {
                        ranges.push_back(Zoom::Range::emptyRange(std::get<3>(frame).at(index)));
                    }
                    else
                    {
                        ranges[index] = ranges.at(index).getUnionWith(std::get<3>(frame).at(index));
                    }
                }
            }
        }
        return ranges;
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
        std::unique_lock<std::mutex> lock(mInfo->accessMutex);
        auto const access = doGetAccess(true);
        MiscWeakAssert(access);
        if(access)
        {
            updated();
            doReleaseAccess(true);
        }
    }

    Impl(std::vector<Points>&& points)
    : mInfo(std::make_shared<Info>())
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo != nullptr)
        {
            mInfo->data = std::make_shared<std::vector<Points>>(std::move(points));
        }
        std::unique_lock<std::mutex> lock(mInfo->accessMutex);
        auto const access = doGetAccess(true);
        MiscWeakAssert(access);
        if(access)
        {
            updated();
            doReleaseAccess(true);
        }
    }

    Impl(std::vector<Columns>&& columns)
    : mInfo(std::make_shared<Info>())
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo != nullptr)
        {
            mInfo->data = std::make_shared<std::vector<Columns>>(std::move(columns));
        }
        std::unique_lock<std::mutex> lock(mInfo->accessMutex);
        auto const access = doGetAccess(true);
        MiscWeakAssert(access);
        if(access)
        {
            updated();
            doReleaseAccess(true);
        }
    }

    ~Impl()
    {
        std::unique_lock<std::mutex> lock(mInfo->accessMutex);
        MiscWeakAssert(mInfo != nullptr && (mInfo.use_count() > 1l || mInfo->accessCounter == 0_z));
    }

    std::shared_ptr<std::vector<Markers> const> getReadMarkers() const noexcept
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return {};
        }
        std::unique_lock<std::mutex> lock(mInfo->accessMutex);
        MiscWeakAssert(hasAccess(true));
        if(!hasAccess(true))
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
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return {};
        }
        std::unique_lock<std::mutex> lock(mInfo->accessMutex);
        MiscWeakAssert(hasAccess(true));
        if(!hasAccess(true))
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
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return {};
        }
        std::unique_lock<std::mutex> lock(mInfo->accessMutex);
        MiscWeakAssert(hasAccess(true));
        if(!hasAccess(true))
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
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return {};
        }
        std::unique_lock<std::mutex> lock(mInfo->accessMutex);
        MiscWeakAssert(hasAccess(false));
        if(!hasAccess(false))
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
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return {};
        }
        std::unique_lock<std::mutex> lock(mInfo->accessMutex);
        MiscWeakAssert(hasAccess(false));
        if(!hasAccess(false))
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
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return {};
        }
        std::unique_lock<std::mutex> lock(mInfo->accessMutex);
        MiscWeakAssert(hasAccess(false));
        if(!hasAccess(false))
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
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return {};
        }
        std::unique_lock<std::mutex> lock(mInfo->accessMutex);
        MiscWeakAssert(hasAccess(true));
        if(!hasAccess(true))
        {
            return {};
        }
        return mInfo->numChannels;
    }

    std::optional<size_t> getNumBins() const noexcept
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return {};
        }
        std::unique_lock<std::mutex> lock(mInfo->accessMutex);
        MiscWeakAssert(hasAccess(true));
        if(!hasAccess(true))
        {
            return {};
        }
        return mInfo->numBins;
    }

    std::optional<Zoom::Range> getValueRange() const noexcept
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return {};
        }
        std::unique_lock<std::mutex> lock(mInfo->accessMutex);
        MiscWeakAssert(hasAccess(true));
        if(!hasAccess(true))
        {
            return {};
        }
        return mInfo->valueRange;
    }

    std::optional<Zoom::Range> getExtraRange(size_t index) const noexcept
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return {};
        }
        std::unique_lock<std::mutex> lock(mInfo->accessMutex);
        MiscWeakAssert(hasAccess(true));
        if(!hasAccess(true))
        {
            return {};
        }
        if(index >= mInfo->extraRanges.size())
        {
            return {};
        }
        return mInfo->extraRanges.at(index);
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
        std::unique_lock<std::mutex> lock(mInfo->accessMutex);
        return doGetAccess(readOnly);
    }

    void releaseAccess(bool readOnly) noexcept
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return;
        }
        std::unique_lock<std::mutex> lock(mInfo->accessMutex);
        doReleaseAccess(readOnly);
    }

private:
    bool doGetAccess(bool readOnly)
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return false;
        }
        std::unique_lock<std::mutex> lock(mInfo->accessMutex, std::try_to_lock);
        MiscWeakAssert(!lock.owns_lock());
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

    void doReleaseAccess(bool readOnly)
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return;
        }
        // This lock should fail
        std::unique_lock<std::mutex> lock(mInfo->accessMutex, std::try_to_lock);
        MiscWeakAssert(!lock.owns_lock());
        auto& counter = mInfo->accessCounter;
        MiscWeakAssert(!readOnly || (counter > 0_z && counter < writeAccess));
        MiscWeakAssert(readOnly || counter == writeAccess);
        if(readOnly && counter > 0_z && counter < writeAccess)
        {
            --counter;
        }
        else if(!readOnly && counter == writeAccess)
        {
            counter = 0_z;
            updated();
        }
        else
        {
            MiscWeakAssert(false);
        }
    }

    bool hasAccess(bool readOnly) const noexcept
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return false;
        }
        // This lock should fail
        std::unique_lock<std::mutex> lock(mInfo->accessMutex, std::try_to_lock);
        MiscWeakAssert(!lock.owns_lock());
        auto const& counter = mInfo->accessCounter;
        return readOnly ? counter > 0_z : counter == writeAccess;
    }

    void updated()
    {
        MiscWeakAssert(mInfo != nullptr);
        if(mInfo == nullptr)
        {
            return;
        }
        // This lock should fail
        std::unique_lock<std::mutex> lock(mInfo->accessMutex, std::try_to_lock);
        MiscWeakAssert(!lock.owns_lock());
        if(auto const* markersPtr = std::get_if<std::shared_ptr<std::vector<Markers>>>(&mInfo->data))
        {
            auto const markers = *markersPtr;
            if(markers != nullptr)
            {
                mInfo->numChannels = markers->size();
                mInfo->numBins = 0_z;
                mInfo->valueRange = Zoom::Range::emptyRange(0.0);
                mInfo->extraRanges = ::Misc::getExtraRange(*markers);
            }
        }
        else if(auto const* pointsPtr = std::get_if<std::shared_ptr<std::vector<Points>>>(&mInfo->data))
        {
            auto const points = *pointsPtr;
            if(points != nullptr)
            {
                mInfo->numChannels = points->size();
                mInfo->numBins = 1_z;
                mInfo->valueRange = ::Misc::getValueRange(*points);
                mInfo->extraRanges = ::Misc::getExtraRange(*points);
            }
        }
        else if(auto const* columnsPtr = std::get_if<std::shared_ptr<std::vector<Columns>>>(&mInfo->data))
        {
            auto const columns = *columnsPtr;
            if(columns != nullptr)
            {
                mInfo->numChannels = columns->size();
                mInfo->numBins = ::Misc::getNumBins(*columns);
                mInfo->valueRange = ::Misc::getValueRange(*columns);
                mInfo->extraRanges = ::Misc::getExtraRange(*columns);
            }
        }
        else
        {
            mInfo->numChannels.reset();
            mInfo->numBins.reset();
            mInfo->valueRange.reset();
        }
    }

    using DataType = std::variant<std::shared_ptr<std::vector<Markers>>, std::shared_ptr<std::vector<Points>>, std::shared_ptr<std::vector<Columns>>>;
    static constexpr size_t writeAccess = std::numeric_limits<size_t>::max();

    struct Info
    {
        DataType data{};
        std::optional<size_t> numChannels{};
        std::optional<size_t> numBins{};
        std::optional<Zoom::Range> valueRange{};
        std::vector<Zoom::Range> extraRanges;
        std::mutex accessMutex;
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

std::optional<Zoom::Range> Track::Result::Data::getExtraRange(size_t index) const noexcept
{
    MiscWeakAssert(mImpl != nullptr);
    if(mImpl == nullptr)
    {
        return {};
    }
    return mImpl->getExtraRange(index);
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
    if(results == nullptr || results->size() <= channel)
    {
        return {};
    }
    auto const& channelResults = results->at(channel);
    if(channelResults.empty())
    {
        return {};
    }
    auto const it = std::upper_bound(channelResults.cbegin(), channelResults.cend(), time, upper_cmp<Marker>);
    if(it == channelResults.cbegin())
    {
        return {};
    }
    return std::get<2_z>(*std::prev(it));
}

std::optional<float> Track::Result::Data::getValue(std::shared_ptr<std::vector<Points> const> results, size_t channel, double time)
{
    if(results == nullptr || results->size() <= channel)
    {
        return {};
    }
    auto const& channelResults = results->at(channel);
    if(channelResults.empty())
    {
        return {};
    }
    auto const it = std::prev(std::upper_bound(std::next(channelResults.cbegin()), channelResults.cend(), time, upper_cmp<Point>));
    if(!std::get<2_z>(*it).has_value())
    {
        return {};
    }
    auto const start = std::get<0_z>(*it) + std::get<1_z>(*it);
    if(time < start)
    {
        return std::get<2_z>(*it).value();
    }
    auto const next = std::next(it);
    if(next == channelResults.cend() || !std::get<2_z>(*next).has_value())
    {
        return {};
    }
    if((std::get<0_z>(*next) - start) < std::numeric_limits<double>::epsilon())
    {
        return std::get<2_z>(*it).value();
    }
    auto const end = std::get<0_z>(*next);
    auto const ratio = std::max(std::min((time - start) / (end - start), 1.0), 0.0);
    if(std::isnan(ratio) || !std::isfinite(ratio)) // Extra check in case (next - end) < std::numeric_limits<double>::epsilon()
    {
        return std::get<2_z>(*it).value();
    }
    return static_cast<float>((1.0 - ratio) * static_cast<double>(std::get<2_z>(*it).value()) + ratio * static_cast<double>(std::get<2_z>(*next).value()));
}

std::optional<float> Track::Result::Data::getValue(std::shared_ptr<std::vector<Columns> const> results, size_t channel, double time, size_t bin)
{
    if(results == nullptr || results->size() <= channel)
    {
        return {};
    }
    auto const& channelResults = results->at(channel);
    if(channelResults.empty())
    {
        return {};
    }
    auto const it = std::upper_bound(channelResults.cbegin(), channelResults.cend(), time, upper_cmp<Column>);
    if(it == channelResults.cbegin())
    {
        return {};
    }
    auto const& column = std::get<2_z>(*std::prev(it));
    if(bin >= column.size())
    {
        return {};
    }
    return column.at(bin);
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
        // clang-format off
        Data::Markers markers
        {
              {8.015293423, 0.0, "John", {}}
            , {8.421642629, 0.0, "Jimi", {}}
            , {8.816381858, 0.0, "James", {}}
        };
        // clang-format on

        beginTest("lower_bounds");
        {
            expect(std::lower_bound(markers.cbegin(), markers.cend(), 0.0, lower_cmp<Data::Marker>) == markers.cbegin());
            expect(std::lower_bound(markers.cbegin(), markers.cend(), 8.015293423, lower_cmp<Data::Marker>) == markers.cbegin());
            expect(std::lower_bound(markers.cbegin(), markers.cend(), 8.1, lower_cmp<Data::Marker>) == std::next(markers.cbegin(), 1l));
            expect(std::lower_bound(markers.cbegin(), markers.cend(), 8.421642629, lower_cmp<Data::Marker>) == std::next(markers.cbegin(), 1l));
            expect(std::lower_bound(markers.cbegin(), markers.cend(), 8.5, lower_cmp<Data::Marker>) == std::next(markers.cbegin(), 2l));
            expect(std::lower_bound(markers.cbegin(), markers.cend(), 8.816381858, lower_cmp<Data::Marker>) == std::next(markers.cbegin(), 2l));
            expect(std::lower_bound(markers.cbegin(), markers.cend(), 8.9, lower_cmp<Data::Marker>) == markers.cend());
        }

        beginTest("upper_bounds");
        {
            expect(std::upper_bound(markers.cbegin(), markers.cend(), 0.0, upper_cmp<Data::Marker>) == markers.cbegin());
            expect(std::upper_bound(markers.cbegin(), markers.cend(), 8.015293423, upper_cmp<Data::Marker>) == std::next(markers.cbegin(), 1l));
            expect(std::upper_bound(markers.cbegin(), markers.cend(), 8.1, upper_cmp<Data::Marker>) == std::next(markers.cbegin(), 1l));
            expect(std::upper_bound(markers.cbegin(), markers.cend(), 8.421642629, upper_cmp<Data::Marker>) == std::next(markers.cbegin(), 2l));
            expect(std::upper_bound(markers.cbegin(), markers.cend(), 8.5, upper_cmp<Data::Marker>) == std::next(markers.cbegin(), 2l));
            expect(std::upper_bound(markers.cbegin(), markers.cend(), 8.816381858, upper_cmp<Data::Marker>) == markers.cend());
            expect(std::upper_bound(markers.cbegin(), markers.cend(), 8.9, upper_cmp<Data::Marker>) == markers.cend());
        }
    }
};

static TrackResultUnitTest trackResultUnitTest;

ANALYSE_FILE_END
