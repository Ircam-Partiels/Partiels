#pragma once

#include "../Track/AnlTrackSection.h"
#include "AnlGroupSection.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class StrechableSection
    : public juce::Component
    {
    public:
        StrechableSection(Director& director, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr);
        ~StrechableSection() override;

        void moveKeyboardFocusTo(juce::String const& identifier);

        std::function<void(juce::String const& identifier)> onTrackInserted = nullptr;

        // juce::Component
        void resized() override;
        juce::KeyboardFocusTraverser* createFocusTraverser() override;

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Transport::Accessor& mTransportAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Accessor::Listener mListener;

        Section mSection{mDirector, mTransportAccessor, mTimeZoomAccessor};
        TrackMap<std::unique_ptr<Track::Section>> mTrackSections;
        DraggableTable mDraggableTable{"Track"};
        ConcertinaTable mConcertinaTable{"", false};
        BoundsListener mBoundsListener;
    };
} // namespace Group

ANALYSE_FILE_END
