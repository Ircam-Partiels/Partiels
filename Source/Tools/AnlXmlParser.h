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
        else if constexpr(std::is_same<T, juce::Colour>::value)
        {
            xml.setAttribute(attributeName, value.toString());
        }
        else if constexpr(std::is_same<T, float>::value)
        {
            xml.setAttribute(attributeName, static_cast<double>(value));
        }
        else if constexpr(std::is_enum<T>::value)
        {
            toXml(xml, attributeName, static_cast<typename std::underlying_type<T>::type>(value));
        }
        else if constexpr(is_specialization<T, std::vector>::value || is_specialization<T, std::set>::value)
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
            anlWeakAssert(xml.hasAttribute(attributeName));
            return xml.getStringAttribute(attributeName, defaultValue);
        }
        else if constexpr(std::is_same<T, juce::Colour>::value)
        {
            anlWeakAssert(xml.hasAttribute(attributeName));
            return juce::Colour::fromString(xml.getStringAttribute(attributeName, defaultValue.toString()));
        }
        else if constexpr(std::is_same<T, float>::value)
        {
            anlWeakAssert(xml.hasAttribute(attributeName));
            return static_cast<float>(xml.getDoubleAttribute(attributeName, static_cast<double>(defaultValue)));
        }
        else if constexpr(std::is_enum<T>::value)
        {
            anlWeakAssert(xml.hasAttribute(attributeName));
            return static_cast<T>(fromXml(xml, attributeName, static_cast<typename std::underlying_type<T>::type>(defaultValue)));
        }
        else if constexpr(is_specialization<T, std::vector>::value)
        {
            T result;
            using value_type = typename T::value_type;
            for(auto* child = xml.getChildByName(attributeName); child != nullptr; child = child->getNextElementWithTagName(attributeName))
            {
                result.push_back(fromXml(*child, "value", value_type{}));
            }
            return result;
        }
        else if constexpr(is_specialization<T, std::set>::value)
        {
            T result;
            using value_type = typename T::value_type;
            for(auto* child = xml.getChildByName(attributeName); child != nullptr; child = child->getNextElementWithTagName(attributeName))
            {
                result.insert(fromXml(*child, "value", value_type{}));
            }
            return result;
        }
        else if constexpr(is_specialization<T, std::map>::value)
        {
            T result;
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
            using element_type = typename T::element_type;
            auto* child = xml.getChildByName(attributeName);
            if(child != nullptr)
            {
                return std::make_unique<element_type>(fromXml(*child, attributeName, *defaultValue.get()));
            }
            return std::make_unique<element_type>(*defaultValue.get());
        }
        else
        {
            anlWeakAssert(xml.hasAttribute(attributeName));
            if(xml.hasAttribute(attributeName))
            {
                T value;
                std::stringstream stream(xml.getStringAttribute(attributeName).toRawUTF8());
                stream >> value;
                return value;
            }
            return defaultValue;
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
