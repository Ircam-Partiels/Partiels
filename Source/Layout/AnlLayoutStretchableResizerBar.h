
#pragma once

#include "../Tools/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Layout
{
    class StretchableResizerBar
    : public juce::StretchableLayoutResizerBar
    {
    public:
        using juce::StretchableLayoutResizerBar::StretchableLayoutResizerBar;
        
        // juce::StretchableLayoutResizerBar
        void hasBeenMoved() override;
        
        std::function<void(void)> onMoved = nullptr;
    private:
        
    };
}

ANALYSE_FILE_END
