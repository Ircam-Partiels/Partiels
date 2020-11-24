#pragma once

#include "AnlDocumentModel.h"
#include "../Tools/AnlColouredPanel.h"
#include "../Zoom/AnlZoomRuler.h"
#include "../Zoom/AnlZoomScrollBar.h"
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
            Section(Analyzer::Accessor& acsr, Zoom::Accessor& zoomAcsr)
            : accessor(acsr)
            , renderer(acsr, zoomAcsr)
            {
            }
            Analyzer::Accessor& accessor;
            Analyzer::ResultRenderer renderer;
            Tools::ColouredPanel separator;
            Zoom::Ruler ruler {accessor.getAccessor<Analyzer::AttrType::zoom>(0), Zoom::Ruler::Orientation::vertical};
            Zoom::ScrollBar scrollbar {accessor.getAccessor<Analyzer::AttrType::zoom>(0), Zoom::ScrollBar::Orientation::vertical, true};
            Tools::ColouredPanel zoomSeparator;
        };
        
        std::vector<std::unique_ptr<Section>> mSections;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainPanel)
    };
}

ANALYSE_FILE_END
