#pragma once

#include "AnlBase.h"

ANALYSE_FILE_BEGIN

class LoadingIcon
: public SpinningIcon
{
public:
    LoadingIcon();
    ~LoadingIcon() override = default;
};

ANALYSE_FILE_END
