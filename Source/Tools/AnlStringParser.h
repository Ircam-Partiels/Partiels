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
        
        template <> juce::File fromString(juce::String const& string);
        template <> juce::String toString(juce::File const& value);
        template <> juce::Range<double> fromString(juce::String const& string);
        template <> juce::String toString(juce::Range<double> const& value);
        template <> std::vector<juce::File> fromString(juce::String const& string);
        template <> juce::String toString(std::vector<juce::File> const& value);
    };
    
    class XmlUtils
    {
    public:
        static std::vector<std::reference_wrapper<juce::XmlElement>> getChilds(juce::XmlElement const& xml, juce::StringRef const& tag);
        
        template<typename T> static void addChilds(juce::XmlElement& xml, std::vector<std::unique_ptr<T>> const& vector, juce::StringRef const& tag)
        {
            for(auto const& element : vector)
            {
                anlWeakAssert(element != nullptr);
                if(element != nullptr)
                {
                    auto child = element->toXml();
                    anlWeakAssert(child != nullptr);
                    if(child != nullptr)
                    {
                        child->setTagName(tag);
                        xml.addChildElement(child.release());
                    }
                }
            }
        }
        
        template<typename T> static void fromXml(juce::XmlElement const& xml, std::vector<std::unique_ptr<T>>& vector, juce::StringRef const& tag)
        {
            for(auto const& element : vector)
            {
                anlWeakAssert(element != nullptr);
                if(element != nullptr)
                {
                    auto child = element->toXml();
                    anlWeakAssert(child != nullptr);
                    if(child != nullptr)
                    {
                        child->setTagName(tag);
                        xml.addChildElement(child.release());
                    }
                }
            }
        }
    };
}

ANALYSE_FILE_END
