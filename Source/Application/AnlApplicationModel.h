#pragma once

#include "../Misc/AnlMisc.h"
#include "AnlApplicationLookAndFeel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    using Flag = Model::Flag;

    struct ExportOptions
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

        inline bool operator==(ExportOptions const& rhd) const noexcept
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

        inline bool operator!=(ExportOptions const& rhd) const noexcept
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

    // clang-format off
    enum class AttrType : size_t
    {
          windowState
        , recentlyOpenedFilesList
        , currentDocumentFile
        , colourMode
        , showInfoBubble
        , exportOptions
    };

    using AttrContainer = Model::Container
    < Model::Attr<AttrType::windowState, juce::String, Flag::basic>
    , Model::Attr<AttrType::recentlyOpenedFilesList, std::vector<juce::File>, Flag::basic>
    , Model::Attr<AttrType::currentDocumentFile, juce::File, Flag::basic>
    , Model::Attr<AttrType::colourMode, LookAndFeel::ColourChart::Mode, Flag::basic>
    , Model::Attr<AttrType::showInfoBubble, bool, Flag::basic>
    , Model::Attr<AttrType::exportOptions, ExportOptions, Flag::basic>
    >;
    // clang-format on

    class Accessor
    : public Model::Accessor<Accessor, AttrContainer>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer>::Accessor;
        // clang-format off
        Accessor()
        : Accessor(AttrContainer(
        {
              {juce::String{}}
            , {std::vector<juce::File>{}}
            , {juce::File{}}
            , {LookAndFeel::ColourChart::Mode::night}
            , {true}
            , {}
        }))
        {
        }
        // clang-format on

        template <attr_enum_type type, typename value_v>
        void setAttr(value_v const& value, NotificationType notification)
        {
            if constexpr(type == AttrType::recentlyOpenedFilesList)
            {
                auto sanitize = [](std::vector<juce::File> const& files)
                {
                    std::set<juce::File> duplicates;
                    std::vector<juce::File> copy;
                    for(auto const& file : files)
                    {
                        if(file.existsAsFile() && duplicates.insert(file).second)
                        {
                            copy.push_back(file);
                        }
                    }
                    return copy;
                };

                Anl::Model::Accessor<Accessor, AttrContainer>::setAttr<AttrType::recentlyOpenedFilesList, std::vector<juce::File>>(sanitize(value), notification);
            }
            else
            {
                Model::Accessor<Accessor, AttrContainer>::setAttr<type, value_v>(value, notification);
            }
        }
    };
} // namespace Application

namespace XmlParser
{
    template <>
    void toXml<Application::ExportOptions>(juce::XmlElement& xml, juce::Identifier const& attributeName, Application::ExportOptions const& value);

    template <>
    auto fromXml<Application::ExportOptions>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Application::ExportOptions const& defaultValue)
        -> Application::ExportOptions;
} // namespace XmlParser

ANALYSE_FILE_END
