#pragma once

#include "AnlApplicationCommandTarget.h"
#include "../Tools/AnlToolTip.h"
#include "../Analyzer/AnlAnalyzerProcessor.h"
#include "../Document/AnlDocumentModel.h"
#include "../Document/AnlDocumentTransport.h"
#include "../Document/AnlDocumentFileInfoPanel.h"
#include "../Document/AnlDocumentAnalyzerPanel.h"
#include "../Zoom/AnlZoomStateTimeRuler.h"
#include "../Zoom/AnlZoomStateSlider.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Interface
    : public juce::Component
    , public CommandTarget
    {
    public:
        
        Interface();
        ~Interface() override;
        
        // juce::Component
        void resized() override;

    private:
        
        Document::Accessor::Listener mDocumentListener;
        
        // Header
        Document::Transport mDocumentTransport;
        Tools::ColouredPanel mDocumentTransportSeparator;
        Document::FileInfoPanel mDocumentFileInfoPanel;
        Tools::ColouredPanel mHeaderSeparator;
        
        // Main
        Zoom::State::TimeRuler mZoomStateTimeRuler;
        Tools::ColouredPanel mMainSeparator;
        Document::AnalyzerPanel mDocumentAnalyzerPanel;
        
        //Footer
        Tools::ColouredPanel mBottomSeparator;
        Zoom::State::Slider mTimeSlider;
        Tools::ColouredPanel mTooTipSeparator;
        ToolTipDisplay mToolTipDisplay;
  
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Interface)
    };
}

ANALYSE_FILE_END

