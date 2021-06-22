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
        Grid(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Zoom::Grid::Justification const justification);
        ~Grid() override;

        // juce::Component
        void paint(juce::Graphics& g) override;

    private:
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Zoom::Grid::Justification mJustification;
        Accessor::Listener mListener{typeid(*this).name()};
        Zoom::Accessor::Listener mZoomListener{typeid(*this).name()};
        Zoom::Grid::Accessor::Listener mGridListener{typeid(*this).name()};
    };
} // namespace Track

ANALYSE_FILE_END
