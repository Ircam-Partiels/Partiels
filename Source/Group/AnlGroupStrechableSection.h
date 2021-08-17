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
        juce::Rectangle<int> getPlotBounds(juce::String const& identifier) const;

        std::function<void(juce::String const& identifier, size_t index, bool copy)> onTrackInserted = nullptr;

        // juce::Component
        void resized() override;
        std::unique_ptr<juce::ComponentTraverser> createKeyboardFocusTraverser() override;

    private:
        void updateContent();

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Transport::Accessor& mTransportAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Accessor::Listener mListener{typeid(*this).name()};

        Section mSection{mDirector, mTransportAccessor, mTimeZoomAccessor};
        TrackMap<std::unique_ptr<Track::Section>> mTrackSections;
        DraggableTable mDraggableTable{"Track"};
        ConcertinaTable mConcertinaTable{"", false};
        BoundsListener mBoundsListener;

        LayoutNotifier mLayoutNotifier;
    };
} // namespace Group

ANALYSE_FILE_END
