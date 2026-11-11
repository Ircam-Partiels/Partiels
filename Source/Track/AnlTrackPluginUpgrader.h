#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    //! @brief Upgrades the tracks that use former versions of plugins
    //! @details The new versions of some plugins are not compatible with the tracks created with the former versions:
    //! the library can be renamed (so the plugin keys change), parameters can be renamed and the parameters that
    //! selected the index of a result can be replaced with the voices of the outputs (the voice N corresponds to the
    //! former index N and the identifier of the first voice is the identifier of the former output). The rules of the
    //! upgrades are defined for each plugin library (see the implementation for the supported libraries). The tracks
    //! are upgraded only if the new versions of the plugins are installed (if the descriptions of the new versions can
    //! be loaded), otherwise they remain unchanged.
    namespace PluginUpgrader
    {
        //! @brief The function that loads the description of a plugin with a state and numbers of voices
        //! @details The function returns an invalid description if the plugin or the output cannot be found.
        using DescriptionLoader = std::function<Plugin::Description(Plugin::Key const& key, double sampleRate, Plugin::State const& state, Plugin::VoiceCounts const& voiceCounts)>;

        //! @brief Loads the description of an installed plugin with a state and numbers of voices
        //! @param key The key of the plugin
        //! @param sampleRate The sample rate used to instantiate the plugin
        //! @param state The state used to set the parameters of the plugin
        //! @param voiceCounts The numbers of voices of the outputs
        //! @return The description of the plugin (or an invalid description if the plugin or the output cannot be found)
        Plugin::Description loadInstalledDescription(Plugin::Key const& key, double sampleRate, Plugin::State const& state, Plugin::VoiceCounts const& voiceCounts);

        //! @brief Upgrades a track that uses a former version of a plugin
        //! @details A track that used an index parameter is converted to the track of the corresponding voice that is
        //! not linked to other tracks.
        //! @param trackXml The XML of the track (converted to the current format)
        //! @param loader The function used to load the descriptions of the new versions of the plugins
        //! @return true if the track has been upgraded, false otherwise
        bool upgradeTrack(juce::XmlElement& trackXml, DescriptionLoader const& loader = loadInstalledDescription);

        //! @brief Upgrades the tracks of a document that use former versions of plugins
        //! @details The tracks that used an index parameter are converted to the tracks of the corresponding voices
        //! and the tracks that performed the same analysis (same plugin, output, state, inputs and sample rate) are
        //! linked (the number of voices is the highest index). A single track remains unlinked. The tracks can be in
        //! the format of a former version of the application: only the key, the state, the description, the numbers
        //! of voices and the link of the tracks are upgraded, the other elements are converted when the tracks are
        //! restored.
        //! @param documentXml The XML of the document (before the restoration of the tracks)
        //! @param loader The function used to load the descriptions of the new versions of the plugins
        void upgradeTracks(juce::XmlElement& documentXml, DescriptionLoader const& loader = loadInstalledDescription);
    } // namespace PluginUpgrader
} // namespace Track

ANALYSE_FILE_END
