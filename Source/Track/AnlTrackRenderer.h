#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    namespace Renderer
    {
        void paintChannels(juce::Graphics& g, juce::Rectangle<int> const& bounds, std::vector<bool> const& channels, juce::Colour const& separatorColour, std::function<void(juce::Rectangle<int>, size_t)> fn);
        void paintClippedImage(juce::Graphics& g, juce::Image const& image, juce::Rectangle<float> const& bounds);
        void paint(Accessor const& accessor, Zoom::Accessor const& timeZoomAcsr, juce::Graphics& g, juce::Rectangle<int> const& bounds, std::vector<bool> const& channels, juce::Colour const colour, Zoom::Grid::Justification outsideGridjustification);
        juce::BorderSize<int> getOutsideGridBorder(Zoom::Grid::Justification outsideGridjustification);

        //! @brief Gets the function that converts the values of the vertical zoom of a track to grid labels
        //! @param accessor The accessor of the track (it must outlive the returned function)
        //! @return The function or nullptr if the track has no vertical zoom
        std::function<juce::String(double)> getGridStringify(Accessor const& accessor);
    } // namespace Renderer
} // namespace Track

ANALYSE_FILE_END
