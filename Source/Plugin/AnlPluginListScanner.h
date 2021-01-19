
#pragma once

#include "AnlPluginListModel.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    class Scanner
    {
    public:

        static std::map<Plugin::Key, Plugin::Description> getPluginDescriptions(double defaultSampleRate = 48000.0);
    };
}

ANALYSE_FILE_END
