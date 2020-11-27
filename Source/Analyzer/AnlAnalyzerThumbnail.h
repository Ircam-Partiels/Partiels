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
        };
        
        Thumbnail(Accessor& accessor);
        ~Thumbnail() override;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
        std::function<void(void)> onRemove = nullptr;
        
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        PropertyPanel mPropertyPanel {mAccessor};
        
        juce::Label mNameLabel;
        juce::TextButton mRemoveButton {juce::CharPointer_UTF8("×")};
        juce::TextButton mPropertiesButton {juce::CharPointer_UTF8("φ")};
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Thumbnail)
    };
}

ANALYSE_FILE_END
