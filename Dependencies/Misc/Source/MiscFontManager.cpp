#include "MiscFontManager.h"
#include <MiscFontsData.h>

MISC_FILE_BEGIN

FontManager::FontManager()
{
    for(auto index = 0; index < MiscFontsData::namedResourceListSize; ++index)
    {
        auto size = 0;
        auto const* name = MiscFontsData::namedResourceList[index];
        MiscWeakAssert(name != nullptr);
        if(name == nullptr)
        {
            continue;
        }
        auto const* data = MiscFontsData::getNamedResource(name, size);
        MiscWeakAssert(data != nullptr && size != 0);
        if(data == nullptr || size == 0)
        {
            continue;
        }
        auto typeface = juce::Typeface::createSystemTypefaceFor(data, static_cast<size_t>(size));
        MiscWeakAssert(typeface != nullptr);
        if(typeface != nullptr)
        {
            mFonts.push_back(typeface);
            // The default typeface reuses the loaded instance to avoid registering the same font twice
            if(data == MiscFontsData::NunitoSansRegular_ttf)
            {
                mDefaultSansSerifTypeface = typeface;
            }
        }
    }
    MiscWeakAssert(mDefaultSansSerifTypeface != nullptr);
}

juce::Typeface::Ptr FontManager::getDefaultSansSerifTypefaceWithStyle(juce::String const& style)
{
    for(auto typeface : mFonts)
    {
        MiscWeakAssert(typeface != nullptr);
        if(typeface != nullptr && typeface->getStyle() == style)
        {
            return typeface;
        }
    }
    return getDefaultSansSerifTypeface();
}

juce::Typeface::Ptr FontManager::getDefaultSansSerifTypeface()
{
    return mDefaultSansSerifTypeface;
}

juce::String FontManager::getDefaultSansSerifTypefaceName()
{
    return getDefaultSansSerifTypeface()->getName();
}

MISC_FILE_END
