#include "AnlTrackTooltip.h"
#include "../Plugin/AnlPluginTools.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

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

juce::String Track::Tools::getExtraTooltip(Accessor const& acsr, size_t index)
{
    auto const& extraOutputs = acsr.getAttr<AttrType::description>().extraOutputs;
    if(index >= extraOutputs.size())
    {
        return {};
    }
    auto const range = Tools::getExtraRange(acsr, index);
    if(range.has_value())
    {
        auto const min = static_cast<float>(range.value().getStart());
        auto const max = static_cast<float>(range.value().getEnd());
        return juce::String(extraOutputs.at(index).description) + " " + Plugin::Tools::rangeToString(min, max, extraOutputs.at(index).isQuantized, extraOutputs.at(index).quantizeStep);
    }
    return juce::String(extraOutputs.at(index).description);
}

namespace Track
{
    namespace Tools
    {
        static double getScaledVerticalValue(Zoom::Accessor const& zoomAcsr, juce::Range<int> const& pixelRange, int y)
        {
            auto const zoomRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            if(pixelRange.isEmpty() || zoomRange.isEmpty())
            {
                return zoomRange.getStart();
            }
            auto const pixelLength = pixelRange.getLength();
            auto const ratio = static_cast<double>(pixelLength - (y - pixelRange.getStart())) / static_cast<double>(pixelLength);
            return ratio * zoomRange.getLength() + zoomRange.getStart();
        }

        static std::optional<double> getScaledVerticalValue(Accessor const& acsr, juce::Component const& component, int y)
        {
            auto const frameType = getFrameType(acsr);
            if(frameType.has_value())
            {
                switch(frameType.value())
                {
                    case FrameType::label:
                        return {};
                    case FrameType::value:
                    {
                        auto const channelRange = getChannelVerticalRange(acsr, component.getLocalBounds(), y, false);
                        if(!channelRange.has_value())
                        {
                            return {};
                        }
                        auto const value = getScaledVerticalValue(acsr.getAcsr<AcsrType::valueZoom>(), std::get<1_z>(channelRange.value()), y);
                        if(acsr.getAttr<AttrType::zoomLogScale>() && hasVerticalZoomInHertz(acsr))
                        {
                            auto const nyquist = acsr.getAttr<AttrType::sampleRate>() / 2.0;
                            auto const midiMax = std::max(getMidiFromHertz(nyquist), 1.0);
                            return getHertzFromMidi(value / nyquist * midiMax);
                        }
                        return value;
                    }
                    case FrameType::vector:
                    {
                        auto const channelRange = getChannelVerticalRange(acsr, component.getLocalBounds(), y, false);
                        if(!channelRange.has_value())
                        {
                            return {};
                        }
                        auto const value = getScaledVerticalValue(acsr.getAcsr<AcsrType::binZoom>(), std::get<1_z>(channelRange.value()), y);
                        if(acsr.getAttr<AttrType::zoomLogScale>() && hasVerticalZoomInHertz(acsr))
                        {
                            auto const numBins = acsr.getAcsr<AcsrType::binZoom>().getAttr<Zoom::AttrType::globalRange>().getEnd();
                            auto const nyquist = acsr.getAttr<AttrType::sampleRate>() / 2.0;
                            auto const midiMax = std::max(getMidiFromHertz(nyquist), 1.0);
                            auto const midi = getHertzFromMidi(value / numBins * midiMax);
                            return midi / nyquist * numBins;
                        }
                        return value;
                    }
                }
            }
            return {};
        }

        [[nodiscard]] static juce::StringArray getExtraTooltip(Plugin::Description const& description, std::vector<float> const& extraValues)
        {
            juce::StringArray lines;
            juce::StringArray extraTooltip;
            auto const& extraOutputs = description.extraOutputs;
            for(auto index = 0_z; index < extraValues.size(); ++index)
            {
                if(index < extraOutputs.size())
                {
                    auto const name = juce::String(extraOutputs.at(index).name);
                    auto const unit = juce::String(extraOutputs.at(index).unit);
                    lines.add(name + ": " + Format::valueToString(extraValues.at(index), 4) + unit);
                }
                else
                {
                    auto const name = juce::translate("Extra INDEX").replace("INDEX", juce::String(index));
                    lines.add(name + ": " + Format::valueToString(extraValues.at(index), 4));
                }
            }
            return lines;
        }
    } // namespace Tools
} // namespace Track

juce::String Track::Tools::getZoomTootip(Accessor const& acsr, juce::Component const& component, int y)
{
    auto const frameType = getFrameType(acsr);
    if(frameType.has_value())
    {
        switch(frameType.value())
        {
            case FrameType::label:
                return {};
            case FrameType::value:
            {
                auto const value = getScaledVerticalValue(acsr, component, y);
                if(value.has_value())
                {
                    return Format::valueToString(value.value(), 4) + getUnit(acsr);
                }
                break;
            }
            case FrameType::vector:
            {
                auto const value = getScaledVerticalValue(acsr, component, y);
                if(value.has_value())
                {
                    auto const index = static_cast<size_t>(std::max(std::floor(value.value()), 0.0));
#if JUCE_DEBUG
                    auto const diff = value.value() - static_cast<double>(index);
                    auto const name = getBinName(acsr, index);
                    return juce::translate("Bin INDEX (+DIFF)").replace("INDEX", juce::String(index)).replace("DIFF", juce::String(diff, 2)) + (name.isNotEmpty() ? juce::String(" - ") + name : "");
#else
                    auto const name = getBinName(acsr, index);
                    return juce::translate("Bin INDEX").replace("INDEX", juce::String(index)) + (name.isNotEmpty() ? juce::String(" - ") + name : "");
#endif
                }
                break;
            }
        }
    }
    return {};
}

