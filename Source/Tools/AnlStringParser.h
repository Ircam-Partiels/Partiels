#pragma once

#include "AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Tools
{
    class StringParser
    {
    public:
        template <typename T> static T fromString(juce::String const& string)
        {
            return T(string);
        }
        
        template <typename T> static juce::String toString(T const& value)
        {
            return juce::String(value);
        }
        
        template <typename T> static T fromXml(juce::XmlElement const& xml, juce::StringRef const& name, T const& defaultValue = {})
        {
            return fromString<T>(xml.getStringAttribute(name, toString<T>(defaultValue)));
        }
        
        template <> juce::File fromString(juce::String const& string)
        {
            return juce::File(string);
        }
        
        template <> juce::String toString(juce::File const& value)
        {
            return value.getFullPathName();
        }
        
        template <> juce::Range<double> fromString(juce::String const& string)
        {
            juce::StringArray stringArray;
            stringArray.addLines(string);
            anlStrongAssert(stringArray.size() == 2);
            auto const start = stringArray.size() >= 1 ? stringArray.getReference(0).getDoubleValue() : 0.0;
            auto const end = stringArray.size() >= 2 ? stringArray.getReference(1).getDoubleValue() : 0.0;
            return {start, end};
        }
        
        template <> juce::String toString(juce::Range<double> const& value)
        {
            juce::StringArray stringArray;
            stringArray.ensureStorageAllocated(2);
            stringArray.add(juce::String(value.getStart()));
            stringArray.add(juce::String(value.getEnd()));
            return stringArray.joinIntoString("\n");
        }
        
        template <> std::vector<juce::File> fromString(juce::String const& string)
        {
            juce::StringArray filePaths;
            filePaths.addLines(string);
            std::vector<juce::File> files;
            files.reserve(static_cast<size_t>(filePaths.size()));
            for(auto const& filePath : filePaths)
            {
                files.push_back({filePath});
            }
            return files;
        }
        
        template <> juce::String toString(std::vector<juce::File> const& value)
        {
            juce::StringArray filePaths;
            filePaths.ensureStorageAllocated(static_cast<int>(value.size()));
            for(auto const& file : value)
            {
                filePaths.add(file.getFullPathName());
            }
            return filePaths.joinIntoString("\n");
        }
    };
}

ANALYSE_FILE_END
