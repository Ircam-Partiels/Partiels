#include "AnlIconManager.h"
#include <IconsData.h>

ANALYSE_FILE_BEGIN

juce::Image IconManager::getIcon(IconType const type)
{
    switch(type)
    {
        case IconType::alert:
            return juce::ImageCache::getFromMemory(IconsData::alert_png, IconsData::alert_pngSize);
        case IconType::cancel:
            return juce::ImageCache::getFromMemory(IconsData::annuler_png, IconsData::annuler_pngSize);
        case IconType::checked:
            return juce::ImageCache::getFromMemory(IconsData::checked_png, IconsData::checked_pngSize);
        case IconType::chevron:
            return juce::ImageCache::getFromMemory(IconsData::chevron_png, IconsData::chevron_pngSize);
        case IconType::conversation:
            return juce::ImageCache::getFromMemory(IconsData::conversation_png, IconsData::conversation_pngSize);
        case IconType::edit:
            return juce::ImageCache::getFromMemory(IconsData::editer_png, IconsData::editer_pngSize);
        case IconType::expand:
            return juce::ImageCache::getFromMemory(IconsData::expand_png, IconsData::expand_pngSize);
        case IconType::layers:
            return juce::ImageCache::getFromMemory(IconsData::layers_png, IconsData::layers_pngSize);
        case IconType::information:
            return juce::ImageCache::getFromMemory(IconsData::information_png, IconsData::information_pngSize);
        case IconType::loading:
            return juce::ImageCache::getFromMemory(IconsData::chargement_png, IconsData::chargement_pngSize);
        case IconType::loop:
            return juce::ImageCache::getFromMemory(IconsData::repeat_png, IconsData::repeat_pngSize);
        case IconType::music:
            return juce::ImageCache::getFromMemory(IconsData::musicplayer_png, IconsData::musicplayer_pngSize);
        case IconType::navigate:
            return juce::ImageCache::getFromMemory(IconsData::naviguer_png, IconsData::naviguer_pngSize);
        case IconType::pause:
            return juce::ImageCache::getFromMemory(IconsData::pause_png, IconsData::pause_pngSize);
        case IconType::perspective:
            return juce::ImageCache::getFromMemory(IconsData::perspective_png, IconsData::perspective_pngSize);
        case IconType::photocamera:
            return juce::ImageCache::getFromMemory(IconsData::photocamera_png, IconsData::photocamera_pngSize);
        case IconType::play:
            return juce::ImageCache::getFromMemory(IconsData::play_png, IconsData::play_pngSize);
        case IconType::plus:
            return juce::ImageCache::getFromMemory(IconsData::plus_png, IconsData::plus_pngSize);
        case IconType::properties:
            return juce::ImageCache::getFromMemory(IconsData::reglages_png, IconsData::reglages_pngSize);
        case IconType::question:
            return juce::ImageCache::getFromMemory(IconsData::question_png, IconsData::question_pngSize);
        case IconType::rewind:
            return juce::ImageCache::getFromMemory(IconsData::leftarrow_png, IconsData::leftarrow_pngSize);
        case IconType::search:
            return juce::ImageCache::getFromMemory(IconsData::chercher_png, IconsData::chercher_pngSize);
        case IconType::share:
            return juce::ImageCache::getFromMemory(IconsData::share_png, IconsData::share_pngSize);
        case IconType::shrink:
            return juce::ImageCache::getFromMemory(IconsData::shrink_png, IconsData::shrink_pngSize);
    }
    return juce::Image();
}

ANALYSE_FILE_END
