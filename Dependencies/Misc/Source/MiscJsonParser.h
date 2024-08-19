#pragma once

#include "MiscBasics.h"

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

    template <>
    struct adl_serializer<juce::Font>
    {
        static void to_json(json& j, juce::Font const& font)
        {
            j = font.toString().toStdString();
        }

        static void from_json(json const& j, juce::Font& font)
        {
            font = juce::Font::fromString(juce::String(j.get<std::string>()));
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
            range = {j.value("start", T(0)), j.value("end", T(0))};
        }
    };

    template <>
    struct adl_serializer<juce::StringPairArray>
    {
        static void to_json(json& j, juce::StringPairArray const& stringPairArray)
        {
            auto const& keys = stringPairArray.getAllKeys();
            auto const& values = stringPairArray.getAllValues();
            for(auto i = 0; i < std::min(keys.size(), values.size()); ++i)
            {
                j[keys[i].toStdString()] = values[i].toStdString();
            }
        }

        static void from_json(json const& j, juce::StringPairArray& stringPairArray)
        {
            for(auto const& [key, value] : j.items())
            {
                stringPairArray.set(key, value);
            }
        }
    };

    template <>
    struct adl_serializer<std::optional<double>>
    {
        static void to_json(json& j, std::optional<double> const& value)
        {
            if(value.has_value())
            {
                j = value.value();
            }
            else
            {
                j.clear();
            }
        }

        static void from_json(json const& j, std::optional<double>& value)
        {
            if(j.empty())
            {
                value.reset();
            }
            else
            {
                value = j.get<double>();
            }
        }
    };

    template <>
    struct adl_serializer<std::optional<float>>
    {
        static void to_json(json& j, std::optional<float> const& value)
        {
            if(value.has_value())
            {
                j = value.value();
            }
            else
            {
                j.clear();
            }
        }

        static void from_json(json const& j, std::optional<float>& value)
        {
            if(j.empty())
            {
                value.reset();
            }
            else
            {
                value = j.get<float>();
            }
        }
    };

    template <>
    struct adl_serializer<std::optional<juce::String>>
    {
        static void to_json(json& j, std::optional<juce::String> const& value)
        {
            if(value.has_value())
            {
                j = value.value();
            }
            else
            {
                j.clear();
            }
        }

        static void from_json(json const& j, std::optional<juce::String>& value)
        {
            if(j.empty())
            {
                value.reset();
            }
            else
            {
                value = j.get<juce::String>();
            }
        }
    };

    json sax_parse_json_object(std::istream& stream, std::string const& key, std::size_t level);
} // namespace nlohmann
