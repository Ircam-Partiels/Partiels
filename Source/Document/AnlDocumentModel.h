#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    struct Model
    {
        uint32_t version;
        
        static Model fromXml(juce::XmlElement const& xml);
        std::unique_ptr<juce::XmlElement> toXml() const;
        
        JUCE_LEAK_DETECTOR(Model)
    };
}

ANALYSE_FILE_END
