#include "MiscFontManager.h"
#include <MiscFontsData.h>

MISC_FILE_BEGIN

FontManager::FontManager()
{
    auto const createTypeface = [](int index) -> juce::Typeface::Ptr
    {
        auto size = 0;
        auto const* name = MiscFontsData::namedResourceList[index];
        MiscWeakAssert(name != nullptr);
        if(name == nullptr)
        {
            return {};
        }
        auto const* data = MiscFontsData::getNamedResource(name, size);
        MiscWeakAssert(data != nullptr && size != 0);
        if(data == nullptr && size == 0)
        {
            return {};
        }
        return juce::Typeface::createSystemTypefaceFor(data, static_cast<size_t>(size));
    };
    for(auto index = 0; index < MiscFontsData::namedResourceListSize; ++index)
    {
        auto typeface = createTypeface(index);
        MiscWeakAssert(typeface != nullptr);
        if(typeface != nullptr)
        {
            mFonts.push_back(typeface);
        }
    }
    mDefaultSerifTypeface = juce::Typeface::createSystemTypefaceFor(MiscFontsData::NunitoSansRegular_ttf, MiscFontsData::NunitoSansRegular_ttfSize);
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
    return mDefaultSerifTypeface;
}

juce::String FontManager::getDefaultSansSerifTypefaceName()
{
    return getDefaultSansSerifTypeface()->getName();
}

MISC_FILE_END
