#pragma once

#include "../Misc/AnlMisc.h"
#include <tinycolormap/tinycolormap.hpp>
#include <vamp-hostsdk/PluginHostAdapter.h>
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wzero-as-null-pointer-constant")
#include <vamp-hostsdk/PluginInputDomainAdapter.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
#include <vamp-hostsdk/PluginWrapper.h>

ANALYSE_FILE_BEGIN

namespace Plugin
{
    using InputDomain = Vamp::Plugin::InputDomain;
    using WindowType = Vamp::HostExt::PluginInputDomainAdapter::WindowType;

    //! @brief The unique key that corresponds to a specific feature of a specific plugin
    //! @details A plugin is not only defined by a unique identifer to a library but also to
    //! to one of the features of the library.
    struct Key
    {
        std::string identifier; //!< The identifier of the plugin
        std::string feature;    //!< The index of the feature

        inline bool operator<(Key const& rhd) const noexcept
        {
            return (identifier + feature) < (rhd.identifier + rhd.feature);
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

    void to_json(nlohmann::json& j, Key const& key);
    void from_json(nlohmann::json const& j, Key& key);

    //! @brief The description of the output of a plugin
    //! @details The structure describes the results returned by a plugin and how to interprete
    //! them but it doesn't contain any effective result returned by the plugin.
    struct Output
    : public Vamp::Plugin::OutputDescriptor
    {
        using Vamp::Plugin::OutputDescriptor::OutputDescriptor;
        Output(Vamp::Plugin::OutputDescriptor const& d)
        : Vamp::Plugin::OutputDescriptor(d)
        {
        }

        bool operator==(Output const& rhd) const noexcept;
        inline bool operator!=(Output const& rhd) const noexcept
        {
            return !(*this == rhd);
        }
    };

    void to_json(nlohmann::json& j, Output const& output);
    void from_json(nlohmann::json const& j, Output& output);

    //! @brief The description of a parameter of a plugin
    //! @details The structure describes the parameters accepted by a plugin and how to
    //! represent and control them but it doesn't contain any effective value of the state of the
    //! the plugin.
    struct Parameter
    : public Vamp::Plugin::ParameterDescriptor
    {
        using Vamp::Plugin::ParameterDescriptor::ParameterDescriptor;
        Parameter(Vamp::Plugin::ParameterDescriptor const& d)
        : Vamp::Plugin::ParameterDescriptor(d)
        {
        }

        bool operator==(Parameter const& rhd) const noexcept;
        inline bool operator!=(Parameter const& rhd) const noexcept
        {
            return !(*this == rhd);
        }
    };

    void to_json(nlohmann::json& j, Parameter const& parameter);
    void from_json(nlohmann::json const& j, Parameter& parameter);

    //! @brief The state of a plugin
    //! @details The type of data returned by a plugin.
    struct State
    {
        size_t blockSize{0_z};                            //!< The default block size (or window size for frequency domain plugins)
        size_t stepSize{0_z};                             //!< The default step size
        WindowType windowType{WindowType::HanningWindow}; //!< The window type for frequency domain plugins
        std::map<std::string, float> parameters{};        //!< The values of the parameters of the plugin

        inline bool operator==(State const& rhd) const noexcept
        {
            return blockSize == rhd.blockSize &&
                   (stepSize == 0_z || rhd.stepSize == 0_z || stepSize == rhd.stepSize) &&
                   windowType == rhd.windowType &&
                   parameters == rhd.parameters;
        }

        inline bool operator!=(State const& rhd) const noexcept
        {
            return !(*this == rhd);
        }
    };

    void to_json(nlohmann::json& j, State const& state);
    void from_json(nlohmann::json const& j, State& state);

    //! @brief The full description of a plugin
    //! @details The structure contains all the informations to represent and to describe how to control
    //! a plugin but it doesn't contain any control data or any result.
    struct Description
    {
        juce::String name{};                              //!< The name of the plugin
        InputDomain inputDomain{InputDomain::TimeDomain}; //!< The input domain of the plugin
        juce::String maker{};                             //!< The maker of the plugin
        unsigned int version{0};                          //!< The version of the plugin
        juce::String category{};                          //!< The category of the plugin
        juce::String details{};                           //!< Further information about the plugin

        State defaultState;                      //!< The default state of the plugin
        std::vector<Parameter> parameters{};     //!< The parameters of the plugin
        Output output{};                         //!< The output of the plugin
        std::map<std::string, State> programs{}; //!< The program of the plugin

        inline bool operator==(Description const& rhd) const noexcept
        {
            return name == rhd.name &&
                   inputDomain == rhd.inputDomain &&
                   maker == rhd.maker &&
                   version == rhd.version &&
                   category == rhd.category &&
                   details == rhd.details &&
                   defaultState == rhd.defaultState &&
                   parameters == rhd.parameters &&
                   output == rhd.output &&
                   programs == rhd.programs;
        }

        inline bool operator!=(Description const& rhd) const noexcept
        {
            return !(*this == rhd);
        }
    };

    void to_json(nlohmann::json& j, Description const& description);
    void from_json(nlohmann::json const& j, Description& description);

    //! @brief The result a plugin
    //! @details The type of data returned by a plugin.
    struct Result
    : public Vamp::Plugin::Feature
    {
        using Vamp::Plugin::Feature::Feature;
        Result(Feature const& feature)
        : Vamp::Plugin::Feature(feature)
        {
        }

        inline bool operator==(Result const& rhd) const noexcept
        {
            return hasTimestamp == rhd.hasTimestamp &&
                   timestamp == rhd.timestamp &&
                   hasDuration == rhd.hasDuration &&
                   duration == rhd.duration &&
                   values.size() == rhd.values.size() &&
                   std::equal(values.cbegin(), values.cend(), rhd.values.cbegin()) &&
                   label == rhd.label;
        }

        inline bool operator!=(Result const& rhd) const noexcept
        {
            return !(*this == rhd);
        }
    };
} // namespace Plugin

namespace XmlParser
{
    template <>
    void toXml<Plugin::Key>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::Key const& value);

    template <>
    auto fromXml<Plugin::Key>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::Key const& defaultValue)
        -> Plugin::Key;

    template <>
    void toXml<Plugin::Parameter>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::Parameter const& value);

    template <>
    auto fromXml<Plugin::Parameter>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::Parameter const& defaultValue)
        -> Plugin::Parameter;

    template <>
    void toXml<Plugin::Output>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::Output const& value);

    template <>
    auto fromXml<Plugin::Output>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::Output const& defaultValue)
        -> Plugin::Output;

    template <>
    void toXml<Plugin::Description>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::Description const& value);

    template <>
    auto fromXml<Plugin::Description>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::Description const& defaultValue)
        -> Plugin::Description;

    template <>
    void toXml<Plugin::State>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::State const& value);

    template <>
    auto fromXml<Plugin::State>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::State const& defaultValue)
        -> Plugin::State;
} // namespace XmlParser

ANALYSE_FILE_END
