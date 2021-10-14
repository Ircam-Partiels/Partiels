#pragma once

#include "../../Dependencies/Misc/Source/Misc.h"
namespace Anl = Misc;

#define ANALYSE_FILE_BEGIN \
    namespace Misc         \
    {
#define ANALYSE_FILE_END }

#define anlWeakAssert MiscWeakAssert
#define anlStrongAssert MiscStrongAssert

#define anlDebug MiscDebug
#define anlError MiscError
