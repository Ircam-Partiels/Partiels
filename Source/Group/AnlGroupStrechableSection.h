#pragma once

#include "../Track/AnlTrackSection.h"
#include "AnlGroupSection.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class StrechableSection
    : public juce::Component
    , private juce::ChangeListener
    {
    public:
        StrechableSection(Director& director, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr);
        ~StrechableSection() override;

        bool isResizing() const;
        juce::Component const& getSection(juce::String const& identifier) const;
        juce::Rectangle<int> getPlotBounds(juce::String const& identifier) const;
        void setResizable(bool state);

        std::function<void(void)> onResizingStarted = nullptr;
        std::function<void(void)> onResizingEnded = nullptr;
        std::function<void(juce::String const& identifier, size_t index, bool copy)> onTrackInserted = nullptr;

        // juce::Component
        void resized() override;

    private:
        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;

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
        ComponentListener mComponentListener;
        bool mIsResizable{true};

        LayoutNotifier mLayoutNotifier;
    };
} // namespace Group

ANALYSE_FILE_END
