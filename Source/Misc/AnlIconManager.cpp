#include "AnlIconManager.h"
#include <IconsData.h>

ANALYSE_FILE_BEGIN

juce::Image IconManager::getIcon(IconType const type)
{
    switch(type)
    {
        case IconType::alert:
            return juce::ImageCache::getFromMemory(IconsData::alert_png, IconsData::alert_pngSize);
            break;
        case IconType::cancel:
            return juce::ImageCache::getFromMemory(IconsData::annuler_png, IconsData::annuler_pngSize);
            break;
        case IconType::checked:
            return juce::ImageCache::getFromMemory(IconsData::checked_png, IconsData::checked_pngSize);
            break;
        case IconType::conversation:
            return juce::ImageCache::getFromMemory(IconsData::conversation_png, IconsData::conversation_pngSize);
            break;
        case IconType::edit:
            return juce::ImageCache::getFromMemory(IconsData::editer_png, IconsData::editer_pngSize);
            break;
        case IconType::expand:
            return juce::ImageCache::getFromMemory(IconsData::expand_png, IconsData::expand_pngSize);
            break;
        case IconType::information:
            return juce::ImageCache::getFromMemory(IconsData::information_png, IconsData::information_pngSize);
            break;
        case IconType::loading:
            return juce::ImageCache::getFromMemory(IconsData::chargement_png, IconsData::chargement_pngSize);
            break;
        case IconType::loop:
            return juce::ImageCache::getFromMemory(IconsData::repeat_png, IconsData::repeat_pngSize);
            break;
        case IconType::navigate:
            return juce::ImageCache::getFromMemory(IconsData::naviguer_png, IconsData::naviguer_pngSize);
            break;
        case IconType::pause:
            return juce::ImageCache::getFromMemory(IconsData::pause_png, IconsData::pause_pngSize);
            break;
        case IconType::play:
            return juce::ImageCache::getFromMemory(IconsData::play_png, IconsData::play_pngSize);
            break;
        case IconType::properties:
            return juce::ImageCache::getFromMemory(IconsData::reglages_png, IconsData::reglages_pngSize);
            break;
        case IconType::question:
            return juce::ImageCache::getFromMemory(IconsData::question_png, IconsData::question_pngSize);
            break;
        case IconType::rewind:
            return juce::ImageCache::getFromMemory(IconsData::leftarrow_png, IconsData::leftarrow_pngSize);
            break;
        case IconType::search:
            return juce::ImageCache::getFromMemory(IconsData::chercher_png, IconsData::chercher_pngSize);
            break;
        case IconType::share:
            return juce::ImageCache::getFromMemory(IconsData::share_png, IconsData::share_pngSize);
            break;
        case IconType::shrink:
            return juce::ImageCache::getFromMemory(IconsData::shrink_png, IconsData::shrink_pngSize);
            break;
    }
    return juce::Image();
}

ANALYSE_FILE_END
