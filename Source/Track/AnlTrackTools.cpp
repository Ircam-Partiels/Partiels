#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

bool Track::Tools::hasPluginKey(Accessor const& acsr)
{
    return !acsr.getAttr<AttrType::key>().identifier.empty();
}

bool Track::Tools::hasResultFile(Accessor const& acsr)
{
    return !acsr.getAttr<AttrType::file>().isEmpty();
}

bool Track::Tools::supportsWindowType(Accessor const& acsr)
{
    auto const description = acsr.getAttr<AttrType::description>();
    return hasPluginKey(acsr) && description.inputDomain == Plugin::InputDomain::FrequencyDomain;
}

bool Track::Tools::supportsBlockSize(Accessor const& acsr)
{
    auto const description = acsr.getAttr<AttrType::description>();
    return hasPluginKey(acsr) && (description.inputDomain == Plugin::InputDomain::FrequencyDomain || description.defaultState.blockSize == 0_z);
}

bool Track::Tools::supportsStepSize(Accessor const& acsr)
{
    auto const description = acsr.getAttr<AttrType::description>();
    return hasPluginKey(acsr) && (description.inputDomain == Plugin::InputDomain::FrequencyDomain || description.defaultState.stepSize > 0_z);
}

bool Track::Tools::supportsInputTrack(Accessor const& acsr)
{
    return hasPluginKey(acsr) && !acsr.getAttr<AttrType::description>().input.identifier.empty();
}

std::optional<Track::FrameType> Track::Tools::getFrameType(Plugin::Output const& output)
{
    if(output.hasFixedBinCount)
    {
        switch(output.binCount)
        {
            case 0_z:
                return FrameType::label;
            case 1_z:
                return FrameType::value;
            default:
                return FrameType::vector;
        }
    }
    return {};
}

std::optional<Track::FrameType> Track::Tools::getFrameType(Accessor const& acsr)
{
    auto frameType = getFrameType(acsr.getAttr<AttrType::description>().output);
    if(frameType.has_value())
    {
        return frameType;
    }
    auto const& results = acsr.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(static_cast<bool>(access))
    {
        if(results.getMarkers() != nullptr)
        {
            return FrameType::label;
        }
        if(results.getPoints() != nullptr)
        {
            return FrameType::value;
        }
        if(results.getColumns() != nullptr)
        {
            return FrameType::vector;
        }
    }
    return {};
}

optional_ref<Zoom::Accessor const> Track::Tools::getVerticalZoomAccessor(Accessor const& accessor, bool useLinkedZoom)
{
    if(useLinkedZoom && accessor.getAttr<AttrType::zoomLink>())
    {
        return accessor.getAttr<AttrType::zoomAcsr>();
    }
    auto const frameType = Tools::getFrameType(accessor);
    if(frameType.has_value())
    {
        switch(frameType.value())
        {
            case FrameType::label:
                return {};
            case FrameType::value:
                return std::ref(accessor.getAcsr<AcsrType::valueZoom>());
            case FrameType::vector:
                return std::ref(accessor.getAcsr<AcsrType::binZoom>());
        }
    }
    return {};
}

optional_ref<Zoom::Accessor> Track::Tools::getVerticalZoomAccessor(Accessor& accessor, bool useLinkedZoom)
{
    if(useLinkedZoom && accessor.getAttr<AttrType::zoomLink>())
    {
        return accessor.getAttr<AttrType::zoomAcsr>();
    }
    auto const frameType = Tools::getFrameType(accessor);
    if(frameType.has_value())
    {
        switch(frameType.value())
        {
            case FrameType::label:
                return {};
            case FrameType::value:
                return std::ref(accessor.getAcsr<AcsrType::valueZoom>());
            case FrameType::vector:
                return std::ref(accessor.getAcsr<AcsrType::binZoom>());
        }
    }
    return {};
}

bool Track::Tools::hasVerticalZoom(Accessor const& accessor)
{
    return Tools::getFrameType(accessor) != FrameType::label;
}

