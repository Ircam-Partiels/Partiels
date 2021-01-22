#pragma once

#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Processor
    {
    public:
        using Result = Anl::Plugin::Result;
        using Description = Anl::Plugin::Description;
        using Parameter = Anl::Plugin::Parameter;
        using Output = Anl::Plugin::Output;
        
        static std::unique_ptr<Processor> create(Accessor const& accessor, double sampleRate, AlertType alertType);
        
        ~Processor() = default;
        
        double getSampleRate() const;
        std::vector<juce::String, Parameter> getParameters() const;
        Description getDescription() const;
        Output getOutput() const;
        
        void setParameterValues(std::map<juce::String, double> const& parameters);
        
        bool prepareForAnalysis(juce::AudioFormatReader& audioFormatReader);
        bool performNextAudioBlock(std::vector<Result>& results);

    private:
        
        Processor(std::unique_ptr<Vamp::Plugin> plugin, size_t feature);
        
        std::unique_ptr<Vamp::Plugin> mPlugin;
        juce::int64 mPosition {0};
        juce::AudioBuffer<float> mBuffer;
        size_t const mFeature;
        juce::AudioFormatReader* mAudioFormatReader;
        
        size_t mWindowSize = 512;
        size_t mWindowOverlapping = 4;
        std::string mIdentifierWindowType;
        std::string mIdentifierWindowSize;
        std::string mIdentifierWindowOverlapping;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
    };
}

ANALYSE_FILE_END
