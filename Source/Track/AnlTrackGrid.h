#pragma once

#include "../Zoom/AnlZoomGrid.h"
#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Grid
    : public juce::Component
    {
    public:
        Grid(Accessor& accessor, juce::Justification const justification);
        ~Grid() override;

        // juce::Component
        void paint(juce::Graphics& g) override;

    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        Zoom::Accessor::Listener mZoomListener;
        Zoom::Grid::Accessor::Listener mGridListener;
        juce::Justification mJustification;
    };
} // namespace Track

ANALYSE_FILE_END