bool Track::Tools::canZoomIn(Accessor const& accessor)
{
    auto const zoomAcsr = getVerticalZoomAccessor(accessor, false);
    return zoomAcsr.has_value() && Zoom::Tools::canZoomIn(zoomAcsr.value());
}

bool Track::Tools::canZoomOut(Accessor const& accessor)
{
    auto const zoomAcsr = getVerticalZoomAccessor(accessor, false);
    return zoomAcsr.has_value() && Zoom::Tools::canZoomOut(zoomAcsr.value());
}

void Track::Tools::zoomIn(Accessor& accessor, double ratio, NotificationType notification)
{
    auto const zoomAcsr = getVerticalZoomAccessor(accessor, false);
    if(zoomAcsr.has_value())
    {
        Zoom::Tools::zoomIn(zoomAcsr.value(), ratio, notification);
    }
}

void Track::Tools::zoomOut(Accessor& accessor, double ratio, NotificationType notification)
{
    auto const zoomAcsr = getVerticalZoomAccessor(accessor, false);
    if(zoomAcsr.has_value())
    {
        Zoom::Tools::zoomOut(zoomAcsr.value(), ratio, notification);
    }
}

bool Track::Tools::hasVerticalZoomInHertz(Accessor const& accessor)
{
    auto const isHertz = [](std::string unit)
    {
        std::transform(unit.cbegin(), unit.cend(), unit.begin(), [](unsigned char c)
                       {
                           return std::tolower(c);
                       });
        return unit.find("hz") != std::string::npos || unit.find("hertz") != std::string::npos;
    };
    if(isHertz(getUnit(accessor).toStdString()))
    {
        return true;
    }
    auto const& binNames = accessor.getAttr<AttrType::description>().output.binNames;
    return std::any_of(binNames.cbegin(), binNames.cend(), isHertz);
}

float Track::Tools::valueToPixel(float value, juce::Range<double> const& valueRange, juce::Rectangle<float> const& bounds)
{
    return (1.0f - static_cast<float>((static_cast<double>(value) - valueRange.getStart()) / valueRange.getLength())) * bounds.getHeight() + bounds.getY();
}

float Track::Tools::pixelToValue(float position, juce::Range<double> const& valueRange, juce::Rectangle<float> const& bounds)
{
    return (1.0f - ((position - bounds.getY()) / bounds.getHeight())) * static_cast<float>(valueRange.getLength()) + static_cast<float>(valueRange.getStart());
}

float Track::Tools::secondsToPixel(double seconds, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds)
{
    return static_cast<float>((seconds - timeRange.getStart()) / timeRange.getLength()) * bounds.getWidth() + bounds.getX();
}

double Track::Tools::pixelToSeconds(float position, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds)
{
    return static_cast<double>(position - bounds.getX()) / static_cast<double>(bounds.getWidth()) * timeRange.getLength() + timeRange.getStart();
}

juce::String Track::Tools::getUnit(Accessor const& acsr)
{
    return acsr.getAttr<AttrType::unit>().value_or(juce::String(acsr.getAttr<AttrType::description>().output.unit));
}

juce::String Track::Tools::getBinName(Accessor const& acsr, size_t index)
{
    auto const& output = acsr.getAttr<AttrType::description>().output;
    return juce::String(index < output.binNames.size() ? output.binNames.at(index) : "");
}

std::optional<Zoom::Range> Track::Tools::getResultRange(Accessor const& accessor)
{
    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    return static_cast<bool>(access) ? results.getValueRange() : decltype(results.getValueRange()){};
}

std::optional<Zoom::Range> Track::Tools::getValueRange(Plugin::Description const& description)
{
    auto const& output = description.output;
    if(!output.hasKnownExtents)
    {
        return std::optional<Zoom::Range>();
    }
    anlWeakAssert(std::isfinite(output.minValue) && std::isfinite(output.maxValue));
    if(!std::isfinite(output.minValue) || !std::isfinite(output.maxValue))
    {
        return std::optional<Zoom::Range>();
    }
    return Zoom::Range(static_cast<double>(output.minValue), static_cast<double>(output.maxValue));
}

