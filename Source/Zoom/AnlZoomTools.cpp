#include "AnlZoomTools.h"

ANALYSE_FILE_BEGIN

Zoom::Range Zoom::Tools::getScaledVisibleRange(Accessor const& acsr, Range const& newGlobalRange)
{
    auto const globalRange = acsr.getAttr<AttrType::globalRange>();
    if(globalRange.isEmpty())
    {
        return newGlobalRange;
    }
    auto const visiblelRange = acsr.getAttr<AttrType::visibleRange>();
    auto const startRatio = (visiblelRange.getStart() - globalRange.getStart()) / globalRange.getLength();
    auto const endRatio = (visiblelRange.getEnd() - globalRange.getStart()) / globalRange.getLength();
    auto const newStart = startRatio * newGlobalRange.getLength() + newGlobalRange.getStart();
    auto const newEnd = endRatio * newGlobalRange.getLength() + newGlobalRange.getStart();
    return {newStart, newEnd};
}

double Zoom::Tools::getScaledValueFromWidth(Accessor const& zoomAcsr, juce::Component const& component, int x)
{
    auto const range = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const width = component.getWidth();
    if(width <= 0 || range.isEmpty())
    {
        return 0.0;
    }
    return static_cast<double>(x) / static_cast<double>(width) * range.getLength() + range.getStart();
}

double Zoom::Tools::getScaledValueFromHeight(Accessor const& zoomAcsr, juce::Component const& component, int y)
{
    auto const range = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const height = component.getHeight();
    if(height <= 0 || range.isEmpty())
    {
        return 0.0;
    }
    return static_cast<double>((height - 1) - y) / static_cast<double>(height) * range.getLength() + range.getStart();
}

int Zoom::Tools::getScaledXFromValue(Accessor const& zoomAcsr, juce::Component const& component, double value)
{
    auto const range = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const width = component.getWidth();
    if(width <= 0 || range.isEmpty())
    {
        return 0;
    }
    return static_cast<int>(std::round((value - range.getStart()) / range.getLength() * static_cast<double>(width)));
}

int Zoom::Tools::getScaledYFromValue(Accessor const& zoomAcsr, juce::Component const& component, double value)
{
    auto const range = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const height = component.getHeight();
    if(height <= 0 || range.isEmpty())
    {
        return 0;
    }
    return (height - 1) - static_cast<int>(std::round((value - range.getStart()) / range.getLength() * static_cast<double>(height)));
}

ANALYSE_FILE_END
