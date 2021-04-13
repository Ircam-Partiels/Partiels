#pragma once

#include "AnlGroupButtonState.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class Thumbnail
    : public juce::Component
    {
    public:
        enum ColourIds : int
        {
              textColourId = 0x2040000
        };
        
        Thumbnail(Accessor& accessor);
        ~Thumbnail() override;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void lookAndFeelChanged() override;
        void parentHierarchyChanged() override;
        void mouseDrag(juce::MouseEvent const& event) override;
        
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        juce::ImageButton mExportButton;
        StateButton mStateButton {mAccessor};
        juce::ImageButton mExpandButton;
    };
}

ANALYSE_FILE_END