std::optional<Zoom::Range> Track::Tools::getBinRange(Plugin::Description const& description)
{
    auto const& output = description.output;
    if(!output.hasFixedBinCount)
    {
        return std::optional<Zoom::Range>();
    }
    return Zoom::Range(0.0, static_cast<double>(output.binCount));
}

std::optional<Zoom::Range> Track::Tools::getExtraRange(Accessor const& accessor, size_t index)
{
    auto const& extraOutputs = accessor.getAttr<AttrType::description>().extraOutputs;
    if(index >= extraOutputs.size())
    {
        return {};
    }
    if(extraOutputs.at(index).hasKnownExtents)
    {
        return Zoom::Range{static_cast<double>(extraOutputs.at(index).minValue), static_cast<double>(extraOutputs.at(index).maxValue)};
    }
    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    return static_cast<bool>(access) ? results.getExtraRange(index) : std::optional<Zoom::Range>();
}

bool Track::Tools::isSelected(Accessor const& acsr, bool emptyTrackSupport)
{
    if(emptyTrackSupport && acsr.getAttr<AttrType::channelsLayout>().empty())
    {
        return acsr.getAttr<AttrType::focused>().size() >= 1_z && acsr.getAttr<AttrType::focused>()[0];
    }
    return !getSelectedChannels(acsr).empty();
}

std::set<size_t> Track::Tools::getSelectedChannels(Accessor const& acsr)
{
    auto const& states = acsr.getAttr<AttrType::focused>();
    auto const maxChannels = std::min(acsr.getAttr<AttrType::channelsLayout>().size(), states.size());
    std::set<size_t> channels;
    for(auto index = 0_z; index < maxChannels; ++index)
    {
        if(states[index])
        {
            channels.insert(index);
        }
    }
    return channels;
}

std::optional<size_t> Track::Tools::getChannel(Accessor const& acsr, juce::Rectangle<int> const& bounds, int y, bool ignoreSeparator)
{
    auto const verticalRanges = getChannelVerticalRanges(acsr, std::move(bounds));
    auto const it = std::find_if(verticalRanges.cbegin(), verticalRanges.cend(), [&](auto const& pair)
                                 {
                                     return ignoreSeparator ? y < pair.second.getEnd() : pair.second.contains(y);
                                 });
    return it != verticalRanges.cend() ? it->first : std::optional<size_t>{};
}

std::optional<std::tuple<size_t, juce::Range<int>>> Track::Tools::getChannelVerticalRange(Accessor const& acsr, juce::Rectangle<int> const& bounds, int y, bool ignoreSeparator)
{
    auto const verticalRanges = getChannelVerticalRanges(acsr, bounds);
    for(auto const& verticalRange : verticalRanges)
    {
        if((ignoreSeparator && y < verticalRange.second.getEnd()) || (!ignoreSeparator && verticalRange.second.contains(y)))
        {
            return std::make_tuple(verticalRange.first, verticalRange.second);
        }
    }
    return {};
}

std::map<size_t, juce::Range<int>> Track::Tools::getChannelVerticalRanges(juce::Rectangle<int> bounds, std::vector<bool> const& channelsLayout)
{
    auto const numVisibleChannels = static_cast<int>(std::count(channelsLayout.cbegin(), channelsLayout.cend(), true));
    if(numVisibleChannels == 0)
    {
        return {};
    }

    std::map<size_t, juce::Range<int>> verticalRanges;

    auto fullHeight = static_cast<float>(bounds.getHeight() - (numVisibleChannels - 1));
    auto const channelHeight = fullHeight / static_cast<float>(numVisibleChannels);
    auto remainder = 0.0f;

    auto channelCounter = 0;
    for(auto channel = 0_z; channel < channelsLayout.size(); ++channel)
    {
        if(channelsLayout[channel])
        {
            ++channelCounter;
            auto const currentHeight = std::min(channelHeight, fullHeight) + remainder;
            remainder = channelHeight - std::round(currentHeight);
            fullHeight -= std::round(currentHeight);
            auto region = bounds.removeFromTop(static_cast<int>(std::round(currentHeight)));
            if(channelCounter != numVisibleChannels)
            {
                region.removeFromBottom(1);
            }
            verticalRanges[channel] = region.getVerticalRange();
        }
    }
    return verticalRanges;
}

