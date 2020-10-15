#pragma once

#define anlStrongAssert assert
#define anlWeakAssert jassert

#define ANALYSE_FILE_BEGIN namespace Anl {
    
#define ANALYSE_FILE_END }

#include "JuceHeader.h"

#include <mutex>
#include <set>
#include <utility>
#include <atomic>
#include <memory>

