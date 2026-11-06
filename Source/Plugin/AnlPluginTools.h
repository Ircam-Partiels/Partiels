#pragma once

#include "AnlPluginModel.h"

ANALYSE_FILE_BEGIN

namespace Plugin
{
    class LoadingError
    : public std::runtime_error
    {
    public:
        LoadingError(char const* message);
    };

    class ParametersError
    : public std::runtime_error
    {
    public:
        ParametersError(char const* message);
    };

    namespace Tools
    {
        juce::String rangeToString(float min, float max, bool quantized, float step);
        //! @brief Gets the tooltip of the property of a parameter
        //! @param parameter The description of the parameter
        //! @return The tooltip used by the property created with createProperty()
        juce::String getParameterTooltip(Parameter const& parameter);
        std::unique_ptr<juce::Component> createProperty(Parameter const& parameter, std::function<void(Parameter const& parameter, float value)> applyChange);
        void setPropertyValue(juce::Component& property, Parameter const& parameter, std::set<float> const& values, juce::NotificationType notification);

        std::vector<std::unique_ptr<Ive::PluginWrapper>> createPluginWrappers(Key const& key, State const& state, size_t numReaderChannels, double readerSampleRate);
        std::optional<size_t> getFeatureIndex(Vamp::Plugin const& plugin, std::string const& feature);

        //! @brief Sets the parameters of a plugin with the values of a state
        //! @details The parameters of the state that are not supported by the plugin are ignored.
        //! @param plugin The plugin
        //! @param state The state that contains the values of the parameters
        void setParameters(Vamp::Plugin& plugin, State const& state);

        //! @brief Sets the numbers of voices of the outputs of a plugin
        //! @details The outputs that have no voices are ignored. The numbers of voices must be set before retrieving
        //! the outputs of the plugin (because the Vamp plugin adapter caches them) and before its initialisation.
        //! @param plugin The plugin
        //! @param voiceCounts The numbers of voices of the outputs
        void setVoiceCounts(Ive::PluginWrapper& plugin, VoiceCounts const& voiceCounts);

        //! @brief Clamps a number of voices to the range of the descriptor of the number of voices
        //! @param voiceDescriptor The descriptor of the number of voices
        //! @param count The number of voices
        //! @return The number of voices in the range of the descriptor (at least 1)
        size_t clampVoiceCount(Parameter const& voiceDescriptor, size_t count);

        //! @brief Gets the name of an output with voices
        //! @details The name is the name of the first voice without the trailing number of the voice (e.g. "Pitch"
        //! for "Pitch 1") or the name of the output if it has no voices.
        //! @param description The description of the output
        //! @return The name of the output
        juce::String getVoicesName(Description const& description);
    } // namespace Tools

    class InputProperty
    : public PropertyList
    {
    public:
        InputProperty(Plugin::Input const& input, std::function<void(Plugin::Input const&, juce::String const&)> fn);
        ~InputProperty() = default;

        //! @brief Gets the tooltip of the property of an input
        //! @param input The description of the input
        //! @return The tooltip used by the property
        static juce::String getInputTooltip(Plugin::Input const& input);

        void setInputs(juce::StringPairArray const& availableInputs, juce::StringArray const& selectedInputs);

    private:
        Plugin::Input const mInput;
        juce::StringArray mInputIds;
    };
} // namespace Plugin

ANALYSE_FILE_END
