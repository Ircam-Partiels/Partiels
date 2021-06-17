#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class LowResCachedComponentImage
: public juce::CachedComponentImage
{
public:
    LowResCachedComponentImage(juce::Component& owner) noexcept;
    ~LowResCachedComponentImage() override = default;
    
    // juce::CachedComponentImage
    void paint(juce::Graphics& g) override;
    bool invalidateAll() override;
    bool invalidate(juce::Rectangle<int> const& area) override;
    void releaseResources() override;
    
private:
    juce::Component& mOwner;
    juce::Image mImage;
    juce::RectangleList<int> mValidArea;
    float mScale = 2.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LowResCachedComponentImage)
};

ANALYSE_FILE_END
