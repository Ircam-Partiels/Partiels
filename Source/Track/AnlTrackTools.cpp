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

Track::Tools::DisplayType Track::Tools::getDisplayType(Accessor const& acsr)
{
    auto const& results = acsr.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(static_cast<bool>(access))
    {
        if(results.getMarkers() != nullptr)
        {
            return DisplayType::markers;
        }
        if(results.getPoints() != nullptr)
        {
            return DisplayType::points;
        }
        if(results.getColumns() != nullptr)
        {
            return DisplayType::columns;
        }
    }
    auto const& output = acsr.getAttr<AttrType::description>().output;
    if(output.hasFixedBinCount)
    {
        switch(output.binCount)
        {
            case 0:
            {
                return DisplayType::markers;
            }
            break;
            case 1:
            {
                return DisplayType::points;
            }
            break;
            default:
            {
                return DisplayType::columns;
            }
            break;
        }
    }
    return DisplayType::points;
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

juce::String Track::Tools::getInfoTooltip(Accessor const& acsr)
{
    auto const& name = acsr.getAttr<AttrType::name>();
    auto const& file = acsr.getAttr<AttrType::file>().file;
    if(file != juce::File{})
    {
        return name + " (File): " + file.getFullPathName();
    }
    auto const& description = acsr.getAttr<AttrType::description>();
    if(description.name.isNotEmpty())
    {
        return name + " (Plugin): " + description.name + " - " + description.output.name;
    }
    return name;
}

juce::String Track::Tools::getValueTootip(Accessor const& accessor, Zoom::Accessor const& timeZoomAcsr, juce::Component const& component, int y, double time)
{
    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return "-";
    }
    auto const& timeGlobalRange = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
    if(results.isEmpty() || timeGlobalRange.isEmpty())
    {
        return "-";
    }

    auto getChannel = [&]() -> std::optional<std::tuple<size_t, juce::Range<int>>>
    {
        auto const verticalRanges = getChannelVerticalRanges(accessor, component.getLocalBounds());
        for(auto const& verticalRange : verticalRanges)
        {
            if(verticalRange.second.contains(y))
            {
                return std::make_tuple(verticalRange.first, verticalRange.second);
            }
        }
        return {};
    };

    auto const channel = getChannel();
    if(!channel.has_value())
    {
        return "-";
    }

    if(auto const markers = results.getMarkers())
    {
        auto const value = Results::getValue(markers, std::get<0>(*channel), time);
        if(value.has_value())
        {
            return *value + accessor.getAttr<AttrType::description>().output.unit;
        }
        return "-";
    }
    if(auto const points = results.getPoints())
    {
        auto const value = Results::getValue(points, std::get<0>(*channel), time);
        if(value.has_value())
        {
            return Format::valueToString(*value, 4) + accessor.getAttr<AttrType::description>().output.unit;
        }
        return "-";
    }
    if(auto const columns = results.getColumns())
    {
        auto getBinIndex = [&]() -> std::optional<size_t>
        {
            auto const binVisibleRange = accessor.getAcsr<AcsrType::binZoom>().getAttr<Zoom::AttrType::visibleRange>();
            auto const start = std::get<1>(*channel).getStart();
            auto const length = std::get<1>(*channel).getLength();
            if(length <= 0 || binVisibleRange.isEmpty())
            {
                return {};
            }
            auto const revertY = std::max(length - (y - start) - 1, 0);
            anlWeakAssert(revertY <= length - 1);
            auto const ratio = static_cast<double>(std::min(revertY, length - 1)) / static_cast<double>(length - 1);
            return static_cast<size_t>(std::floor(ratio * binVisibleRange.getLength() + binVisibleRange.getStart()));
        };

        auto binIndex = getBinIndex();
        if(!binIndex.has_value())
        {
            return "-";
        }

        auto const value = Results::getValue(columns, std::get<0>(*channel), time, *binIndex);
        if(value.has_value())
        {
            auto const& output = accessor.getAttr<AttrType::description>().output;
            auto getBinName = [&]()
            {
                if(*binIndex >= output.binNames.size() || output.binNames[*binIndex].empty())
                {
                    return "[" + juce::String(*binIndex) + "]";
                }
                return "[" + juce::String(*binIndex) + " - " + output.binNames[*binIndex] + "]";
            };
            return Format::valueToString(*value, 4) + output.unit + " " + getBinName();
        }
        return "-";
    }
    return "-";
}

