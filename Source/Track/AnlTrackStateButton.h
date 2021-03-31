#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class StateButton
    : public juce::Component
    {
    public:
        StateButton(Accessor& accessor);
        ~StateButton() override;
        
        // juce::Component
        void resized() override;
        
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        LoadingCircle mProcessingButton;
    };
}

ANALYSE_FILE_END
