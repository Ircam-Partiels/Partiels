#pragma once

#include "AnlPluginListModel.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    class Scanner
    {
    public:
        std::tuple<std::map<Plugin::Key, Plugin::Description>, juce::StringArray> getPlugins(double sampleRate, bool forceRescan);
        Plugin::Description getDescription(Plugin::Key const& key, double sampleRate);

        static Plugin::Description loadDescription(Plugin::Key const& key, double sampleRate);

        //! @brief Loads the description of a plugin with a state and numbers of voices
        //! @details The description depends on the parameters of the state and on the numbers of voices.
        //! @param key The key of the plugin
        //! @param sampleRate The sample rate used to instantiate the plugin
        //! @param state The state used to set the parameters of the plugin
        //! @param voiceCounts The numbers of voices of the outputs
        //! @return The description of the plugin (or an invalid description if the plugin or the output cannot be found)
        static Plugin::Description loadDescription(Plugin::Key const& key, double sampleRate, Plugin::State const& state, Plugin::VoiceCounts const& voiceCounts);

        //! @brief Loads the descriptions of the voices of an output of a plugin with a state and numbers of voices
        //! @details If the output has no voices, only the description of the output is returned.
        //! @param key The key of the plugin (the feature should be the first voice of the output)
        //! @param sampleRate The sample rate used to instantiate the plugin
        //! @param state The state used to set the parameters of the plugin
        //! @param voiceCounts The numbers of voices of the outputs
        //! @return The descriptions of the voices (or an empty list if the plugin or the output cannot be found)
        static std::vector<Plugin::Description> loadVoiceDescriptions(Plugin::Key const& key, double sampleRate, Plugin::State const& state, Plugin::VoiceCounts const& voiceCounts);

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
