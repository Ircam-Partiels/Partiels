#pragma once

#include "AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace XmlParser
{
    template<typename T>
    void toXml(juce::XmlElement& xml, juce::Identifier const& attributeName, T const& value)
    {
        if constexpr(std::is_same<T, int>::value)
        {
            xml.setAttribute(attributeName, value);
        }
        else if constexpr(std::is_same<T, double>::value)
        {
            xml.setAttribute(attributeName, value);
        }
        else if constexpr(std::is_same<T, juce::String>::value)
        {
            xml.setAttribute(attributeName, value);
        }
        else if constexpr(std::is_same<T, float>::value)
        {
            xml.setAttribute(attributeName, static_cast<double>(value));
        }
        else if constexpr(std::is_enum<T>::value)
        {
            toXml(xml, attributeName, static_cast<typename std::underlying_type<T>::type>(value));
        }
        else if constexpr(is_specialization<T, std::vector>::value)
        {
            for(auto const& element : value)
            {
                auto child = std::make_unique<juce::XmlElement>(attributeName);
                anlWeakAssert(child != nullptr);
                if(child != nullptr)
                {
                    toXml(*child.get(), "value", element);
                    xml.addChildElement(child.release());
                }
            }
        }
        else if constexpr(is_specialization<T, std::map>::value)
        {
            for(auto const& element : value)
            {
                auto child = std::make_unique<juce::XmlElement>(attributeName);
                anlWeakAssert(child != nullptr);
                if(child != nullptr)
                {
                    toXml(*child.get(), "key", element.first);
                    toXml(*child.get(), "value", element.second);
                    xml.addChildElement(child.release());
                }
            }
        }
        else if constexpr(is_specialization<T, std::unique_ptr>::value)
        {
            if(value != nullptr)
            {
                auto child = std::make_unique<juce::XmlElement>(attributeName);
                anlWeakAssert(child != nullptr);
                if(child != nullptr)
                {
                    toXml(*child, attributeName, *value.get());
                    xml.addChildElement(child.release());
                }
            }
        }
        else
        {
            std::ostringstream stream;
            stream << value;
            xml.setAttribute(attributeName, juce::String(stream.str()));
        }
    }
    
    template<>
    void toXml<juce::Range<double>>(juce::XmlElement& xml, juce::Identifier const& attributeName, juce::Range<double> const& value);
    
    template<>
    void toXml<juce::File>(juce::XmlElement& xml, juce::Identifier const& attributeName, juce::File const& value);
    
    template<typename T>
    auto fromXml(juce::XmlElement const& xml, juce::Identifier const& attributeName, T const& defaultValue)
    -> T
    {
        if constexpr(std::is_same<T, int>::value)
        {
            anlWeakAssert(xml.hasAttribute(attributeName));
            return xml.getIntAttribute(attributeName, defaultValue);
        }
        else if constexpr(std::is_same<T, double>::value)
        {
            anlWeakAssert(xml.hasAttribute(attributeName));
            return xml.getDoubleAttribute(attributeName, defaultValue);
        }
        else if constexpr(std::is_same<T, juce::String>::value)
        {
            return xml.getStringAttribute(attributeName, defaultValue);
        }
        else if constexpr(std::is_same<T, float>::value)
        {
            anlWeakAssert(xml.hasAttribute(attributeName));
            return static_cast<float>(xml.getDoubleAttribute(attributeName, static_cast<double>(defaultValue)));
        }
        else if constexpr(std::is_enum<T>::value)
        {
            return static_cast<T>(fromXml(xml, attributeName, static_cast<typename std::underlying_type<T>::type>(defaultValue)));
        }
        else if constexpr(is_specialization<T, std::vector>::value)
        {
            T result {defaultValue};
            using value_type = typename T::value_type;
            size_t index = 0;
            for(auto* child = xml.getChildByName(attributeName); child != nullptr; child = child->getNextElementWithTagName(attributeName))
            {
                if(index < result.size())
                {
                    result[index] = fromXml(*child, "value", result[index]);
                }
                else
                {
                    result.push_back(fromXml(*child, "value", value_type{}));
                }
                ++index;
            }
            return result;
        }
        else if constexpr(is_specialization<T, std::map>::value)
        {
            T result {defaultValue};
            using key_type = typename T::key_type;
            for(auto* child = xml.getChildByName(attributeName); child != nullptr; child = child->getNextElementWithTagName(attributeName))
            {
                auto const entry = fromXml(*child, "key", key_type{});
                result[entry] = fromXml(*child, "value", result[entry]);
            }
            return result;
        }
        else if constexpr(is_specialization<T, std::unique_ptr>::value)
        {
            auto* child = xml.getChildByName(attributeName);
            if(child != nullptr)
            {
                return std::make_unique<T>(fromXml(*child, "attributeName", *defaultValue.get()));
            }
            return std::make_unique<T>(*defaultValue.get());
        }
        else
        {
            T value {defaultValue};
            std::stringstream stream(xml.getStringAttribute("attributeName").toRawUTF8());
            stream >> value;
            return value;
        }
    }
    
    template<>
    auto fromXml<juce::Range<double>>(juce::XmlElement const& xml, juce::Identifier const& attributeName, juce::Range<double> const& defaultValue)
    -> juce::Range<double>;
    
    
    template<>
    auto fromXml<juce::File>(juce::XmlElement const& xml, juce::Identifier const& attributeName, juce::File const& defaultValue)
    -> juce::File;
}

ANALYSE_FILE_END
