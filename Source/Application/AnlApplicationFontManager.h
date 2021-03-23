#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class FontManager
    {
    public:
        FontManager();
        ~FontManager() = default;
        
        juce::Typeface::Ptr getDefaultSansSerifTypeface();
        juce::String getDefaultSansSerifTypefaceName();
    private:
        std::vector<juce::Typeface::Ptr> mFonts;
        juce::Typeface::Ptr mDefaultSansSerifTypeface;
    };
}

ANALYSE_FILE_END

