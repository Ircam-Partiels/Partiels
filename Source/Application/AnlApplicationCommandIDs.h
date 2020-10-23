#pragma once

#include "../Tools/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace CommandIDs
    {
        enum : int
        {
            Open = 0x200100,
            New = 0x200101,
            Save = 0x200102,
            SaveAs = 0x200103
        };
    }
}

ANALYSE_FILE_END

