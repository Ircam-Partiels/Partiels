#pragma once

#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Processor
    : public Vamp::HostExt::PluginWrapper
    {
    public:
        Processor(Vamp::Plugin* plugin);
        ~Processor() override = default;
        
        // Vamp::PluginBase
        ParameterList getParameterDescriptors() const override;
        float getParameter(std::string identifier) const override;
        void setParameter(std::string identifier, float value) override;
        
        size_t getWindowSize() const;
        size_t getStepSize() const;
    private:
        size_t mWindowSize = 512;
        size_t mWindowOverlapping = 4;
        std::string mIdentifierWindowType;
        std::string mIdentifierWindowSize;
        std::string mIdentifierWindowOverlapping;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
    };
    
    std::unique_ptr<Processor> createProcessor(Accessor const& accessor, double sampleRate, AlertType alertType);
    
    std::vector<Analyzer::Result> performAnalysis(Accessor const& accessor, juce::AudioFormatReader& audioFormatReader);
}

ANALYSE_FILE_END
