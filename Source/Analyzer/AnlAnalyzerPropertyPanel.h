#pragma once

#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Thumbnail
    : public juce::Component
    {
    public:
        using Attribute = Model::Attribute;
        
        Thumbnail(Accessor& accessor);
        ~Thumbnail() override;
        
        // juce::Component
        void resized() override;
        
        std::function<void(void)> onRemove = nullptr;
        
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        juce::Label mNameLabel;
        juce::TextButton mRemoveButton {juce::CharPointer_UTF8("×")};
        juce::TextButton mPropertiesButton {juce::CharPointer_UTF8("Ρ")};
        juce::TextButton mRelaunchButton {juce::CharPointer_UTF8("μ")};
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Thumbnail)
    };
}

ANALYSE_FILE_END
