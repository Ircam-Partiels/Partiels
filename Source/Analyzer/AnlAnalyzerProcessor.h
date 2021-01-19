#pragma once

#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Processor
    : public Vamp::HostExt::PluginWrapper
    {
    public:
        using Result = Anl::Plugin::Result;
        
        Processor(Vamp::Plugin* plugin, size_t feature);
        ~Processor() override = default;
        
        // Vamp::PluginBase
        ParameterList getParameterDescriptors() const override;
        float getParameter(std::string identifier) const override;
        void setParameter(std::string identifier, float value) override;
        
        std::map<juce::String, double> getParameters() const;
        void setParameters(std::map<juce::String, double> const& parameters);
        
        bool hasZoomInfo(size_t const feature) const;
        std::tuple<Zoom::Range, double> getZoomInfo(size_t const feature) const;
        
        bool prepareForAnalysis(juce::AudioFormatReader& audioFormatReader);
        bool performNextAudioBlock(std::vector<Result>& results);
        
        size_t getWindowSize() const;
        size_t getStepSize() const;
    private:
        
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
    
    std::unique_ptr<Processor> createProcessor(Accessor const& accessor, double sampleRate, AlertType alertType);
}

ANALYSE_FILE_END
