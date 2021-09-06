#pragma once

#include "AnlBasics.h"

namespace nlohmann
{
    template <>
    struct adl_serializer<juce::String>
    {
        static void to_json(json& j, juce::String const& string)
        {
            j = string.toStdString();
        }

        static void from_json(json const& j, juce::String& string)
        {
            string = juce::String(j.get<std::string>());
        }
    };

    template <>
    struct adl_serializer<juce::Colour>
    {
        static void to_json(json& j, juce::Colour const& colour)
        {
            j = colour.toString().toStdString();
        }

        static void from_json(json const& j, juce::Colour& colour)
        {
            colour = juce::Colour::fromString(juce::String(j.get<std::string>()));
        }
    };

    template <>
    struct adl_serializer<juce::File>
    {
        static void to_json(json& j, juce::File const& file)
        {
            j = file.getFullPathName().toStdString();
        }

        static void from_json(json const& j, juce::File& file)
        {
            file = juce::File(juce::String(j.get<std::string>()));
        }
    };

    template <typename T>
    struct adl_serializer<juce::Range<T>>
    {
        static void to_json(json& j, juce::Range<T> const& range)
        {
            j["start"] = range.getStart();
            j["end"] = range.getEnd();
        }

        static void from_json(json const& j, juce::Range<T>& range)
        {
            T start, end;
            j.at("start").get_to(start);
            j.at("end").get_to(end);
            range = {start, end};
        }
    };
} // namespace nlohmann
