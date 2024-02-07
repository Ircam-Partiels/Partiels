#pragma once

#include "AnlTrackDirector.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Plot
    : public juce::Component
    {
    public:
        Plot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor);
        ~Plot() override;

        // juce::Component
        void paint(juce::Graphics& g) override;

    private:
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        Zoom::Accessor::Listener mZoomListener{typeid(*this).name()};
        Zoom::Grid::Accessor::Listener mGridListener{typeid(*this).name()};
    };
} // namespace Track

ANALYSE_FILE_END
