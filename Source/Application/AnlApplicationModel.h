#pragma once

#include "../Document/AnlDocumentExporter.h"
#include "AnlApplicationLookAndFeel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    using Flag = Model::Flag;

    // clang-format off
    enum class ColourMode
    {
          night
        , day
        , grass
        , automatic
    };

    enum class AttrType : size_t
    {
          desktopGlobalScaleFactor
        , windowState
        , recentlyOpenedFilesList
        , currentDocumentFile
        , colourMode
        , showInfoBubble
        , exportOptions
        , adaptationToSampleRate
        , autoLoadConvertedFile
    };

    using AttrContainer = Model::Container
    < Model::Attr<AttrType::desktopGlobalScaleFactor, float, Flag::basic>
    , Model::Attr<AttrType::windowState, juce::String, Flag::basic>
    , Model::Attr<AttrType::recentlyOpenedFilesList, std::vector<juce::File>, Flag::basic>
    , Model::Attr<AttrType::currentDocumentFile, juce::File, Flag::basic>
    , Model::Attr<AttrType::colourMode, ColourMode, Flag::basic>
    , Model::Attr<AttrType::showInfoBubble, bool, Flag::basic>
    , Model::Attr<AttrType::exportOptions, Document::Exporter::Options, Flag::basic>
    , Model::Attr<AttrType::adaptationToSampleRate, bool, Flag::basic>
    , Model::Attr<AttrType::autoLoadConvertedFile, bool, Flag::basic>
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
              {1.0f}
            , {juce::String{}}
            , {std::vector<juce::File>{}}
            , {juce::File{}}
            , {ColourMode::automatic}
            , {true}
            , {}
            , {false}
            , {true}
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

                Model::Accessor<Accessor, AttrContainer>::setAttr<AttrType::recentlyOpenedFilesList, std::vector<juce::File>>(sanitize(value), notification);
            }
            else if constexpr(type == AttrType::desktopGlobalScaleFactor)
            {
                Model::Accessor<Accessor, AttrContainer>::setAttr<AttrType::desktopGlobalScaleFactor, value_v>(std::clamp(value, 1.0f, 2.0f), notification);
            }
            else
            {
                Model::Accessor<Accessor, AttrContainer>::setAttr<type, value_v>(value, notification);
            }
        }
    };
} // namespace Application

ANALYSE_FILE_END
