#pragma once

#include "AnlAnalyzerModel.h"

namespace Vamp
{
    class Plugin;
}

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class PluginManager
    {
    public:
        static std::shared_ptr<Vamp::Plugin> createInstance(Accessor const& accessor, bool showMessageOnFailure);
    };
}

ANALYSE_FILE_END
