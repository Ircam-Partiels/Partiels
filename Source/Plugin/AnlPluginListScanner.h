#pragma once

#include "AnlPluginListModel.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    class Scanner
    {
    public:
        static std::set<Plugin::Key> getPluginKeys(double sampleRate, AlertType const alertType);
        static Plugin::Description getPluginDescription(Plugin::Key const& key, double sampleRate, AlertType const alertType);
        
    private:
        static Vamp::Plugin* loadPlugin(std::string const& key, float sampleRate, AlertType const alertType, std::string const& errorMessage = "");
    };
}

ANALYSE_FILE_END
