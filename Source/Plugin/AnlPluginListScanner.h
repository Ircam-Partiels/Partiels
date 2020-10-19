
#pragma once

#include "AnlPluginListModel.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    class Scanner
    {
    public:
        using Description = PluginList::Model::Description;
        
        static std::set<std::string> getPluginKeys();
        static std::map<juce::String, Description> getPluginDescriptions(double defaultSampleRate = 48000.0);
    };
}

ANALYSE_FILE_END
