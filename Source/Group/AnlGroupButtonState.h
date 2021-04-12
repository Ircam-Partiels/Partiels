#pragma once

#include "AnlGroupTrackManager.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class StateButton
    : public juce::Component
    , public juce::SettableTooltipClient
    , private TrackManager<std::reference_wrapper<Track::Accessor>>
    {
    public:
        StateButton(Accessor& groupAccessor);
        ~StateButton() override;
        
        // juce::Component
        void resized() override;
        
    private:
        // TrackManager<std::reference_wrapper<Track::Accessor>>
        void removeFromContents(std::reference_wrapper<Track::Accessor>& content) override;
        std::reference_wrapper<Track::Accessor> createForContents(Track::Accessor& trackAccessor) override;
        
        Track::Accessor::Listener mTrackListener;
        LoadingCircle mProcessingButton;
    };
}

ANALYSE_FILE_END
