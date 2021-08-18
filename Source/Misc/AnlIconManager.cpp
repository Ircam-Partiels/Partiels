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
        case IconType::comment:
            return juce::ImageCache::getFromMemory(IconsData::comment_png, IconsData::comment_pngSize);
        case IconType::expand:
            return juce::ImageCache::getFromMemory(IconsData::expand_png, IconsData::expand_pngSize);
        case IconType::focus:
            return juce::ImageCache::getFromMemory(IconsData::focus_png, IconsData::focus_pngSize);
        case IconType::grid_off:
            return juce::ImageCache::getFromMemory(IconsData::gridoff_png, IconsData::gridoff_pngSize);
        case IconType::grid_partial:
            return juce::ImageCache::getFromMemory(IconsData::gridpartial_png, IconsData::gridpartial_pngSize);
        case IconType::grid_full:
            return juce::ImageCache::getFromMemory(IconsData::gridfull_png, IconsData::gridfull_pngSize);
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
        case IconType::pause:
            return juce::ImageCache::getFromMemory(IconsData::pause_png, IconsData::pause_pngSize);
        case IconType::photocamera:
            return juce::ImageCache::getFromMemory(IconsData::photocamera_png, IconsData::photocamera_pngSize);
        case IconType::play:
            return juce::ImageCache::getFromMemory(IconsData::play_png, IconsData::play_pngSize);
        case IconType::plus:
            return juce::ImageCache::getFromMemory(IconsData::plus_png, IconsData::plus_pngSize);
        case IconType::properties:
            return juce::ImageCache::getFromMemory(IconsData::settings_png, IconsData::settings_pngSize);
        case IconType::question:
            return juce::ImageCache::getFromMemory(IconsData::question_png, IconsData::question_pngSize);
        case IconType::rewind:
            return juce::ImageCache::getFromMemory(IconsData::leftarrow_png, IconsData::leftarrow_pngSize);
        case IconType::shrink:
            return juce::ImageCache::getFromMemory(IconsData::shrink_png, IconsData::shrink_pngSize);
        case IconType::verified:
            return juce::ImageCache::getFromMemory(IconsData::verified_png, IconsData::verified_pngSize);
    }
    return juce::Image();
}

ANALYSE_FILE_END
