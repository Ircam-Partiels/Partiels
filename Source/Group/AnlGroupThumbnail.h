#pragma once

#include "AnlGroupButtonState.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class Thumbnail
    : public juce::Component
    , public juce::SettableTooltipClient
    {
    public:
        enum ColourIds : int
        {
              textColourId = 0x2040100
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
        
        std::function<void(void)> onRemove = nullptr;
        
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        juce::ImageButton mDropdownButton;
        juce::ImageButton mNameButton;
        juce::ImageButton mExportButton;
        StateButton mStateButton {mAccessor};
        juce::ImageButton mExpandButton;
        juce::ImageButton mRemoveButton;
    };
}

ANALYSE_FILE_END
