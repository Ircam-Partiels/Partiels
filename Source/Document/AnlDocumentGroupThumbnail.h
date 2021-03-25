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
              textColourId = 0x2040200
        };
        
        GroupThumbnail(Accessor& accessor);
        ~GroupThumbnail() override = default;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void lookAndFeelChanged() override;
        void parentHierarchyChanged() override;
        
    private:
        Accessor& mAccessor;
        
        juce::ImageButton mPropertiesButton;
        juce::ImageButton mExportButton;
        LoadingCircle mProcessingButton;
        juce::ImageButton mRemoveButton;
    };
}

ANALYSE_FILE_END
