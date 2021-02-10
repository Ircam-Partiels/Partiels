#pragma once

#include "AnlAnalyzerPropertyPanel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Thumbnail
    : public juce::Component
    {
    public:
        
        enum ColourIds : int
        {
              backgroundColourId = 0x2000330
            , borderColourId
            , textColourId
        };
        
        Thumbnail(Accessor& accessor);
        ~Thumbnail() override;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void lookAndFeelChanged() override;
        void parentHierarchyChanged() override;
        
        std::function<void(void)> onRemove = nullptr;
        
    private:
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        PropertyPanel mPropertyPanel {mAccessor};
        juce::ImageButton mPropertiesButton;
        juce::ImageButton mExportButton;
        LoadingCircle mProcessingButton;
        juce::ImageButton mRemoveButton;
    };
}

ANALYSE_FILE_END
