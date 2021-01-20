
#pragma once

#include "../Tools/AnlModel.h"
#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginWrapper.h>

ANALYSE_FILE_BEGIN

namespace Plugin
{
    struct Key
    {
        std::string identifier; //!< The identifier of the plugin
        std::string feature;    //!< The index of the feature
        
        inline bool operator<(Key const& rhd) const noexcept
        {
            return identifier < rhd.identifier || feature < rhd.feature;
        }
        
        inline bool operator==(Key const& rhd) const noexcept
        {
            return identifier == rhd.identifier && feature == rhd.feature;
        }
        
        inline bool operator!=(Key const& rhd) const noexcept
        {
            return !(*this == rhd);
        }
    };
    
    struct Output
    : public Vamp::Plugin::OutputDescriptor
    {
        using Vamp::Plugin::OutputDescriptor::OutputDescriptor;
        Output(Vamp::Plugin::OutputDescriptor const& d)
        : Vamp::Plugin::OutputDescriptor(d)
        {
        }

        bool operator==(Output const& rhd) const  noexcept;
        inline bool operator!=(Output const& rhd) const  noexcept
        {
            return !(*this == rhd);
        }
    };
    
    struct Parameter
    : public Vamp::Plugin::ParameterDescriptor
    {
        using Vamp::Plugin::ParameterDescriptor::ParameterDescriptor;
        Parameter(Vamp::Plugin::ParameterDescriptor& d)
        : Vamp::Plugin::ParameterDescriptor(d)
        {
        }

        bool operator==(Parameter const& rhd) const noexcept;
        inline bool operator!=(Parameter const& rhd) const noexcept
        {
            return !(*this == rhd);
        }
    };
    
    struct Result
    : public Vamp::Plugin::Feature
    {
        using Vamp::Plugin::Feature::Feature;
        Result(Feature const& feature)
        : Vamp::Plugin::Feature(feature)
        {
        }
        
        inline bool operator==(Result const& other) const noexcept
        {
            return hasTimestamp == other.hasTimestamp &&
            timestamp == other.timestamp &&
            hasDuration == other.hasDuration &&
            duration == other.duration &&
            values.size() == other.values.size() &&
            std::equal(values.cbegin(), values.cend(), other.values.cbegin()) &&
            label == other.label;
        }
        
        inline bool operator!=(Result const& rhd) const noexcept
        {
            return !(*this == rhd);
        }
    };
    
    struct Description
    {
        juce::String name {};               //!< The name of the plugin
        juce::String specialization {};      //!< The feature specialization of the plugin
        juce::String maker {};              //!< The maker of the plugin
        unsigned int api {0};               //!< The API version used by the plugin
        std::set<juce::String> categories;  //!< The categories of the plugin
        juce::String details;               //!< Further information about the plugin
        
        inline bool operator==(Description const& rhd) const noexcept
        {
            return name == rhd.name &&
            maker == rhd.maker &&
            api == rhd.api &&
            categories == rhd.categories &&
            details == rhd.details;
        }
        
        inline bool operator!=(Description const& rhd) const noexcept
        {
            return !(*this == rhd);
        }
    };
}

namespace XmlParser
{
    template<>
    void toXml<Plugin::Key>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::Key const& value);
    
    template<>
    auto fromXml<Plugin::Key>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::Key const& defaultValue)
    -> Plugin::Key;
    
    template<>
    void toXml<Plugin::Output>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::Output const& value);
    
    template<>
    auto fromXml<Plugin::Output>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::Output const& defaultValue)
    -> Plugin::Output;
    
    template<>
    void toXml<Plugin::Description>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::Description const& value);

    template<>
    auto fromXml<Plugin::Description>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::Description const& defaultValue)
    -> Plugin::Description;
}

ANALYSE_FILE_END
