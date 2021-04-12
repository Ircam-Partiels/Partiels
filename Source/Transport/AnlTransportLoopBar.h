#pragma once

#include "AnlTransportModel.h"
#include "../Zoom/AnlZoomModel.h"

ANALYSE_FILE_BEGIN

namespace Transport
{
    class LoopBar
    : public juce::Component
    {
    public:
        enum ColourIds : int
        {
              backgroundColourId = 0x2020000
            , thumbCoulourId
        };
        
        LoopBar(Accessor& accessor, Zoom::Accessor& zoomAcsr);
        ~LoopBar() override;
        
        // juce::Component
        void paint(juce::Graphics& g) override;
        void mouseMove(juce::MouseEvent const& event) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseUp(juce::MouseEvent const& event) override;
        
    private:
        enum class EditMode
        {
              none
            , select
            , drag
            , resizeLeft
            , resizeRight
        };
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        Zoom::Accessor& mZoomAccessor;
        Zoom::Accessor::Listener mZoomListener;
        juce::Range<double> mLoopRange;
        double mCickTime = 0.0;
        juce::Range<double> mSavedRange;
        EditMode mEditMode = EditMode::none;
    };
}

ANALYSE_FILE_END
