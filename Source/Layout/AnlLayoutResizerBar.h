
#pragma once

#include "../Tools/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Layout
{
    class ResizerBar :
    public juce::Component
    {
    public:
        
        enum ColourIds : int
        {
              activeColourId = 0x2005300
            , inactiveColourId = 0x2005301
        };
        
        enum Orientation
        {
              vertical
            , horizontal
        };
        
        ResizerBar(Orientation orientation);
        ~ResizerBar() override = default;
        
        void paint(juce::Graphics& g) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        
        std::function<void(int)> onMoved = nullptr;
    private:
        Orientation const mOrientation;
        int mSavedPosition;
    };
}

ANALYSE_FILE_END
