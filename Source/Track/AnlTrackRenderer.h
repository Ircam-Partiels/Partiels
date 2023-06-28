#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    namespace Renderer
    {
        void paintChannels(Accessor const& acsr, juce::Graphics& g, juce::Rectangle<int> const& bounds, juce::Colour const& separatorColour, std::function<void(juce::Rectangle<int>, size_t)> fn);
        void paintClippedImage(juce::Graphics& g, juce::Image const& image, juce::Rectangle<float> const& bounds);
        void paint(Accessor const& accessor, Zoom::Accessor const& timeZoomAcsr, juce::Graphics& g, juce::Rectangle<int> const& bounds, juce::Colour const colour);
    } // namespace Renderer
} // namespace Track

ANALYSE_FILE_END
