#pragma once

#include "AnlApplicationCommandTarget.h"
#include "../Tools/AnlTooltip.h"
#include "../Analyzer/AnlAnalyzerProcessor.h"
#include "../Document/AnlDocumentModel.h"
#include "../Document/AnlDocumentTransport.h"
#include "../Document/AnlDocumentFileInfoPanel.h"
#include "../Document/AnlDocumentControlPanel.h"
#include "../Zoom/AnlZoomTimeRuler.h"
#include "../Zoom/AnlZoomScrollBar.h"

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
        Zoom::TimeRuler mZoomStateTimeRuler;
        Tools::ColouredPanel mZoomStateTimeRulerSeparator;
        Document::ControlPanel mDocumentControlPanel;
        Tools::ColouredPanel mDocumentControlPanelSeparator;
        
        //Footer
        Tools::ColouredPanel mBottomSeparator;
        Zoom::ScrollBar mTimeScrollBar;
        Tools::ColouredPanel mTooTipSeparator;
        Tooltip::Display mToolTipDisplay;
  
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Interface)
    };
}

ANALYSE_FILE_END

