#pragma once

#include "AnlAnalyzerPropertyPanel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class ResultRenderer
    : public juce::Component
    {
    public:        
        ResultRenderer(Accessor& accessor);
        ~ResultRenderer() override;
        
        // juce::Component
        void resized() override;
        
    private:
        Accessor& mAccessor;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResultRenderer)
    };
}

ANALYSE_FILE_END
