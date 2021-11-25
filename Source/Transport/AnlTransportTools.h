#pragma once

#include "AnlTransportModel.h"

ANALYSE_FILE_BEGIN

namespace Transport
{
    namespace Tools
    {
        bool canRewindPlayhead(Accessor const& accessor);
        void rewindPlayhead(Accessor& accessor);
    } // namespace Tools
} // namespace Transport

ANALYSE_FILE_END
