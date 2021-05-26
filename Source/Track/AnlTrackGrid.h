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
        Grid(Accessor& accessor);
        ~Grid() override;

        // juce::Component
        void paint(juce::Graphics& g) override;

    private:
        Accessor& mAccessor;
        Zoom::Accessor::Listener mZoomListener;
        Accessor::Listener mListener;
    };
} // namespace Track

ANALYSE_FILE_END
