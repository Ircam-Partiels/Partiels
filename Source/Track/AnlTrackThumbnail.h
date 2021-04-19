#pragma once

#include "AnlTrackPropertyPanel.h"
#include "AnlTrackStateButton.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Thumbnail
    : public juce::Component
    , public juce::SettableTooltipClient
    , private juce::Timer
    {
    public:
        enum ColourIds : int
        {
              textColourId = 0x2030100
            , titleBackgroundColourId
        };
        
        Thumbnail(Accessor& accessor);
        ~Thumbnail() override;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void lookAndFeelChanged() override;
        void parentHierarchyChanged() override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseUp(juce::MouseEvent const& event) override;
        
        std::function<void(void)> onRemove = nullptr;
        
    private:
        void timerCallback() override;
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        PropertyPanel mPropertyPanel {mAccessor};
        juce::ImageButton mDropdownButton;
        juce::ImageButton mPropertiesButton;
        juce::ImageButton mExportButton;
        StateButton mStateButton {mAccessor};
        juce::ImageButton mRemoveButton;
    };
}

ANALYSE_FILE_END
