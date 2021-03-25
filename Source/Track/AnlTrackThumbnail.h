#pragma once

#include "AnlTrackPropertyPanel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Thumbnail
    : public juce::Component
    {
    public:
        enum ColourIds : int
        {
              textColourId = 0x2030200
        };
        
        Thumbnail(Accessor& accessor);
        ~Thumbnail() override;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void lookAndFeelChanged() override;
        void parentHierarchyChanged() override;
        void mouseDrag(juce::MouseEvent const& event) override;
        
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