std::map<size_t, juce::Range<int>> Track::Tools::getChannelVerticalRanges(Accessor const& acsr, juce::Rectangle<int> bounds)
{
    return getChannelVerticalRanges(std::move(bounds), acsr.getAttr<AttrType::channelsLayout>());
}

Track::Results Track::Tools::convert(Plugin::Output const& output, std::vector<std::vector<Plugin::Result>>& pluginResults, std::atomic<bool> const& shouldAbort)
{
    auto const rtToS = [](Vamp::RealTime const& rt)
    {
        return static_cast<double>(rt.sec) + static_cast<double>(rt.nsec) / 1000000000.0;
    };

    if(shouldAbort)
    {
        return {};
    }

    if(output.hasFixedBinCount && output.binCount == 0_z)
    {
        std::vector<Results::Markers> results;
        results.reserve(pluginResults.size());
        for(auto& channelResults : pluginResults)
        {
            if(shouldAbort)
            {
                return {};
            }
            std::vector<Results::Marker> markers;
            markers.reserve(pluginResults.size());
            for(auto& result : channelResults)
            {
                anlWeakAssert(result.hasTimestamp);
                if(result.hasTimestamp)
                {
                    auto const time = rtToS(result.timestamp);
                    auto const duration = result.hasDuration ? rtToS(result.duration) : 0.0;
                    markers.push_back(std::make_tuple(time, duration, result.label, std::move(result.values)));
                }
            }
            results.push_back(std::move(markers));
        }
        return Results(std::move(results));
    }
    else if(output.hasFixedBinCount && output.binCount == 1_z)
    {
        std::vector<Results::Points> results;
        results.reserve(pluginResults.size());
        for(auto& channelResults : pluginResults)
        {
            if(shouldAbort)
            {
                return {};
            }
            std::vector<Results::Point> points;
            points.reserve(pluginResults.size());
            for(auto& result : channelResults)
            {
                anlWeakAssert(result.hasTimestamp);
                if(result.hasTimestamp)
                {
                    auto const time = rtToS(result.timestamp);
                    auto const duration = result.hasDuration ? rtToS(result.duration) : 0.0;
                    auto const valid = !result.values.empty() && std::isfinite(result.values.at(0)) && !std::isnan(result.values.at(0));
                    auto const value = valid ? std::make_optional(result.values.at(0)) : std::optional<float>();
                    if(!result.values.empty())
                    {
                        result.values.erase(result.values.begin());
                    }
                    points.push_back(std::make_tuple(time, duration, value, std::move(result.values)));
                }
            }
            results.push_back(std::move(points));
        }
        return Results(std::move(results));
    }
    else if(output.hasFixedBinCount)
    {
        std::vector<Results::Columns> results;
        results.reserve(pluginResults.size());
        for(auto& channelResults : pluginResults)
        {
            if(shouldAbort)
            {
                return {};
            }
            std::vector<Results::Column> columns;
            columns.reserve(pluginResults.size());
            for(auto& result : channelResults)
            {
                anlWeakAssert(result.hasTimestamp);
                if(result.hasTimestamp)
                {
                    auto const time = rtToS(result.timestamp);
                    auto const duration = result.hasDuration ? rtToS(result.duration) : 0.0;
                    auto const size = output.binCount - std::min(output.binCount, result.values.size());
                    std::vector<float> extra(size);
                    if(size > 0_z)
                    {
                        auto const end = std::next(result.values.cbegin(), static_cast<long>(size));
                        std::copy(end, result.values.cend(), extra.begin());
                        result.values.erase(end, result.values.cend());
                    }
                    columns.push_back(std::make_tuple(time, duration, std::move(result.values), std::move(extra)));
                }
            }
            results.push_back(std::move(columns));
        }
        return Results(std::move(results));
    }

    std::vector<Results::Columns> results;
    results.reserve(pluginResults.size());
    for(auto const& channelResults : pluginResults)
    {
        if(shouldAbort)
        {
            return {};
        }
        std::vector<Results::Column> columns;
        columns.reserve(pluginResults.size());
        for(auto& result : channelResults)
        {
            anlWeakAssert(result.hasTimestamp);
            if(result.hasTimestamp)
            {
                auto const time = rtToS(result.timestamp);
                auto const duration = result.hasDuration ? rtToS(result.duration) : 0.0;
                columns.push_back(std::make_tuple(time, duration, std::move(result.values), std::vector<float>{}));
            }
        }
        results.push_back(std::move(columns));
    }
    return Results(std::move(results));
}

