#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

namespace XmlParser
{
    template <typename T>
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
        else if constexpr(std::is_same<T, std::string>::value)
        {
            xml.setAttribute(attributeName, juce::String(value));
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
        else if constexpr(is_specialization<T, std::pair>::value)
        {
            auto child = std::make_unique<juce::XmlElement>(attributeName);
            toXml(*child.get(), "key", value.first);
            toXml(*child.get(), "value", value.second);
            xml.addChildElement(child.release());
        }
        else if constexpr(is_specialization<T, std::vector>::value || is_specialization<T, std::set>::value)
        {
            while(auto* child = xml.getChildByName(attributeName))
            {
                xml.removeChildElement(child, true);
            }
            auto child = std::make_unique<juce::XmlElement>(attributeName);
            if constexpr(std::is_lvalue_reference<decltype(*value.cbegin())>::value)
            {
                for(auto const& element : value)
                {
                    auto subChild = std::make_unique<juce::XmlElement>(attributeName);
                    toXml(*subChild.get(), "value", element);
                    child->addChildElement(subChild.release());
                }
            }
            else
            {
                for(auto const element : value)
                {
                    auto subChild = std::make_unique<juce::XmlElement>(attributeName);
                    toXml(*subChild.get(), "value", element);
                    child->addChildElement(subChild.release());
                }
            }
            xml.addChildElement(child.release());
        }
        else if constexpr(is_specialization<T, std::map>::value)
        {
            while(auto* child = xml.getChildByName(attributeName))
            {
                xml.removeChildElement(child, true);
            }
            auto child = std::make_unique<juce::XmlElement>(attributeName);
            for(auto const& element : value)
            {
                toXml(*child.get(), attributeName, element);
            }
            xml.addChildElement(child.release());
        }
        else if constexpr(is_specialization<T, std::unique_ptr>::value)
        {
            while(auto* child = xml.getChildByName(attributeName))
            {
                xml.removeChildElement(child, true);
            }
            if(value != nullptr)
            {
                auto child = std::make_unique<juce::XmlElement>(attributeName);
                toXml(*child, attributeName, *value.get());
                xml.addChildElement(child.release());
            }
        }
        else if constexpr(is_specialization<T, std::optional>::value)
        {
            while(auto* child = xml.getChildByName(attributeName))
            {
                xml.removeChildElement(child, true);
            }
            if(value.has_value())
            {
                auto child = std::make_unique<juce::XmlElement>(attributeName);
                toXml(*child, attributeName, *value);
                xml.addChildElement(child.release());
            }
        }
        else
        {
            std::ostringstream stream;
            stream << value;
            xml.setAttribute(attributeName, juce::String(stream.str()));
        }
    }

