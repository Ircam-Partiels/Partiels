#pragma once

#include "../Tools/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    struct Model
    {
        juce::String windowState;
        std::vector<juce::File> recentlyOpenedFilesList;
        std::vector<juce::File> currentOpenedFilesList;
        juce::File currentDocumentFile;
        
        static Model fromXml(juce::XmlElement const& xml);
        std::unique_ptr<juce::XmlElement> toXml() const;
        
        static std::vector<juce::File> fromString(juce::String const& filesAsString);
        static juce::String toString(std::vector<juce::File> const& files);
        static void sanitize(std::vector<juce::File>& files);
        
        JUCE_LEAK_DETECTOR(Model)
    };
}

ANALYSE_FILE_END
