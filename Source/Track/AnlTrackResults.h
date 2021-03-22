#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Results
    {
    public:
        using Result = Plugin::Result;
        using Container = std::vector<Result>;
        
        static Container::const_iterator getResultAt(Container const& results, double time);
        static Zoom::Range getValueRange(Container const& results);
        static Zoom::Range getBinRange(Container const& results);
    };
}

ANALYSE_FILE_END
