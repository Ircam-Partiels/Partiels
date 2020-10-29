#pragma once

#include "AnlApplicationCommandTarget.h"
#include "../Analyzer/AnlAnalyzerProcessor.h"
#include "../Document/AnlDocumentModel.h"
#include "../Document/AnlDocumentTransport.h"
#include "../Document/AnlDocumentFileInfoPanel.h"
#include "../Document/AnlDocumentAnalyzerPanel.h"

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
        Document::AnalyzerPanel mDocumentAnalyzerPanel;
  
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Interface)
    };
}

ANALYSE_FILE_END

