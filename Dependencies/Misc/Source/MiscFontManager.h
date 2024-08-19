#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

class FontManager
{
public:
    FontManager();
    ~FontManager() = default;

    juce::Typeface::Ptr getDefaultSansSerifTypeface();
    juce::Typeface::Ptr getDefaultSansSerifTypefaceWithStyle(juce::String const& style);
    juce::String getDefaultSansSerifTypefaceName();

private:
    std::vector<juce::Typeface::Ptr> mFonts;
    juce::Typeface::Ptr mDefaultSerifTypeface;
};

MISC_FILE_END
