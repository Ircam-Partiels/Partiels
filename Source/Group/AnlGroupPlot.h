#pragma once

#include "AnlGroupDirector.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class Plot
    : public juce::Component
    {
    public:
        Plot(Accessor& accessor, Zoom::Accessor& timeZoomAcsr);
        ~Plot() override;

        // juce::Component
        void paint(juce::Graphics& g) override;

    private:
        void updateContent();

        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        Zoom::Accessor& mTimeZoomAccessor;
        Zoom::Accessor::Listener mZoomListener{typeid(*this).name()};
        Zoom::Grid::Accessor::Listener mGridListener{typeid(*this).name()};
        Track::Accessor::Listener mTrackListener{typeid(*this).name()};
        TrackMap<std::reference_wrapper<Track::Accessor>> mTrackAccessors;
        LayoutNotifier mLayoutNotifier;
    };
} // namespace Group

ANALYSE_FILE_END
