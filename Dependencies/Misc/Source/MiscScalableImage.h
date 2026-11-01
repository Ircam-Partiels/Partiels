#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

//! @brief An image defined by a bitmap or by a vector graphic
//! @details The vector graphic is rendered at the required size and the result is cached.
class ScalableImage
{
public:
    ScalableImage() = default;
    ScalableImage(juce::Image const& image);
    ScalableImage(std::unique_ptr<juce::Drawable> drawable);
    ScalableImage(ScalableImage const&) = default;
    ScalableImage(ScalableImage&&) = default;
    ScalableImage& operator=(ScalableImage const&) = default;
    ScalableImage& operator=(ScalableImage&&) = default;
    ~ScalableImage() = default;

    //! @brief Creates an image from SVG data
    //! @param data The SVG data
    //! @param size The size of the data in bytes
    //! @return The image (invalid if the data cannot be parsed)
    static ScalableImage fromSvg(void const* data, int size);

    //! @brief Checks if the image has a bitmap or a vector graphic
    bool isValid() const;

    //! @brief Gets an image that fits in the size
    //! @details The bitmap is returned as is while the vector graphic is rendered
    //! (preserving its proportions) to fit in the size.
    //! @param width The maximum width in pixels
    //! @param height The maximum height in pixels
    //! @return The image
    juce::Image getImage(int width, int height);

    bool operator==(ScalableImage const& other) const;
    bool operator!=(ScalableImage const& other) const;

private:
    juce::Image mImage;
    std::shared_ptr<juce::Drawable const> mDrawable;
    juce::Image mCachedImage;
};

MISC_FILE_END
