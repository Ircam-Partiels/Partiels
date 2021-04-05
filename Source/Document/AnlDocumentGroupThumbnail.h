#pragma once

#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class GroupThumbnail
    : public juce::Component
    {
    public:
        enum ColourIds : int
        {
              textColourId = 0x2040100
        };
        
        GroupThumbnail(Accessor& accessor);
        ~GroupThumbnail() override;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void lookAndFeelChanged() override;
        void parentHierarchyChanged() override;
        
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        juce::ImageButton mExportButton;
        juce::ImageButton mExpandButton;
    };
}

ANALYSE_FILE_END
