#include "AnlApplicationTranslationManager.h"
#include <TranslationsData.h>

ANALYSE_FILE_BEGIN

void Application::TranslationManager::loadFromBinaries()
{
    auto ls = std::make_unique<juce::LocalisedStrings>(juce::String(), false);
    if(ls != nullptr)
    {
        for(int i = 0; i < TranslationsData::namedResourceListSize; ++i)
        {
            int size = 0;
            auto const* data = TranslationsData::namedResourceList[i];
            TranslationsData::getNamedResource(data, size);
            ls->addStrings({juce::String::createStringFromData(data, size), false});
        }
    }
    juce::LocalisedStrings::setCurrentMappings(ls.release());
}

ANALYSE_FILE_END
