#pragma once

#include "AnlPluginListModel.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    class Scanner
    {
    public:
        std::tuple<std::map<Plugin::Key, Plugin::Description>, juce::StringArray> getPlugins(double sampleRate);
        Plugin::Description getDescription(Plugin::Key const& key, double sampleRate);

        static Plugin::Description loadDescription(Plugin::Key const& key, double sampleRate);

    private:
        Ive::PluginWrapper* loadPlugin(std::string const& key, float sampleRate);

        using entry_t = std::tuple<std::string, float>;
        struct entry_comp
        {
            bool operator()(entry_t const& lhs, entry_t const& rhs) const
            {
                return std::get<0>(lhs) + std::to_string(std::get<1>(lhs)) < std::get<0>(rhs) + std::to_string(std::get<1>(rhs));
            }
        };

        std::mutex mMutex;
        std::map<entry_t, std::unique_ptr<Ive::PluginWrapper>, entry_comp> mPlugins;
    };
} // namespace PluginList

ANALYSE_FILE_END
