#pragma once

#include "AnlGroupSection.h"
#include "../Track/AnlTrackSection.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class StrechableSection
    : public juce::Component
    , private TrackManager<std::unique_ptr<Track::Section>>
    {
    public:
        StrechableSection(Accessor& accessor, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr);
        ~StrechableSection() override;
        
        std::function<void(juce::String const& identifier)> onRemoveTrack = nullptr;
        
        // juce::Component
        void resized() override;
        
    private:
        // TrackManager<std::unique_ptr<Track::Section>>
        void updateContentsEnded() override;
        void removeFromContents(std::unique_ptr<Track::Section>& content) override;
        std::unique_ptr<Track::Section> createForContents(Track::Accessor& trackAccessor) override;
        
        Accessor& mAccessor;
        Transport::Accessor& mTransportAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Accessor::Listener mListener;
        
        Section mSection {mAccessor, mTransportAccessor, mTimeZoomAccessor};
        DraggableTable mDraggableTable;
        ConcertinaTable mConcertinaTable {"", false};
        BoundsListener mBoundsListener;
    };
}

ANALYSE_FILE_END
