#pragma once

#include "../../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    namespace Result
    {
        struct FileInfo
        {
            juce::File file;
            juce::String commit; // unique identifier that is used for versioning

            FileInfo(juce::File const& f = {});

            bool operator==(FileInfo const& rhs) const noexcept;
            bool operator!=(FileInfo const& rhs) const noexcept;
            bool isEmpty() const noexcept;
        };

        void to_json(nlohmann::json& j, FileInfo const& file);
        void from_json(nlohmann::json const& j, FileInfo& file);

        struct FileDescription
        {
            // clang-format off
            enum class Format
            {
                  binary
                , csv
                , lab
                , json
                , cue
                , reaper
                , puredata
                , max
                , sdif
            };
            
            enum class ColumnSeparator
            {
                  comma
                , space
                , tab
                , pipe
                , slash
                , colon
            };
            
            enum class ReaperType
            {
                  marker
                , region
            };
            // clang-format on

            static char toChar(ColumnSeparator const& separator);
            static ColumnSeparator toColumnSeparator(char const& separator);

            juce::File file;
            Format format{Format::binary};
            juce::String extension{".dat"};
            bool includeHeaderRow{false};
            ReaperType reaperType{ReaperType::marker};
            ColumnSeparator columnSeparator{ColumnSeparator::comma};
            bool disableLabelEscaping{false};
            bool includeDescription{true};
            juce::String sdifFrameSignature{"????"};
            juce::String sdifMatrixSignature{"????"};
            nlohmann::json extra; // parsed values (mainly for restoring track attributes from JSON) / not saved

            FileDescription(juce::File const& f = {});

            bool operator==(FileDescription const& rhs) const noexcept;
            bool operator!=(FileDescription const& rhs) const noexcept;
            bool isEmpty() const noexcept;
        };

        void to_json(nlohmann::json& j, FileDescription const& file);
        void from_json(nlohmann::json const& j, FileDescription& file);
    } // namespace Result
} // namespace Track

namespace XmlParser
{
    template <>
    void toXml<Track::Result::FileInfo>(juce::XmlElement& xml, juce::Identifier const& attributeName, Track::Result::FileInfo const& value);

    template <>
    auto fromXml<Track::Result::FileInfo>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Track::Result::FileInfo const& defaultValue)
        -> Track::Result::FileInfo;

    template <>
    void toXml<Track::Result::FileDescription>(juce::XmlElement& xml, juce::Identifier const& attributeName, Track::Result::FileDescription const& value);

    template <>
    auto fromXml<Track::Result::FileDescription>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Track::Result::FileDescription const& defaultValue)
        -> Track::Result::FileDescription;
} // namespace XmlParser

ANALYSE_FILE_END
