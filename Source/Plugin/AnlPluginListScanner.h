#pragma once

#include "AnlPluginListModel.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    class Scanner
    {
    public:
        
        std::tuple<std::set<Plugin::Key>, juce::StringArray> getKeys(double sampleRate = 48000.0);
        Plugin::Description getDescription(Plugin::Key const& key, double sampleRate = 48000.0);
        
    private:
        Vamp::Plugin* loadPlugin(std::string const& key, float sampleRate);
        
        using entry_t = std::tuple<std::string, float>;
        struct entry_comp
        {
            bool operator() (entry_t const& lhs, entry_t const& rhs) const
            {
                return std::get<0>(lhs) + std::to_string(std::get<1>(lhs)) < std::get<0>(rhs) + std::to_string(std::get<1>(rhs));
            }
        };
        
        std::mutex mMutex;
        std::map<entry_t, std::unique_ptr<Vamp::Plugin>, entry_comp> mPlugins;
    };
}

ANALYSE_FILE_END