std::vector<std::vector<Plugin::Result>> Track::Tools::convert(Plugin::Input const& input, Results const& results)
{
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return {};
    }
    auto const frameType = getFrameType(input);
    if(frameType.has_value())
    {
        switch(frameType.value())
        {
            case FrameType::label:
            {
                auto const sourceResults = results.getMarkers();
                if(sourceResults == nullptr)
                {
                    return {};
                }
                std::vector<std::vector<Plugin::Result>> pluginResults;
                pluginResults.resize(sourceResults->size());
                for(auto channelIndex = 0_z; channelIndex < sourceResults->size(); ++channelIndex)
                {
                    auto& pluginChannel = pluginResults[channelIndex];
                    auto const& sourceChannel = sourceResults->at(channelIndex);
                    pluginChannel.resize(sourceChannel.size());
                    for(auto markerIndex = 0_z; markerIndex < sourceChannel.size(); ++markerIndex)
                    {
                        auto const& sourceMarker = sourceChannel.at(markerIndex);
                        auto& pluginMaker = pluginChannel[markerIndex];
                        pluginMaker.hasTimestamp = true;
                        pluginMaker.timestamp = Vamp::RealTime::fromSeconds(std::get<0_z>(sourceMarker));
                        pluginMaker.hasDuration = input.hasDuration;
                        if(input.hasDuration)
                        {
                            pluginMaker.duration = Vamp::RealTime::fromSeconds(std::get<1_z>(sourceMarker));
                        }
                        pluginMaker.label = std::get<2_z>(sourceMarker);
                    }
                }
                return pluginResults;
            }
            case FrameType::value:
            {
                auto const sourceResults = results.getPoints();
                if(sourceResults == nullptr)
                {
                    return {};
                }
                std::vector<std::vector<Plugin::Result>> pluginResults;
                pluginResults.resize(sourceResults->size());
                for(auto channelIndex = 0_z; channelIndex < sourceResults->size(); ++channelIndex)
                {
                    auto& pluginChannel = pluginResults[channelIndex];
                    auto const& sourceChannel = sourceResults->at(channelIndex);
                    pluginChannel.resize(sourceChannel.size());
                    for(auto markerIndex = 0_z; markerIndex < sourceChannel.size(); ++markerIndex)
                    {
                        auto const& sourcePoint = sourceChannel.at(markerIndex);
                        auto& pluginPoint = pluginChannel[markerIndex];
                        pluginPoint.hasTimestamp = true;
                        pluginPoint.timestamp = Vamp::RealTime::fromSeconds(std::get<0_z>(sourcePoint));
                        pluginPoint.hasDuration = input.hasDuration;
                        if(input.hasDuration)
                        {
                            pluginPoint.duration = Vamp::RealTime::fromSeconds(std::get<1_z>(sourcePoint));
                        }
                        if(std::get<2_z>(sourcePoint).has_value())
                        {
                            pluginPoint.values = {std::get<2_z>(sourcePoint).value()};
                        }
                    }
                }
                return pluginResults;
            }
            case FrameType::vector:
            {
                auto const sourceResults = results.getColumns();
                if(sourceResults == nullptr)
                {
                    return {};
                }
                std::vector<std::vector<Plugin::Result>> pluginResults;
                pluginResults.resize(sourceResults->size());
                for(auto channelIndex = 0_z; channelIndex < sourceResults->size(); ++channelIndex)
                {
                    auto& pluginChannel = pluginResults[channelIndex];
                    auto const& sourceChannel = sourceResults->at(channelIndex);
                    pluginChannel.resize(sourceChannel.size());
                    for(auto markerIndex = 0_z; markerIndex < sourceChannel.size(); ++markerIndex)
                    {
                        auto const& sourceColumn = sourceChannel.at(markerIndex);
                        auto& pluginColumn = pluginChannel[markerIndex];
                        pluginColumn.hasTimestamp = true;
                        pluginColumn.timestamp = Vamp::RealTime::fromSeconds(std::get<0_z>(sourceColumn));
                        pluginColumn.hasDuration = input.hasDuration;
                        if(input.hasDuration)
                        {
                            pluginColumn.duration = Vamp::RealTime::fromSeconds(std::get<1_z>(sourceColumn));
                        }
                        pluginColumn.values = std::get<2_z>(sourceColumn);
                    }
                }
                return pluginResults;
            }
        }
    }
    return {};
}

