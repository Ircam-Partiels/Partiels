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
        std::unique_ptr<juce::Component> createProperty(Parameter const& parameter, std::function<void(Parameter const& parameter, float value)> applyChange);
        std::unique_ptr<Ive::PluginWrapper> createPluginWrapper(Key const& key, double readerSampleRate);
        std::vector<std::unique_ptr<Ive::PluginWrapper>> createAndInitializePluginWrappers(Key const& key, State const& state, size_t numReaderChannels, double readerSampleRate);
    } // namespace Tools
} // namespace Plugin

ANALYSE_FILE_END