    template <typename T>
    auto fromXml(juce::XmlElement const& xml, juce::Identifier const& attributeName, T const& defaultValue)
        -> T
    {
        if constexpr(std::is_same<T, int>::value)
        {
            MiscWeakAssert(xml.hasAttribute(attributeName));
            return xml.getIntAttribute(attributeName, defaultValue);
        }
        else if constexpr(std::is_same<T, double>::value)
        {
            MiscWeakAssert(xml.hasAttribute(attributeName));
            return xml.getDoubleAttribute(attributeName, defaultValue);
        }
        else if constexpr(std::is_same<T, juce::String>::value)
        {
            MiscWeakAssert(xml.hasAttribute(attributeName));
            return xml.getStringAttribute(attributeName, defaultValue);
        }
        else if constexpr(std::is_same<T, std::string>::value)
        {
            MiscWeakAssert(xml.hasAttribute(attributeName));
            return xml.getStringAttribute(attributeName, juce::String(defaultValue)).toStdString();
        }
        else if constexpr(std::is_same<T, juce::Colour>::value)
        {
            MiscWeakAssert(xml.hasAttribute(attributeName));
            return juce::Colour::fromString(xml.getStringAttribute(attributeName, defaultValue.toString()));
        }
        else if constexpr(std::is_same<T, float>::value)
        {
            MiscWeakAssert(xml.hasAttribute(attributeName));
            return static_cast<float>(xml.getDoubleAttribute(attributeName, static_cast<double>(defaultValue)));
        }
        else if constexpr(std::is_enum<T>::value)
        {
            MiscWeakAssert(xml.hasAttribute(attributeName));
            return static_cast<T>(fromXml(xml, attributeName, static_cast<typename std::underlying_type<T>::type>(defaultValue)));
        }
        else if constexpr(is_specialization<T, std::pair>::value)
        {
            using first_type = typename T::first_type;
            using second_type = typename T::second_type;
            if(auto* child = xml.getChildByName(attributeName))
            {
                return std::make_pair(fromXml(*child, "key", first_type{}), fromXml(*child, "value", second_type{}));
            }
            return std::make_pair(first_type{}, second_type{});
        }
        else if constexpr(is_specialization<T, std::vector>::value)
        {
            if(auto* child = xml.getChildByName(attributeName))
            {
                T result;
                using value_type = typename T::value_type;
                for(auto* subChild = child->getChildByName(attributeName); subChild != nullptr; subChild = subChild->getNextElementWithTagName(attributeName))
                {
                    result.push_back(fromXml(*subChild, "value", value_type{}));
                }
                return result;
            }
            else
            {
                return defaultValue;
            }
        }
        else if constexpr(is_specialization<T, std::set>::value)
        {
            if(auto* child = xml.getChildByName(attributeName))
            {
                T result;
                using value_type = typename T::value_type;
                for(auto* subChild = child->getChildByName(attributeName); subChild != nullptr; subChild = subChild->getNextElementWithTagName(attributeName))
                {
                    result.insert(fromXml(*subChild, "value", value_type{}));
                }
                return result;
            }
            else
            {
                return defaultValue;
            }
        }
        else if constexpr(is_specialization<T, std::map>::value)
        {
            if(auto* child = xml.getChildByName(attributeName))
            {
                T result;
                using key_type = typename T::key_type;
                using mapped_type = typename T::mapped_type;
                for(auto* subChild = child->getChildByName(attributeName); subChild != nullptr; subChild = subChild->getNextElementWithTagName(attributeName))
                {
                    auto const entry = fromXml(*subChild, "key", key_type{});
                    result[entry] = fromXml(*subChild, "value", mapped_type{});
                }
                return result;
            }
            else
            {
                return defaultValue;
            }
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
        else if constexpr(is_specialization<T, std::optional>::value)
        {
            using value_type = typename T::value_type;
            auto* child = xml.getChildByName(attributeName);
            if(child != nullptr)
            {
                return std::optional<value_type>(fromXml(*child, attributeName, defaultValue.has_value() ? *defaultValue : value_type{}));
            }
            return std::optional<value_type>{};
        }
        else
        {
            MiscWeakAssert(xml.hasAttribute(attributeName));
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

    template <>
    void toXml<juce::Range<double>>(juce::XmlElement& xml, juce::Identifier const& attributeName, juce::Range<double> const& value);

    template <>
    auto fromXml<juce::Range<double>>(juce::XmlElement const& xml, juce::Identifier const& attributeName, juce::Range<double> const& defaultValue)
        -> juce::Range<double>;

    template <>
    void toXml<juce::Range<int>>(juce::XmlElement& xml, juce::Identifier const& attributeName, juce::Range<int> const& value);

    template <>
    auto fromXml<juce::Range<int>>(juce::XmlElement const& xml, juce::Identifier const& attributeName, juce::Range<int> const& defaultValue)
        -> juce::Range<int>;

    template <>
    void toXml<juce::File>(juce::XmlElement& xml, juce::Identifier const& attributeName, juce::File const& value);

    template <>
    auto fromXml<juce::File>(juce::XmlElement const& xml, juce::Identifier const& attributeName, juce::File const& defaultValue)
        -> juce::File;

    template <>
    void toXml<juce::Font>(juce::XmlElement& xml, juce::Identifier const& attributeName, juce::Font const& value);

    template <>
    auto fromXml<juce::Font>(juce::XmlElement const& xml, juce::Identifier const& attributeName, juce::Font const& defaultValue)
        -> juce::Font;

    template <>
    void toXml<juce::FontOptions>(juce::XmlElement& xml, juce::Identifier const& attributeName, juce::FontOptions const& value);

    template <>
    auto fromXml<juce::FontOptions>(juce::XmlElement const& xml, juce::Identifier const& attributeName, juce::FontOptions const& defaultValue)
        -> juce::FontOptions;

    template <>
    void toXml<juce::Point<int>>(juce::XmlElement& xml, juce::Identifier const& attributeName, juce::Point<int> const& value);

    template <>
    auto fromXml<juce::Point<int>>(juce::XmlElement const& xml, juce::Identifier const& attributeName, juce::Point<int> const& defaultValue)
        -> juce::Point<int>;

    template <>
    void toXml<juce::StringPairArray>(juce::XmlElement& xml, juce::Identifier const& attributeName, juce::StringPairArray const& value);

    template <>
    auto fromXml<juce::StringPairArray>(juce::XmlElement const& xml, juce::Identifier const& attributeName, juce::StringPairArray const& defaultValue)
        -> juce::StringPairArray;

    //! @brief Replaces all occurrences of a specific attribute value in the XML element with a new value.
    void replaceAllAttributeValues(juce::XmlElement& element, juce::String const& previousValue, juce::String const& newValue);
} // namespace XmlParser

MISC_FILE_END