std::unique_ptr<juce::Component> Track::Tools::createValueRangeEditor(Accessor& acsr)
{
    class RangeEditor
    : public juce::Component
    {
    public:
        RangeEditor(Zoom::Accessor& accessor, juce::String const name, juce::String const unit)
        : mAccessor(accessor)
        , mName("Name", name)
        , mPropertyStart("Low Value", "The low value of the visible range", unit, {0.0f, 1.0f}, 0.0f, [&](float value)
                         {
                             auto const range = mAccessor.getAttr<Zoom::AttrType::visibleRange>().withStart(static_cast<double>(value));
                             mAccessor.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
                         })
        , mPropertyEnd("High Value", "The high value of the visible range", unit, {0.0f, 1.0f}, 0.0f, [&](float value)
                       {
                           auto const range = mAccessor.getAttr<Zoom::AttrType::visibleRange>().withEnd(static_cast<double>(value));
                           mAccessor.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
                       })
        {
            setWantsKeyboardFocus(true);
            addAndMakeVisible(mName);
            addAndMakeVisible(mPropertyStart);
            addAndMakeVisible(mPropertyEnd);
            mListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
            {
                switch(attribute)
                {
                    case Zoom::AttrType::globalRange:
                    {
                        auto const range = acsr.getAttr<Zoom::AttrType::globalRange>();
                        mPropertyStart.entry.setRange(range, 0.0, juce::NotificationType::dontSendNotification);
                        mPropertyEnd.entry.setRange(range, 0.0, juce::NotificationType::dontSendNotification);
                    }
                    break;
                    case Zoom::AttrType::visibleRange:
                    {
                        auto const range = acsr.getAttr<Zoom::AttrType::visibleRange>();
                        mPropertyStart.entry.setValue(range.getStart(), juce::NotificationType::dontSendNotification);
                        mPropertyEnd.entry.setValue(range.getEnd(), juce::NotificationType::dontSendNotification);
                    }
                    break;
                    case Zoom::AttrType::minimumLength:
                    case Zoom::AttrType::anchor:
                        break;
                }
            };

            mAccessor.addListener(mListener, NotificationType::synchronous);
            setSize(180, 74);
        }

        ~RangeEditor() override
        {
            mAccessor.removeListener(mListener);
        }

        // juce::Component
        void resized() override
        {
            auto bounds = getLocalBounds();
            mName.setBounds(bounds.removeFromTop(24));
            mPropertyEnd.setBounds(bounds.removeFromTop(mPropertyEnd.getHeight()));
            mPropertyStart.setBounds(bounds.removeFromTop(mPropertyStart.getHeight()));
            setSize(getWidth(), bounds.getY() + 2);
        }

    private:
        Zoom::Accessor& mAccessor;
        Zoom::Accessor::Listener mListener{typeid(*this).name()};
        juce::Label mName;
        PropertyNumber mPropertyStart;
        PropertyNumber mPropertyEnd;
    };

    return std::make_unique<RangeEditor>(acsr.getAcsr<AcsrType::valueZoom>(), acsr.getAttr<AttrType::name>(), getUnit(acsr));
}

