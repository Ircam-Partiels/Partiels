#pragma once

#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Playhead
    : public juce::Component
    {
    public:
        
        enum ColourIds : int
        {
              backgroundColourId = 0x2000410
            , playheadColourId = 0x2000411
        };
        
        Playhead(Accessor& accessor);
        ~Playhead() override;
        
        void paint(juce::Graphics& g) override;
    private:
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        Zoom::Accessor::Listener mZoomListener;
    };
}

ANALYSE_FILE_END
