#include "AnlLoadingIcon.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

LoadingIcon::LoadingIcon()
: SpinningIcon(ScalableImage::fromSvg(AnlIconsData::Loading_svg, AnlIconsData::Loading_svgSize))
{
}

ANALYSE_FILE_END