std::unique_ptr<juce::Component> Track::Tools::createBinRangeEditor(Accessor& acsr)
{
    class RangeEditor
    : public juce::Component
    {
    public:
        RangeEditor(Zoom::Accessor& accessor, juce::String const name, std::vector<std::string> const names)
        : mAccessor(accessor)
        , mName("Name", name)
        , mNames(names)
        , mPropertyStart("Low Bin", "The low bin of the visible range", "", {0.0f, 1.0f}, 0.01f, [&](float value)
                         {
                             auto const range = mAccessor.getAttr<Zoom::AttrType::visibleRange>().withStart(static_cast<double>(value));
                             mAccessor.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
                         })
        , mPropertyEnd("High Bin", "The high bin of the visible range", "", {0.0f, 1.0f}, 0.01f, [&](float value)
                       {
                           auto const range = mAccessor.getAttr<Zoom::AttrType::visibleRange>().withEnd(static_cast<double>(value));
                           mAccessor.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
                       })
        {
            setWantsKeyboardFocus(true);
            addAndMakeVisible(mName);
            addAndMakeVisible(mPropertyStart);
            addAndMakeVisible(mPropertyEnd);
            mListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
            {
                switch(attribute)
                {
                    case Zoom::AttrType::globalRange:
                    {
                        auto const range = acsr.getAttr<Zoom::AttrType::globalRange>();
                        mPropertyStart.entry.setRange(range, 0.0, juce::NotificationType::dontSendNotification);
                        mPropertyEnd.entry.setRange(range, 0.0, juce::NotificationType::dontSendNotification);
                    }
                    break;
                    case Zoom::AttrType::visibleRange:
                    {
                        auto const setValue = [this](NumberField& field, double value)
                        {
                            field.setValue(value, juce::NotificationType::dontSendNotification);
                            auto const index = static_cast<size_t>(std::floor(value));
                            field.setTextValueSuffix(index < mNames.size() ? " - " + mNames[index] : std::string());
                        };
                        auto const range = acsr.getAttr<Zoom::AttrType::visibleRange>();
                        setValue(mPropertyStart.entry, range.getStart());
                        setValue(mPropertyEnd.entry, range.getEnd());
                    }
                    break;
                    case Zoom::AttrType::minimumLength:
                    case Zoom::AttrType::anchor:
                        break;
                }
            };

            mAccessor.addListener(mListener, NotificationType::synchronous);
            setSize(180, 74);
        }

        ~RangeEditor() override
        {
            mAccessor.removeListener(mListener);
        }

        // juce::Component
        void resized() override
        {
            auto bounds = getLocalBounds();
            mName.setBounds(bounds.removeFromTop(24));
            mPropertyEnd.setBounds(bounds.removeFromTop(mPropertyEnd.getHeight()));
            mPropertyStart.setBounds(bounds.removeFromTop(mPropertyStart.getHeight()));
            setSize(getWidth(), bounds.getY() + 2);
        }

    private:
        Zoom::Accessor& mAccessor;
        Zoom::Accessor::Listener mListener{typeid(*this).name()};
        juce::Label mName;
        std::vector<std::string> const mNames;
        PropertyNumber mPropertyStart;
        PropertyNumber mPropertyEnd;
    };

    auto& zoomAcsr = acsr.getAcsr<AcsrType::binZoom>();
    auto const name = acsr.getAttr<AttrType::name>();
    auto const unit = acsr.getAttr<AttrType::description>().output.binNames;
    return std::make_unique<RangeEditor>(zoomAcsr, name, unit);
}

ANALYSE_FILE_END
