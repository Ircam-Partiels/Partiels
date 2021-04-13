#pragma once

#include "AnlGroupSection.h"
#include "../Track/AnlTrackSection.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class StrechableSection
    : public juce::Component
    {
    public:
        StrechableSection(Accessor& accessor, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr);
        ~StrechableSection() override;
        
        std::function<void(juce::String const& identifier)> onRemoveGroup = nullptr;
        std::function<void(juce::String const& identifier)> onRemoveTrack = nullptr;
        
        // juce::Component
        void resized() override;
        
    private:
        Accessor& mAccessor;
        Transport::Accessor& mTransportAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Accessor::Listener mListener;
        
        Section mSection {mAccessor, mTransportAccessor, mTimeZoomAccessor};
        TrackMap<std::unique_ptr<Track::Section>> mTrackSections;
        DraggableTable mDraggableTable;
        ConcertinaTable mConcertinaTable {"", false};
        BoundsListener mBoundsListener;
    };
}

ANALYSE_FILE_END
