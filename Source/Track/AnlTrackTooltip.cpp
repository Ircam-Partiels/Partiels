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

juce::String Track::Tools::getZoomTootip(Accessor const& acsr, juce::Component const& component, int y)
{
    switch(getFrameType(acsr))
    {
        case FrameType::label:
            return {};
        case FrameType::value:
            return Format::valueToString(Zoom::Tools::getScaledValueFromHeight(acsr.getAcsr<AcsrType::valueZoom>(), component, y), 4) + getUnit(acsr);
        case FrameType::vector:
            auto const value = Zoom::Tools::getScaledValueFromHeight(acsr.getAcsr<AcsrType::binZoom>().getAttr<Zoom::AttrType::visibleRange>(), component, y);
            auto const bin = static_cast<size_t>(std::floor(value));
            auto const& output = acsr.getAttr<AttrType::description>().output;
            if(bin >= output.binNames.size() || output.binNames.at(bin).empty())
            {
                return juce::translate("Bin INDEX").replace("INDEX", juce::String(bin));
            }
            else
            {
                return juce::translate("Bin INDEX").replace("INDEX", juce::String(bin)) + " - " + output.binNames.at(bin);
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
    auto const getExtraTooltip = [&](std::vector<float> const& extraValues)
    {
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
    };

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
        getExtraTooltip(std::get<3_z>(*it));
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
            getExtraTooltip(std::get<3_z>(*it));
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
        auto it = std::upper_bound(channelResults.cbegin(), channelResults.cend(), time, Result::upper_cmp<Result::Data::Column>);
        if(it == channelResults.cbegin())
        {
            lines.add(name + ": -");
            return lines;
        }
        it = std::prev(it);
        auto const value = Zoom::Tools::getScaledValueFromHeight(accessor.getAcsr<AcsrType::binZoom>().getAttr<Zoom::AttrType::visibleRange>(), component, y);
        auto const binIndex = static_cast<size_t>(std::floor(value));
        auto const& column = std::get<2_z>(*it);
        if(binIndex >= column.size())
        {
            lines.add(name + ": -");
            return lines;
        }
        lines.add(name + ": " + Format::valueToString(column.at(binIndex), 4) + unit);
        getExtraTooltip(std::get<3_z>(*it));
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
