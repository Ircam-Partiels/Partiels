#include "MiscZoomTools.h"

MISC_FILE_BEGIN

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

double Zoom::Tools::getScaledXFromValue(Accessor const& zoomAcsr, juce::Component const& component, double value)
{
    auto const range = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const width = component.getWidth();
    if(width <= 0 || range.isEmpty())
    {
        return 0;
    }
    return (value - range.getStart()) / range.getLength() * static_cast<double>(width);
}

double Zoom::Tools::getScaledYFromValue(Accessor const& zoomAcsr, juce::Component const& component, double value)
{
    auto const range = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const height = component.getHeight();
    if(height <= 0 || range.isEmpty())
    {
        return 0;
    }
    return static_cast<double>(height - 1) - ((value - range.getStart()) / range.getLength() * static_cast<double>(height));
}

double Zoom::Tools::getNearestValue(Accessor const& zoomAcsr, double value, std::set<double> const& markers)
{
    auto const range = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
    if(markers.empty())
    {
        return range.clipValue(value);
    }
    if(value <= *markers.cbegin())
    {
        auto const next = range.clipValue(*markers.cbegin());
        auto const previous = range.getStart();
        return std::abs(next - value) <= std::abs(previous - value) ? next : previous;
    }
    if(value > *markers.crbegin())
    {
        auto const previous = range.clipValue(*markers.crbegin());
        auto const next = range.getEnd();
        return std::abs(next - value) <= std::abs(previous - value) ? next : previous;
    }
    auto const nextIt = std::lower_bound(markers.cbegin(), markers.cend(), value);
    auto const previousIt = std::prev(nextIt);
    auto const next = range.clipValue(*nextIt);
    auto const previous = range.clipValue(*previousIt);
    return std::abs(next - value) <= std::abs(previous - value) ? next : previous;
}

juce::Range<double> Zoom::Tools::getNearestRange(Accessor const& zoomAcsr, double value, std::set<double> const& markers)
{
    auto const range = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
    if(markers.empty())
    {
        return range;
    }
    if(value <= *markers.cbegin())
    {
        auto const next = range.clipValue(*markers.cbegin());
        auto const previous = range.getStart();
        return {previous, next};
    }
    if(value > *markers.crbegin())
    {
        auto const previous = range.clipValue(*markers.crbegin());
        auto const next = range.getEnd();
        return {previous, next};
    }
    auto const nextIt = std::lower_bound(markers.cbegin(), markers.cend(), value);
    auto const previousIt = std::prev(nextIt);
    auto const next = range.clipValue(*nextIt);
    auto const previous = range.clipValue(*previousIt);
    return {previous, next};
}

bool Zoom::Tools::canZoomIn(Accessor const& accessor)
{
    return accessor.getAttr<Zoom::AttrType::visibleRange>().getLength() > accessor.getAttr<Zoom::AttrType::minimumLength>();
}

bool Zoom::Tools::canZoomOut(Accessor const& accessor)
{
    return accessor.getAttr<Zoom::AttrType::visibleRange>().getLength() < accessor.getAttr<Zoom::AttrType::globalRange>().getLength();
}

void Zoom::Tools::zoomIn(Accessor& accessor, double ratio, NotificationType notification)
{
    auto const& visibleRange = accessor.getAttr<AttrType::visibleRange>();
    auto const& globalRange = accessor.getAttr<AttrType::globalRange>();
    accessor.setAttr<Zoom::AttrType::visibleRange>(visibleRange.expanded(-globalRange.getLength() * ratio), notification);
}

void Zoom::Tools::zoomOut(Accessor& accessor, double ratio, NotificationType notification)
{
    zoomIn(accessor, -ratio, notification);
}

void Zoom::Tools::zoomIn(Accessor& accessor, double ratio, double anchor, NotificationType notification)
{
    auto const& visibleRange = accessor.getAttr<AttrType::visibleRange>();
    auto const amountStart = (anchor - visibleRange.getStart()) * (1.0 - ratio);
    auto const amountEnd = (visibleRange.getEnd() - anchor) * (1.0 - ratio);
    accessor.setAttr<AttrType::visibleRange>(Range(anchor - amountStart, anchor + amountEnd), notification);
}

void Zoom::Tools::zoomOut(Accessor& accessor, double ratio, double anchor, NotificationType notification)
{
    zoomIn(accessor, -ratio, anchor, notification);
}

MISC_FILE_END
