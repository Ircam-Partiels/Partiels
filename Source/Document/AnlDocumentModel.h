#pragma once

#include "../Model/AnlModelAnalyzer.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    //! @brief The data model of a document
    //! @brief The document is a container for a set of analysis
    struct Model
    {
        juce::String version; //!< The version of the data model
        juce::File file; //!< The audio file associated with the document
        juce::File project; //!< The project file associated with the document
        std::bitset<64> channels; //!< The visibility states of the channels
        std::vector<Analyzer::Model> tracks; //!< The visibility states of the channels
        
        static Model fromXml(juce::XmlElement const& xml);
        std::unique_ptr<juce::XmlElement> toXml() const;
        
        JUCE_LEAK_DETECTOR(Model)
    };
}

ANALYSE_FILE_END