juce::StringArray Track::Tools::getValueTootip(Accessor const& accessor, Zoom::Accessor const& timeZoomAcsr, juce::Component const& component, int y, double time)
{
    juce::StringArray lines;
    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return lines;
    }
    auto const& timeGlobalRange = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
    if(results.isEmpty() || timeGlobalRange.isEmpty())
    {
        return lines;
    }

    auto const channelRange = getChannelVerticalRange(accessor, component.getLocalBounds(), y, false);
    if(!channelRange.has_value())
    {
        return lines;
    }

    auto const& description = accessor.getAttr<AttrType::description>();
    auto const name = juce::String(description.output.name.empty() ? "Feature" : description.output.name);
    auto const unit = getUnit(accessor);
    auto const channel = std::get<0_z>(channelRange.value());
    if(auto const markers = results.getMarkers())
    {
        if(markers->size() <= channel)
        {
            lines.add(name + ": -");
            return lines;
        }
        auto const& channelResults = markers->at(channel);
        if(channelResults.empty())
        {
            lines.add(name + ": -");
            return lines;
        }
        auto const it = std::prev(std::upper_bound(std::next(channelResults.cbegin()), channelResults.cend(), time, Result::upper_cmp<Result::Data::Marker>));
        if(std::get<2_z>(*it).empty())
        {
            lines.add(name + ": -");
        }
        else
        {
            lines.add(name + ": " + juce::String(std::get<2_z>(*it)) + unit);
        }
        lines.addArray(getExtraTooltip(description, std::get<3_z>(*it)));
        return lines;
    }
    if(auto const points = results.getPoints())
    {
        if(points->size() <= channel)
        {
            lines.add(name + ": -");
            return lines;
        }
        auto const& channelResults = points->at(channel);
        if(channelResults.empty())
        {
            lines.add(name + ": -");
            return lines;
        }
        auto const it = std::prev(std::upper_bound(std::next(channelResults.cbegin()), channelResults.cend(), time, Result::upper_cmp<Result::Data::Point>));
        if(!std::get<2_z>(*it).has_value())
        {
            lines.add(name + ": -");
            return lines;
        }
        auto const getValue = [&]() -> std::optional<float>
        {
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
        };
        auto const value = getValue();
        if(value.has_value())
        {
            lines.add(name + ": " + Format::valueToString(value.value(), 4) + unit);
            lines.addArray(getExtraTooltip(description, std::get<3_z>(*it)));
        }
        else
        {
            lines.add(name + ": -");
        }
        return lines;
    }
    if(auto const columns = results.getColumns())
    {
        if(columns->size() <= channel)
        {
            lines.add(name + ": -");
            return lines;
        }
        auto const& channelResults = columns->at(channel);
        if(channelResults.empty())
        {
            lines.add(name + ": -");
            return lines;
        }
        auto const value = getScaledVerticalValue(accessor, component, y);
        if(!value.has_value())
        {
            lines.add(name + ": -");
            return lines;
        }
        auto const it = std::prev(std::upper_bound(std::next(channelResults.cbegin()), channelResults.cend(), time, Result::upper_cmp<Result::Data::Column>));
        auto const binIndex = static_cast<size_t>(std::floor(value.value()));
        auto const frameIndex = std::distance(channelResults.cbegin(), it);
        auto const& column = std::get<2_z>(*it);
        if(binIndex >= column.size())
        {
            lines.add(name + ": -");
            return lines;
        }
        auto const binName = getBinName(accessor, binIndex);
        auto const info = juce::translate("frame FIDX - bin BIDX").replace("FIDX", juce::String(frameIndex)).replace("BIDX", juce::String(binIndex)) + (binName.isNotEmpty() ? (" - " + binName) : "");
        lines.add(name + ": " + Format::valueToString(column.at(binIndex), 4) + unit + " (" + info + ")");
        lines.addArray(getExtraTooltip(description, std::get<3_z>(*it)));
        return lines;
    }
    return lines;
}

juce::String Track::Tools::getStateTootip(Accessor const& acsr, bool includeAnalysisState, bool includeRenderingState)
{
    auto const& state = acsr.getAttr<AttrType::processing>();
    auto const& warnings = acsr.getAttr<AttrType::warnings>();
    auto const isLoading = acsr.getAttr<AttrType::file>().file != juce::File{};
    if(std::get<0_z>(state))
    {
        if(isLoading)
        {
            return juce::translate("Loading...");
        }
        return juce::translate("Analysing...");
    }
    if(std::get<2_z>(state) && includeRenderingState)
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

ANALYSE_FILE_END
