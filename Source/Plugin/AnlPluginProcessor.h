#pragma once

#include "AnlPluginModel.h"

ANALYSE_FILE_BEGIN

namespace Plugin
{
    class Processor
    {
    public:
        static std::unique_ptr<Processor> create(Key const& key, State const& state, juce::AudioFormatReader& audioFormatReader, AlertType alertType);
        
        ~Processor() = default;
        
        bool performNextAudioBlock(std::vector<Result>& results);

        Description getDescription() const;
        State getState() const;
    private:
        
        Processor(juce::AudioFormatReader& audioFormatReader, std::unique_ptr<Vamp::Plugin> plugin, size_t const feature, State const& state);
        
        juce::AudioFormatReader& mAudioFormatReader;
        std::unique_ptr<Vamp::Plugin> mPlugin;
        size_t const mFeature;
        State const mState;
        
        juce::int64 mPosition {0};
        juce::AudioBuffer<float> mBuffer;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
    };
}

ANALYSE_FILE_END
