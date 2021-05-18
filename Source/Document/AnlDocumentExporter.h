#pragma once

#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    namespace Exporter
    {
        struct Options
        {
            // clang-format off
            enum class Format
            {
                  jpeg
                , png
                , csv
                , json
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
            // clang-format on

            Format format{Format::jpeg};
            bool useGroupOverview{false};
            bool useAutoSize{false};
            int imageWidth{1920};
            int imageHeight{1200};
            bool includeHeaderRaw{true};
            bool ignoreGridResults{true};
            ColumnSeparator columnSeparator{ColumnSeparator::comma};

            inline bool operator==(Options const& rhd) const noexcept
            {
                return format == rhd.format &&
                       useGroupOverview == rhd.useGroupOverview &&
                       useAutoSize == rhd.useAutoSize &&
                       imageWidth == rhd.imageWidth &&
                       imageHeight == rhd.imageHeight &&
                       includeHeaderRaw == rhd.includeHeaderRaw &&
                       ignoreGridResults == rhd.ignoreGridResults &&
                       columnSeparator == rhd.columnSeparator;
            }

            inline bool operator!=(Options const& rhd) const noexcept
            {
                return !(*this == rhd);
            }

            inline bool useImageFormat() const noexcept
            {
                return format == Format::jpeg || format == Format::png;
            }

            inline bool useTextFormat() const noexcept
            {
                return !useImageFormat();
            }

            juce::String getFormatName() const
            {
                return juce::String(std::string(magic_enum::enum_name(format)));
            }

            juce::String getFormatExtension() const
            {
                return getFormatName().toLowerCase();
            }

            juce::String getFormatWilcard() const
            {
                if(format == Format::jpeg)
                {
                    return "*.jpeg;*.jpg";
                }
                return "*." + getFormatExtension();
            }

            char getSeparatorChar() const
            {
                switch(columnSeparator)
                {
                    case ColumnSeparator::comma:
                    {
                        return ';';
                    }
                    case ColumnSeparator::space:
                    {
                        return ' ';
                    }
                    case ColumnSeparator::tab:
                    {
                        return '\t';
                    }
                    case ColumnSeparator::pipe:
                    {
                        return '|';
                    }
                    case ColumnSeparator::slash:
                    {
                        return '/';
                    }
                    case ColumnSeparator::colon:
                    {
                        return ':';
                    }
                    default:
                    {
                        return ';';
                    }
                }
            }
        };

        juce::Result toFile(Accessor& accessor, juce::File const file, juce::String const& identifier, Options const& options, std::function<std::pair<int, int>(juce::String const& identifier)> getSizeFor = nullptr);
    } // namespace Exporter
} // namespace Document

namespace XmlParser
{
    template <>
    void toXml<Document::Exporter::Options>(juce::XmlElement& xml, juce::Identifier const& attributeName, Document::Exporter::Options const& value);

    template <>
    auto fromXml<Document::Exporter::Options>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Document::Exporter::Options const& defaultValue)
        -> Document::Exporter::Options;
} // namespace XmlParser

ANALYSE_FILE_END
