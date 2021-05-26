#pragma once

#include "AnlZoomModel.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    namespace Tools
    {
        Range getScaledVisibleRange(Accessor const& zoomAcsr, Range const& newGlobalRange);
        double getScaledValueFromWidth(Accessor const& zoomAcsr, juce::Component const& component, int x);
        double getScaledValueFromHeight(Accessor const& zoomAcsr, juce::Component const& component, int y);
        double getScaledXFromValue(Accessor const& zoomAcsr, juce::Component const& component, double value);
        double getScaledYFromValue(Accessor const& zoomAcsr, juce::Component const& component, double value);
    }
}

ANALYSE_FILE_END
