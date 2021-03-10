#pragma once

#include "AnlZoomModel.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    class Playhead
    : public juce::Component
    {
    public:
        
        enum ColourIds : int
        {
              playheadColourId = 0x2010000
        };
        
        Playhead(Accessor& accessor, juce::BorderSize<int> const& borderSize = {});
        ~Playhead() override;
        
        void setBorderSize(juce::BorderSize<int> const& borderSize);
        void setPosition(double position);
        
        // juce::Component
        void paint(juce::Graphics& g) override;
        void parentSizeChanged() override;
        void colourChanged() override;
    private:
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        juce::BorderSize<int> mBorderSize {};
        double mPosition = 0.0;
    };
}

ANALYSE_FILE_END
