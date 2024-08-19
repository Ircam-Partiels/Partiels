#pragma once

#include "MiscZoomModel.h"

MISC_FILE_BEGIN

namespace Zoom
{
    namespace Tools
    {
        Range getScaledVisibleRange(Accessor const& zoomAcsr, Range const& newGlobalRange);
        double getScaledValueFromWidth(Accessor const& zoomAcsr, juce::Component const& component, int x);
        double getScaledValueFromHeight(Accessor const& zoomAcsr, juce::Component const& component, int y);
        double getScaledXFromValue(Accessor const& zoomAcsr, juce::Component const& component, double value);
        double getScaledYFromValue(Accessor const& zoomAcsr, juce::Component const& component, double value);
        double getNearestValue(Accessor const& zoomAcsr, double value, std::set<double> const& markers);
        juce::Range<double> getNearestRange(Accessor const& accessor, double value, std::set<double> const& markers);
        bool canZoomIn(Accessor const& accessor);
        bool canZoomOut(Accessor const& accessor);
        void zoomIn(Accessor& accessor, double ratio, NotificationType notification);
        void zoomOut(Accessor& accessor, double ratio, NotificationType notification);
        void zoomIn(Accessor& accessor, double ratio, double anchor, NotificationType notification);
        void zoomOut(Accessor& accessor, double ratio, double anchor, NotificationType notification);
    } // namespace Tools
} // namespace Zoom

MISC_FILE_END
