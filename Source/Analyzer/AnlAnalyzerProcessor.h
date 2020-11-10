#pragma once

#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Processor
    {
    public:
        Processor(Accessor& accessor);
        ~Processor();
        
        void perform(juce::AudioFormatReader& audioFormatReader, size_t blockSize = 512);

    private:
        
        class Impl;
        std::unique_ptr<Impl> mImpl;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
    };
}

ANALYSE_FILE_END
