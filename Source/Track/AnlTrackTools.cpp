#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Tools::DisplayType Track::Tools::getDisplayType(Accessor const& acsr)
{
    auto const& results = acsr.getAttr<AttrType::results>();
    if(!results.isEmpty())
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

size_t Track::Tools::getNumColumns(Accessor const& acsr)
{
    auto const& results = acsr.getAttr<AttrType::results>();
    if(results.isEmpty())
    {
        return 0_z;
    }
    if(auto markers = results.getMarkers())
    {
        return std::accumulate(markers->cbegin(), markers->cend(), 0_z, [](auto const& val, auto const& channel)
                               {
                                   return std::max(val, channel.size());
                               });
    }
    if(auto points = results.getPoints())
    {
        return std::accumulate(points->cbegin(), points->cend(), 0_z, [](auto const& val, auto const& channel)
                               {
                                   return std::max(val, channel.size());
                               });
    }
    if(auto columns = results.getColumns())
    {
        return std::accumulate(columns->cbegin(), columns->cend(), 0_z, [](auto const& val, auto const& channel)
                               {
                                   return std::max(val, channel.size());
                               });
    }
    return 0_z;
}

size_t Track::Tools::getNumBins(Accessor const& acsr)
{
    auto const& results = acsr.getAttr<AttrType::results>();
    if(results.isEmpty() || results.getMarkers() != nullptr)
    {
        return 0_z;
    }
    if(results.getPoints() != nullptr)
    {
        return 1_z;
    }
    auto const columns = results.getColumns();
    if(columns == nullptr || columns->empty())
    {
        return 0_z;
    }
    return std::accumulate(columns->cbegin(), columns->cend(), 0_z, [](auto const& val, auto const& channel)
                           {
                               return std::accumulate(channel.cbegin(), channel.cend(), val, [](auto const& rval, auto const& column)
                                                      {
                                                          return std::max(rval, std::get<2>(column).size());
                                                      });
                           });
}

float Track::Tools::valueToPixel(float value, juce::Range<double> const& valueRange, juce::Rectangle<float> const& bounds)
{
    return (1.0f - static_cast<float>((static_cast<double>(value) - valueRange.getStart()) / valueRange.getLength())) * bounds.getHeight() + bounds.getY();
}

float Track::Tools::secondsToPixel(double seconds, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds)
{
    return static_cast<float>((seconds - timeRange.getStart()) / timeRange.getLength()) * bounds.getWidth() + bounds.getX();
}

double Track::Tools::pixelToSeconds(float position, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds)
{
    return static_cast<double>(position - bounds.getX()) / static_cast<double>(bounds.getWidth()) * timeRange.getLength() + timeRange.getStart();
}

std::optional<std::string> Track::Tools::getValue(Results::SharedMarkers results, size_t channel, Zoom::Range const& globalRange, double time)
{
    if(results == nullptr || results->size() <= channel || results->at(channel).empty())
    {
        return {};
    }
    auto const& channelResults = results->at(channel);
    auto const it = findFirstAt(channelResults, globalRange, time);
    if(it != channelResults.cend())
    {
        return std::get<2>(*it);
    }
    return {};
}

std::optional<float> Track::Tools::getValue(Results::SharedPoints results, size_t channel, Zoom::Range const& globalRange, double time)
{
    if(results == nullptr || results->size() <= channel || results->at(channel).empty())
    {
        return {};
    }
    auto const& channelResults = results->at(channel);
    auto const first = findFirstAt(channelResults, globalRange, time);
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
    if((next - end) > std::numeric_limits<double>::epsilon() || !std::get<2>(*first).has_value())
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

std::optional<float> Track::Tools::getValue(Results::SharedColumns results, size_t channel, Zoom::Range const& globalRange, double time, size_t bin)
{
    if(results == nullptr || results->size() <= channel || results->at(channel).empty())
    {
        return {};
    }
    auto const& channelResults = results->at(channel);
    auto const it = findFirstAt(channelResults, globalRange, time);
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

juce::String Track::Tools::getValueTootip(Accessor const& accessor, Zoom::Accessor const& timeZoomAcsr, juce::Component const& component, int y, double time)
{
    auto const results = accessor.getAttr<AttrType::results>();
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

    if(auto markers = results.getMarkers())
    {
        auto const value = getValue(markers, std::get<0>(*channel), timeGlobalRange, time);
        if(value.has_value())
        {
            return *value + accessor.getAttr<AttrType::description>().output.unit;
        }
        return "-";
    }
    if(auto points = results.getPoints())
    {
        auto const value = getValue(points, std::get<0>(*channel), timeGlobalRange, time);
        if(value.has_value())
        {
            return Format::valueToString(*value, 4) + accessor.getAttr<AttrType::description>().output.unit;
        }
        return "-";
    }
    if(auto columns = results.getColumns())
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

        auto const value = getValue(columns, std::get<0>(*channel), timeGlobalRange, time, *binIndex);
        if(value.has_value())
        {
            auto const& output = accessor.getAttr<AttrType::description>().output;
            auto getBinName = [&]()
            {
                if(*binIndex >= output.binNames.size())
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

juce::String Track::Tools::getStateTootip(Accessor const& acsr)
{
    auto const& state = acsr.getAttr<AttrType::processing>();
    auto const& warnings = acsr.getAttr<AttrType::warnings>();
    auto const isLoading = acsr.getAttr<AttrType::file>() != juce::File{};
    auto const name = acsr.getAttr<AttrType::name>() + ": ";
    if(std::get<0>(state))
    {
        if(isLoading)
        {
            return name + juce::translate("loading... (" + juce::String(static_cast<int>(std::round(std::get<1>(state) * 100.f))) + "%)");
        }
        return name + juce::translate("analysing... (" + juce::String(static_cast<int>(std::round(std::get<1>(state) * 100.f))) + "%)");
    }
    else if(std::get<2>(state))
    {
        return name + juce::translate("rendering... (" + juce::String(static_cast<int>(std::round(std::get<3>(state) * 100.f))) + "%)");
    }
    switch(warnings)
    {
        case WarningType::none:
            return name + juce::translate("analysis and rendering successfully completed!");
        case WarningType::plugin:
            return name + juce::translate("analysis failed: the plugin cannot be found or allocated!");
        case WarningType::state:
            return name + juce::translate("analysis failed: the step size or the block size might not be supported!");
        case WarningType::file:
            return name + juce::translate("loading failed: the results file cannot be parsed!");
    }
    return name + (isLoading ? juce::translate("loading and rendering successfully completed!") : juce::translate("analysis and rendering successfully completed!"));
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

std::optional<Zoom::Range> Track::Tools::getValueRange(Results const& results)
{
    if(results.isEmpty())
    {
        return std::optional<Zoom::Range>();
    }
    return results.getValueRange().isEmpty() ? std::optional<Zoom::Range>() : results.getValueRange();
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

std::optional<Zoom::Range> Track::Tools::getBinRange(Results const& results)
{
    if(results.isEmpty())
    {
        return std::optional<Zoom::Range>();
    }
    return Zoom::Range(0.0, static_cast<double>(results.getNumBins()));
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
            return Results(std::make_shared<const std::vector<Results::Markers>>(std::move(results)));
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

            auto const valueRange = getValueRange(results);
            return Results(std::make_shared<const std::vector<Results::Points>>(std::move(results)), valueRange);
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

            auto const numBins = getNumBins(results);
            auto const valueRange = getValueRange(results);
            return Results(std::make_shared<const std::vector<Results::Columns>>(std::move(results)), numBins, valueRange);
        }
        break;
    }
}

size_t Track::Tools::getNumBins(std::vector<Results::Columns> const& results)
{
    return std::accumulate(results.cbegin(), results.cend(), 0_z, [](auto const& val, auto const& channel)
                           {
                               return std::accumulate(channel.cbegin(), channel.cend(), val, [](auto const& rval, auto const& column)
                                                      {
                                                          return std::max(rval, std::get<2>(column).size());
                                                      });
                           });
}

Zoom::Range Track::Tools::getValueRange(std::vector<Results::Columns> const& results)
{
    auto const resultRange = std::accumulate(results.cbegin(), results.cend(), std::optional<Zoom::Range>{}, [&](auto const& range, auto const& channel) -> std::optional<Zoom::Range>
                                             {
                                                 auto const newRange = std::accumulate(channel.cbegin(), channel.cend(), range, [&](auto const& crange, auto const& column) -> std::optional<Zoom::Range>
                                                                                       {
                                                                                           auto const& values = std::get<2>(column);
                                                                                           auto const [min, max] = std::minmax_element(values.cbegin(), values.cend());
                                                                                           if(min == values.cend() || max == values.cend())
                                                                                           {
                                                                                               return crange;
                                                                                           }
                                                                                           anlWeakAssert(std::isfinite(*min) && std::isfinite(*max) && !std::isnan(*min) && !std::isnan(*max));
                                                                                           if(!std::isfinite(*min) || !std::isfinite(*max) || std::isnan(*min) || std::isnan(*max))
                                                                                           {
                                                                                               return crange;
                                                                                           }
                                                                                           Zoom::Range const newCrange{*min, *max};
                                                                                           return crange.has_value() ? newCrange.getUnionWith(*crange) : newCrange;
                                                                                       });
                                                 return range.has_value() ? (newRange.has_value() ? newRange->getUnionWith(*range) : range) : newRange;
                                             });
    if(resultRange.has_value())
    {
        return *resultRange;
    }
    return {};
}

Zoom::Range Track::Tools::getValueRange(std::vector<Results::Points> const& results)
{
    bool initialized = false;
    return std::accumulate(results.cbegin(), results.cend(), Zoom::Range(0.0, 0.0), [&](auto const& range, auto const& channel)
                           {
                               auto const [min, max] = std::minmax_element(channel.cbegin(), channel.cend(), [](auto const& lhs, auto const& rhs)
                                                                           {
                                                                               if(!std::get<2>(rhs).has_value())
                                                                               {
                                                                                   return false;
                                                                               }
                                                                               if(!std::get<2>(lhs).has_value())
                                                                               {
                                                                                   return false;
                                                                               }
                                                                               return *std::get<2>(lhs) < *std::get<2>(rhs);
                                                                           });
                               if(min == channel.cend() || max == channel.cend())
                               {
                                   return range;
                               }
                               anlWeakAssert(std::isfinite(*std::get<2>(*min)) && std::isfinite(*std::get<2>(*max)) && !std::isnan(*std::get<2>(*min)) && !std::isnan(*std::get<2>(*max)));
                               if(!std::isfinite(*std::get<2>(*min)) || !std::isfinite(*std::get<2>(*max)) || std::isnan(*std::get<2>(*min)) || std::isnan(*std::get<2>(*max)))
                               {
                                   return range;
                               }
                               Zoom::Range const newRange{*std::get<2>(*min), *std::get<2>(*max)};
                               return std::exchange(initialized, true) ? range.getUnionWith(newRange) : newRange;
                           });
}

void Track::Tools::showValueRangeEditor(Accessor& acsr)
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

    auto const point = juce::Desktop::getMousePosition();
    auto& zoomAcsr = acsr.getAcsr<AcsrType::valueZoom>();
    auto const name = acsr.getAttr<AttrType::name>();
    auto const unit = acsr.getAttr<AttrType::description>().output.unit;
    auto& box = juce::CallOutBox::launchAsynchronously(std::make_unique<RangeEditor>(zoomAcsr, name, unit), {point.getX(), point.getY(), 180, 74}, nullptr);
    box.setArrowSize(0.0f);
    box.resized();
}

void Track::Tools::showBinRangeEditor(Accessor& acsr)
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

    auto const point = juce::Desktop::getMousePosition();
    auto& zoomAcsr = acsr.getAcsr<AcsrType::binZoom>();
    auto const name = acsr.getAttr<AttrType::name>();
    auto const unit = acsr.getAttr<AttrType::description>().output.binNames;
    auto& box = juce::CallOutBox::launchAsynchronously(std::make_unique<RangeEditor>(zoomAcsr, name, unit), {point.getX(), point.getY(), 180, 74}, nullptr);
    box.setArrowSize(0.0f);
    box.resized();
}

ANALYSE_FILE_END
