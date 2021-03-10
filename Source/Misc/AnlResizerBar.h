#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class ResizerBar
: public juce::Component
{
public:
    
    enum ColourIds : int
    {
          activeColourId = 0x2000500
        , inactiveColourId
    };
    
    enum Orientation
    {
          vertical
        , horizontal
    };
    
    ResizerBar(Orientation const orientation, juce::Range<int> const range = {});
    ~ResizerBar() override = default;
    
    void paint(juce::Graphics& g) override;
    void mouseDown(juce::MouseEvent const& event) override;
    void mouseDrag(juce::MouseEvent const& event) override;
    
    std::function<void(int)> onMoved = nullptr;
private:
    Orientation const mOrientation;
    juce::Range<int> const mRange;
    int mSavedPosition;
};

ANALYSE_FILE_END
