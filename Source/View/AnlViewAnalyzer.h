#pragma once

#include "../Model/AnlModelAnalyzer.h"

#include <vamp-hostsdk/Plugin.h>

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class View
    {
    public:
        using Accessor = Accessor;
        using Attribute = Model::Attribute;
        
        View(Accessor& accessor);
        ~View();
        
        void perform(juce::AudioFormatReader& audioFormatReader, size_t blockSize = 512);

    private:
        
        class Impl;
        std::unique_ptr<Impl> mImpl;
        
        JUCE_LEAK_DETECTOR(Model)
    };
}

ANALYSE_FILE_END
