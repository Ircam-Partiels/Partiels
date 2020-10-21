#pragma once

#include "../Analyzer/AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    //! @brief The audio reader of a document
    class Reader
    : public juce::PositionableAudioSource
    {

        JUCE_LEAK_DETECTOR(Reader)
    };
}

ANALYSE_FILE_END
