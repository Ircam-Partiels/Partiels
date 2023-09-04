#include "AnlLoadingIcon.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

LoadingIcon::LoadingIcon()
: SpinningIcon(juce::ImageCache::getFromMemory(AnlIconsData::loading_png, AnlIconsData::loading_pngSize))
{
}

ANALYSE_FILE_END
