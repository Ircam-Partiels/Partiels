#include "AnlLowResCachedImageComponent.h"

ANALYSE_FILE_BEGIN

LowResCachedComponentImage::LowResCachedComponentImage(juce::Component& owner) noexcept
: mOwner(owner)
{
}

void LowResCachedComponentImage::paint(juce::Graphics& g)
{
    mScale = g.getInternalContext().getPhysicalPixelScaleFactor();
    auto const compBounds = mOwner.getLocalBounds();
    auto const imageBounds = compBounds * mScale;
    
    if(mImage.isNull() || mImage.getBounds() != imageBounds)
    {
        mImage = juce::Image(mOwner.isOpaque() ? juce::Image::RGB : juce::Image::ARGB, std::max(1, imageBounds.getWidth()), std::max(1, imageBounds.getHeight()), !mOwner.isOpaque());
        mValidArea.clear();
    }
    
    if(!mValidArea.containsRectangle(compBounds))
    {
        juce::Graphics imG (mImage);
        auto& lg = imG.getInternalContext();
        
        lg.addTransform(juce::AffineTransform::scale(mScale));
        
        for(auto const& i : mValidArea)
        {
            lg.excludeClipRectangle(i);
        }
        
        if(!mOwner.isOpaque())
        {
            lg.setFill(juce::Colours::transparentBlack);
            lg.fillRect(compBounds, true);
            lg.setFill(juce::Colours::black);
        }
        
        mOwner.paintEntireComponent(imG, true);
    }
    
    mValidArea = compBounds;
    
    g.setColour(juce::Colours::black.withAlpha(mOwner.getAlpha()));
    g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
    g.drawImageTransformed(mImage, juce::AffineTransform::scale(static_cast<float>(compBounds.getWidth())  / static_cast<float>(imageBounds.getWidth()), static_cast<float>(compBounds.getHeight()) / static_cast<float>(imageBounds.getHeight())), false);
}

bool LowResCachedComponentImage::invalidateAll()
{
    mValidArea.clear();
    return true;
}

bool LowResCachedComponentImage::invalidate(juce::Rectangle<int> const& area)
{
    mValidArea.subtract(area);
    return true;
}

void LowResCachedComponentImage::releaseResources()
{
    mImage = juce::Image();
}

ANALYSE_FILE_END
