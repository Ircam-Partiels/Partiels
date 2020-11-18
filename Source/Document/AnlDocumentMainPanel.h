#pragma once

#include "AnlDocumentModel.h"
#include "../Tools/AnlColouredPanel.h"
#include "../Analyzer/AnlAnalyzerResultRenderer.h"
#include "../Analyzer/AnlAnalyzerProcessor.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class MainPanel
    : public juce::Component
    {
    public:
        MainPanel(Accessor& accessor);
        ~MainPanel() override;
        
        void resized() override;
    private:
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        struct Section
        {
            Section(Analyzer::Accessor& acsr)
            : accessor(acsr)
            {
            }
            Analyzer::Accessor& accessor;
            Analyzer::ResultRenderer renderer {accessor};
            Tools::ColouredPanel separator;
        };
        
        std::vector<std::unique_ptr<Section>> mSections;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainPanel)
    };
}

ANALYSE_FILE_END
