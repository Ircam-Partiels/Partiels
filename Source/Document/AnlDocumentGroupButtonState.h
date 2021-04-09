#pragma once

#include "AnlDocumentModel.h"
#include "AnlDocumentGroupContainer.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class GroupStateButton
    : public juce::Component
    , public juce::SettableTooltipClient
    , private GroupContainer<std::reference_wrapper<Track::Accessor>>
    {
    public:
        GroupStateButton(Accessor& accessor);
        ~GroupStateButton() override;
        
        // juce::Component
        void resized() override;
        
    private:
        // GroupContainer<std::reference_wrapper<Track::Accessor>>
        void removeFromGroup(std::reference_wrapper<Track::Accessor>& value) override;
        std::reference_wrapper<Track::Accessor> createForGroup(Track::Accessor& trackAccessor) override;
        
        Track::Accessor::Listener mTrackListener;
        LoadingCircle mProcessingButton;
    };
}

ANALYSE_FILE_END
