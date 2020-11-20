#pragma once

#include "AnlApplicationCommandTarget.h"
#include "../Tools/AnlTooltip.h"
#include "../Analyzer/AnlAnalyzerProcessor.h"
#include "../Document/AnlDocumentModel.h"
#include "../Document/AnlDocumentTransport.h"
#include "../Document/AnlDocumentFileInfoPanel.h"
#include "../Document/AnlDocumentControlPanel.h"
#include "../Document/AnlDocumentMainPanel.h"
#include "../Zoom/AnlZoomScrollBar.h"
#include "../Zoom/AnlZoomRuler.h"

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
        Zoom::Ruler mZoomTimeRuler;
        Tools::ColouredPanel mZoomTimeRulerSeparator;
        Document::ControlPanel mDocumentControlPanel;
        Tools::ColouredPanel mDocumentControlPanelSeparator;
        Document::MainPanel mDocumentMainPanel;
        
        //Footer
        Tools::ColouredPanel mBottomSeparator;
        Zoom::ScrollBar mTimeScrollBar;
        Tools::ColouredPanel mTooTipSeparator;
        Tooltip::Display mToolTipDisplay;
  
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Interface)
    };
}

ANALYSE_FILE_END