juce::String Track::Tools::getStateTootip(Accessor const& acsr, bool includeAnalysisState, bool includeRenderingState)
{
    auto const& state = acsr.getAttr<AttrType::processing>();
    auto const& warnings = acsr.getAttr<AttrType::warnings>();
    auto const isLoading = acsr.getAttr<AttrType::file>().file != juce::File{};
    if(std::get<0>(state))
    {
        if(isLoading)
        {
            return juce::translate("Loading...");
        }
        return juce::translate("Analysing...");
    }
    if(std::get<2>(state) && includeRenderingState)
    {
        return juce::translate("Rendering...");
    }

    switch(warnings)
    {
        case WarningType::none:
            break;
        case WarningType::library:
            return juce::translate("The library cannot be found or loaded!");
        case WarningType::plugin:
            return juce::translate("The plugin cannot be allocated!");
        case WarningType::state:
            return juce::translate("The parameters are invalid!");
        case WarningType::file:
            return juce::translate("The file cannot be parsed!");
    }
    if(includeAnalysisState && includeRenderingState)
    {
        return isLoading ? juce::translate("Loading and rendering completed successfully!") : juce::translate("Analysis and rendering are completed successfully!");
    }
    else if(includeAnalysisState)
    {
        return isLoading ? juce::translate("Loading completed successfully!") : juce::translate("Analysis completed successfully!");
    }
    else if(includeRenderingState)
    {
        return juce::translate("Rendering completed successfully!");
    }
    return juce::translate("Loading and rendering completed successfully!");
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

bool Track::Tools::isSelected(Accessor const& acsr)
{
    if(acsr.getAttr<AttrType::channelsLayout>().empty())
    {
        return acsr.getAttr<AttrType::focused>().any();
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

size_t Track::Tools::getChannel(Accessor const& acsr, juce::Rectangle<int> bounds, int y)
{
    auto const verticalRanges = getChannelVerticalRanges(acsr, std::move(bounds));
    for(auto const& verticalRange : verticalRanges)
    {
        if(y < verticalRange.second.getEnd())
        {
            return verticalRange.first;
        }
    }
    return verticalRanges.empty() ? 0_z : verticalRanges.crbegin()->first;
}

std::map<size_t, juce::Range<int>> Track::Tools::getChannelVerticalRanges(Accessor const& acsr, juce::Rectangle<int> bounds)
{
    auto const channelsLayout = acsr.getAttr<AttrType::channelsLayout>();
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

void Track::Tools::paintChannels(Accessor const& acsr, juce::Graphics& g, juce::Rectangle<int> const& bounds, juce::Colour const& separatorColour, std::function<void(juce::Rectangle<int>, size_t channel)> fn)
{
    auto const verticalRanges = getChannelVerticalRanges(acsr, bounds);
    for(auto const& verticalRange : verticalRanges)
    {
        juce::Graphics::ScopedSaveState sss(g);
        auto const top = verticalRange.second.getStart();
        auto const bottom = verticalRange.second.getEnd();
        auto const region = bounds.withTop(top).withBottom(bottom);
        if(bottom < bounds.getHeight())
        {
            g.setColour(separatorColour);
            g.fillRect(bounds.withTop(bottom).withBottom(bottom + 1));
        }
        g.reduceClipRegion(region);
        if(fn != nullptr)
        {
            fn(region, verticalRange.first);
        }
    }
}

void Track::Tools::paintClippedImage(juce::Graphics& g, juce::Image const& image, juce::Rectangle<float> const& bounds)
{
    auto const graphicsBounds = g.getClipBounds().toFloat();
    auto const deltaX = -bounds.getX();
    auto const deltaY = -bounds.getY();
    auto const scaleX = graphicsBounds.getWidth() / bounds.getWidth();
    auto const scaleY = graphicsBounds.getHeight() / bounds.getHeight();

    g.drawImageTransformed(image, juce::AffineTransform::translation(deltaX, deltaY).scaled(scaleX, scaleY).translated(graphicsBounds.getX(), graphicsBounds.getY()));
}

Track::Results Track::Tools::getResults(Plugin::Output const& output, std::vector<std::vector<Plugin::Result>> const& pluginResults, std::atomic<bool> const& shouldAbort)
{
    auto rtToS = [](Vamp::RealTime const& rt)
    {
        return static_cast<double>(rt.sec) + static_cast<double>(rt.nsec) / 1000000000.0;
    };

    auto getBinCount = [&]() -> size_t
    {
        if(output.hasFixedBinCount)
        {
            return output.binCount;
        }
        return std::accumulate(pluginResults.cbegin(), pluginResults.cend(), 0_z, [&](auto val, auto const& channelResults)
                               {
                                   if(shouldAbort)
                                   {
                                       return val;
                                   }
                                   auto it = std::max_element(channelResults.cbegin(), channelResults.cend(), [](auto const& lhs, auto const& rhs)
                                                              {
                                                                  return lhs.values.size() < rhs.values.size();
                                                              });
                                   if(it != channelResults.cend())
                                   {
                                       return std::max(val, it->values.size());
                                   }
                                   return val;
                               });
    };

    if(shouldAbort)
    {
        return {};
    }

    switch(getBinCount())
    {
        case 0_z:
        {
            std::vector<Results::Markers> results;
            results.reserve(pluginResults.size());
            for(auto const& channelResults : pluginResults)
            {
                if(shouldAbort)
                {
                    return {};
                }
                std::vector<Results::Marker> markers;
                markers.reserve(pluginResults.size());
                for(auto const& result : channelResults)
                {
                    anlWeakAssert(result.hasTimestamp);
                    if(result.hasTimestamp)
                    {
                        markers.push_back(std::make_tuple(rtToS(result.timestamp), result.hasDuration ? rtToS(result.duration) : 0.0, result.label));
                    }
                }
                results.push_back(std::move(markers));
            }
            return Results(std::move(results));
        }
        break;
        case 1_z:
        {
            std::vector<Results::Points> results;
            results.reserve(pluginResults.size());
            for(auto const& channelResults : pluginResults)
            {
                if(shouldAbort)
                {
                    return {};
                }
                std::vector<Results::Point> points;
                points.reserve(pluginResults.size());
                for(auto const& result : channelResults)
                {
                    anlWeakAssert(result.hasTimestamp);
                    if(result.hasTimestamp)
                    {
                        auto const valid = !result.values.empty() && std::isfinite(result.values[0]) && !std::isnan(result.values[0]);
                        points.push_back(std::make_tuple(rtToS(result.timestamp), result.hasDuration ? rtToS(result.duration) : 0.0, valid ? result.values[0] : std::optional<float>()));
                    }
                }
                results.push_back(std::move(points));
            }
            return Results(std::move(results));
        }
        break;
        default:
        {
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
                        columns.push_back(std::make_tuple(rtToS(result.timestamp), result.hasDuration ? rtToS(result.duration) : 0.0, std::move(result.values)));
                    }
                }
                results.push_back(std::move(columns));
            }
            return Results(std::move(results));
        }
        break;
    }
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

    auto& zoomAcsr = acsr.getAcsr<AcsrType::valueZoom>();
    auto const name = acsr.getAttr<AttrType::name>();
    auto const unit = acsr.getAttr<AttrType::description>().output.unit;
    return std::make_unique<RangeEditor>(zoomAcsr, name, unit);
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
