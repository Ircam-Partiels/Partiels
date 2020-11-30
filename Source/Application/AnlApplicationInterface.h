#pragma once

#include "AnlApplicationCommandTarget.h"
#include "../Tools/AnlTooltip.h"
#include "../Analyzer/AnlAnalyzerProcessor.h"
#include "../Document/AnlDocumentModel.h"
#include "../Document/AnlDocumentTransport.h"
#include "../Document/AnlDocumentFileInfoPanel.h"
#include "../Document/AnlDocumentSection.h"
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
        Document::FileInfoPanel mDocumentFileInfoPanel;
        Document::Transport mDocumentTransport;
        
        // Main
        Document::Section mDocumentSection;
        
        //Footer
        Tooltip::Display mToolTipDisplay;
  
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Interface)
    };
}

ANALYSE_FILE_END

