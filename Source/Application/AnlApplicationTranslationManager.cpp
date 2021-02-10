#include "AnlApplicationTranslationManager.h"
#include <TranslationsData.h>

ANALYSE_FILE_BEGIN

Application::TranslationManager::TranslationManager()
{
    auto ls = std::make_unique<juce::LocalisedStrings>(juce::String::createStringFromData(TranslationsData::Fr_txt, TranslationsData::Fr_txtSize), false);
    if(ls != nullptr)
    {
        ls->addStrings({juce::String::createStringFromData(TranslationsData::Plugin_txt, TranslationsData::Plugin_txtSize), false});
        ls->addStrings({juce::String::createStringFromData(TranslationsData::Analyzer_txt, TranslationsData::Analyzer_txtSize), false});
    }
    juce::LocalisedStrings::setCurrentMappings(ls.release());
}

ANALYSE_FILE_END

