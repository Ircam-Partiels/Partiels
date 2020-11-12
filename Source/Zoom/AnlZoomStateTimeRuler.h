#pragma once

#include "AnlZoomStateRuler.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    class TimeRuler
    : public juce::Component
    {
    public:
        enum ColourIds
        {
            backgroundColourId = 0x2002000
        };
        
        JUCE_DEPRECATED(TimeRuler(Accessor& accessor));
        ~TimeRuler() override = default;
        
        // juce::Component
        void paint(juce::Graphics &g) override;
        void resized() override;
        
        std::function<void()> onDoubleClick = nullptr;
        
    private:
        
        Ruler mPrimaryRuler;
        Ruler mSecondaryRuler;
    };
}

ANALYSE_FILE_END
