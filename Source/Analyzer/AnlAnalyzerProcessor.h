#pragma once

#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Processor
    {
    public:
        using Accessor = Accessor;
        using Attribute = Model::Attribute;
        
        Processor(Accessor& accessor);
        ~Processor();
        
        void perform(juce::AudioFormatReader& audioFormatReader, size_t blockSize = 512);

    private:
        
        class Impl;
        std::unique_ptr<Impl> mImpl;
        
        JUCE_LEAK_DETECTOR(Model)
    };
}

ANALYSE_FILE_END
