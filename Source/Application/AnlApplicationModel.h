#pragma once

#include "../Document/AnlDocumentExporter.h"
#include "../Track/AnlTrackModel.h"
#include "AnlApplicationLookAndFeel.h"
#include "AnlApplicationOsc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
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
        , defaultTemplateFile
        , currentTranslationFile
        , colourMode
        , showInfoBubble
        , exportOptions
        , adaptationToSampleRate
        , autoLoadConvertedFile
        , silentFileManagement
        , routingMatrix
        , autoUpdate
        , lastVersion
        , timeZoomAnchorOnPlayhead
        , globalGraphicPreset
    };
    
    enum class AcsrType : size_t
    {
          osc
    };

    using AttrContainer = Model::Container
    < Model::Attr<AttrType::desktopGlobalScaleFactor, float, Model::Flag::basic>
    , Model::Attr<AttrType::windowState, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::recentlyOpenedFilesList, std::vector<juce::File>, Model::Flag::basic>
    , Model::Attr<AttrType::currentDocumentFile, juce::File, Model::Flag::basic>
    , Model::Attr<AttrType::defaultTemplateFile, juce::File, Model::Flag::basic>
    , Model::Attr<AttrType::currentTranslationFile, juce::File, Model::Flag::basic>
    , Model::Attr<AttrType::colourMode, ColourMode, Model::Flag::basic>
    , Model::Attr<AttrType::showInfoBubble, bool, Model::Flag::basic>
    , Model::Attr<AttrType::exportOptions, Document::Exporter::Options, Model::Flag::basic>
    , Model::Attr<AttrType::adaptationToSampleRate, bool, Model::Flag::basic>
    , Model::Attr<AttrType::autoLoadConvertedFile, bool, Model::Flag::basic>
    , Model::Attr<AttrType::silentFileManagement, bool, Model::Flag::basic>
    , Model::Attr<AttrType::routingMatrix, std::vector<std::vector<bool>>, Model::Flag::basic>
    , Model::Attr<AttrType::autoUpdate, bool, Model::Flag::basic>
    , Model::Attr<AttrType::lastVersion, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::timeZoomAnchorOnPlayhead, bool, Model::Flag::basic>
    , Model::Attr<AttrType::globalGraphicPreset, Track::GraphicsSettings, Model::Flag::basic>
    >;
    
    using AcsrContainer = Model::Container
    < Model::Acsr<AcsrType::osc, Osc::Accessor, Model::Flag::saveable | Model::Flag::notifying, 1>
    >;
    // clang-format on

    class Accessor
    : public Model::Accessor<Accessor, AttrContainer, AcsrContainer>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer, AcsrContainer>::Accessor;
        // clang-format off
        Accessor()
        : Accessor(AttrContainer(
        {
              {1.0f}
            , {juce::String{}}
            , {std::vector<juce::File>{}}
            , {juce::File{}}
            , {getFactoryTemplateFile()}
            , {juce::File{}}
            , {ColourMode::automatic}
            , {true}
            , {}
            , {false}
            , {true}
            , {true}
            , {{}}
            , {true}
            , {ProjectInfo::versionString}
            , {false}
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
                setRecentlyOpenedFilesList(value, notification);
            }
            else if constexpr(type == AttrType::desktopGlobalScaleFactor)
            {
                setDesktopGlobalScaleFactor(value, notification);
            }
            else
            {
                Model::Accessor<Accessor, AttrContainer, AcsrContainer>::setAttr<type, value_v>(value, notification);
            }
        }

        std::unique_ptr<juce::XmlElement> parseXml(juce::XmlElement const& xml, int version) override;
        void setRecentlyOpenedFilesList(std::vector<juce::File> const& value, NotificationType notification);
        void setDesktopGlobalScaleFactor(float const& value, NotificationType notification);
        static juce::File getFactoryTemplateFile();
    };
} // namespace Application

ANALYSE_FILE_END
