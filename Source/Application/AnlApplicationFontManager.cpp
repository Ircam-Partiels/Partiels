#include "AnlApplicationFontManager.h"
#include <FontsData.h>

ANALYSE_FILE_BEGIN

Application::FontManager::FontManager()
{
    auto createTypeface = [](int index) -> juce::Typeface::Ptr
    {
        int size = 0;
        auto const* name = FontsData::namedResourceList[index];
        anlWeakAssert(name != nullptr);
        if(name == nullptr)
        {
            return {};
        }
        auto const* data = FontsData::getNamedResource(name, size);
        anlWeakAssert(data != nullptr && size != 0);
        if(data == nullptr && size == 0)
        {
            return {};
        }
        return juce::Typeface::createSystemTypefaceFor(data, static_cast<size_t>(size));
    };
    for(int index = 0; index < FontsData::namedResourceListSize; ++index)
    {
        auto typeface = createTypeface(index);
        anlWeakAssert(typeface != nullptr);
        if(typeface != nullptr)
        {
            mFonts.push_back(typeface);
            mDefaultSansSerifTypeface = typeface;
        }
    }
}

juce::Typeface::Ptr Application::FontManager::getDefaultSansSerifTypeface()
{
    static auto typeface = juce::Typeface::createSystemTypefaceFor(FontsData::FallingSkyLightK2EX_otf, FontsData::FallingSkyLightK2EX_otfSize);
    return typeface;
}

juce::String Application::FontManager::getDefaultSansSerifTypefaceName()
{
    return getDefaultSansSerifTypeface()->getName();
}

ANALYSE_FILE_END
