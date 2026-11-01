#include "MiscScalableImage.h"

MISC_FILE_BEGIN

ScalableImage::ScalableImage(juce::Image const& image)
: mImage(image)
{
}

ScalableImage::ScalableImage(std::unique_ptr<juce::Drawable> drawable)
: mDrawable(std::move(drawable))
{
}

ScalableImage ScalableImage::fromSvg(void const* data, int size)
{
    auto drawable = juce::Drawable::createFromImageData(data, static_cast<size_t>(size));
    MiscWeakAssert(drawable != nullptr);
    return {std::move(drawable)};
}

bool ScalableImage::isValid() const
{
    return mImage.isValid() || mDrawable != nullptr;
}

juce::Image ScalableImage::getImage(int width, int height)
{
    if(mDrawable == nullptr)
    {
        return mImage;
    }
    auto const bounds = mDrawable->getDrawableBounds();
    if(bounds.isEmpty() || width <= 0 || height <= 0)
    {
        return {};
    }
    auto const ratio = std::min(static_cast<float>(width) / bounds.getWidth(), static_cast<float>(height) / bounds.getHeight());
    auto const imageWidth = std::max(juce::roundToInt(bounds.getWidth() * ratio), 1);
    auto const imageHeight = std::max(juce::roundToInt(bounds.getHeight() * ratio), 1);
    if(mCachedImage.isValid() && mCachedImage.getWidth() == imageWidth && mCachedImage.getHeight() == imageHeight)
    {
        return mCachedImage;
    }
    mCachedImage = juce::Image(juce::Image::ARGB, imageWidth, imageHeight, true);
    juce::Graphics g(mCachedImage);
    mDrawable->drawWithin(g, mCachedImage.getBounds().toFloat(), juce::RectanglePlacement::stretchToFit, 1.0f);
    return mCachedImage;
}

bool ScalableImage::operator==(ScalableImage const& other) const
{
    return mImage == other.mImage && mDrawable == other.mDrawable;
}

bool ScalableImage::operator!=(ScalableImage const& other) const
{
    return !(*this == other);
}

MISC_FILE_END
