#pragma once

#include "../Plugin/AnlPluginModel.h"
#include "Result/AnlTrackResultModel.h"
#include <bitset>
#include <tinycolormap/tinycolormap.hpp>

ANALYSE_FILE_BEGIN

namespace Track
{
    // clang-format off
    enum class FrameType
    {
          label
        , value
        , vector
    };
    // clang-format on

    // clang-format off
    enum class ZoomValueMode
    {
          undefined
        , plugin
        , results
        , custom
    };
    // clang-format on

    using Results = Result::Data;
    using FileInfo = Result::File;
    using Images = std::vector<std::vector<juce::Image>>;
    using ColourMap = tinycolormap::ColormapType;
    using FocusInfo = std::bitset<static_cast<size_t>(512)>;

    struct Edition
    {
        size_t channel;
        Result::ChannelData data;
        juce::Range<double> range;

        inline bool operator==(Edition const& rhd) const noexcept
        {
            return channel == rhd.channel &&
                   data == rhd.data &&
                   range == rhd.range;
        }

        inline bool operator!=(Edition const& rhd) const noexcept
        {
            return !(*this == rhd);
        }
    };

    struct ColourSet
    {
        ColourMap map = ColourMap::Inferno;
        juce::Colour background = juce::Colours::transparentBlack;
        juce::Colour foreground = juce::Colours::black;
        juce::Colour duration = juce::Colours::black.withAlpha(0.4f);
        juce::Colour text = juce::Colours::transparentBlack;
        juce::Colour shadow = juce::Colours::transparentBlack;

        inline bool operator==(ColourSet const& rhd) const noexcept
        {
            return map == rhd.map &&
                   background == rhd.background &&
                   foreground == rhd.foreground &&
                   duration == rhd.duration &&
                   text == rhd.text &&
                   shadow == rhd.shadow;
        }

        inline bool operator!=(ColourSet const& rhd) const noexcept
        {
            return !(*this == rhd);
        }
    };

    void to_json(nlohmann::json& j, ColourSet const& colourSet);
    void from_json(nlohmann::json const& j, ColourSet& colourSet);

    struct LabelLayout
    {
        // clang-format off
        enum class Justification
        {
              top
            , centred
            , bottom
        };
        // clang-format on

        float position = 12.0f;
        Justification justification = Justification::top;

        const auto tie() const
        {
            return std::tie(position, justification);
        }

        bool operator==(LabelLayout const& rhs) const
        {
            return tie() == rhs.tie();
        }

        bool operator!=(LabelLayout const& rhs) const
        {
            return tie() != rhs.tie();
        }

        bool operator>(LabelLayout const& rhs) const
        {
            return tie() > rhs.tie();
        }

        bool operator<(LabelLayout const& rhs) const
        {
            return tie() < rhs.tie();
        }
    };

    void to_json(nlohmann::json& j, LabelLayout const& labelLayout);
    void from_json(nlohmann::json const& j, LabelLayout& labelLayout);

    // clang-format off
    enum class GridMode
    {
          hidden
        , partial
        , full
    };

    enum class WarningType
    {
          none
        , library
        , plugin
        , state
        , file
    };

    enum class AttrType : size_t
    {
          identifier
        , name
        , file
        , results
        , edit
        , description
        , key
        , input
        , state
        
        , height
        , colours
        , font
        , unit
        , labelLayout
        , channelsLayout
        , showInGroup
        , zoomValueMode
        , zoomLink
        , zoomAcsr
        , extraThresholds
        
        , graphics
        , warnings
        , processing
        , focused
        , grid
        , hasPluginColourMap
    };

    enum class AcsrType : size_t
    {
          valueZoom
        , binZoom
    };

    enum class SignalType
    {
          showProperties
        , showTable
    };

    using AttrContainer = Model::Container
    < Model::Attr<AttrType::identifier, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::name, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::file, FileInfo, Model::Flag::basic>
    , Model::Attr<AttrType::results, Results, Model::Flag::notifying>
    , Model::Attr<AttrType::edit, Edition, Model::Flag::notifying>
    , Model::Attr<AttrType::description, Plugin::Description, Model::Flag::basic>
    , Model::Attr<AttrType::key, Plugin::Key, Model::Flag::basic>
    , Model::Attr<AttrType::input, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::state, Plugin::State, Model::Flag::basic>
    
    , Model::Attr<AttrType::height, int, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::colours, ColourSet, Model::Flag::basic>
    , Model::Attr<AttrType::font, juce::Font, Model::Flag::basic>
    , Model::Attr<AttrType::unit, std::optional<juce::String>, Model::Flag::basic>
    , Model::Attr<AttrType::labelLayout, LabelLayout, Model::Flag::basic>
    , Model::Attr<AttrType::channelsLayout, std::vector<bool>, Model::Flag::basic>
    , Model::Attr<AttrType::showInGroup, bool, Model::Flag::basic>
    , Model::Attr<AttrType::zoomValueMode, ZoomValueMode, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::zoomLink, bool, Model::Flag::basic>
    , Model::Attr<AttrType::zoomAcsr, std::optional<std::reference_wrapper<Zoom::Accessor>>, Model::Flag::notifying>
    , Model::Attr<AttrType::extraThresholds, std::vector<std::optional<float>>, Model::Flag::basic>
    
    , Model::Attr<AttrType::graphics, Images, Model::Flag::notifying>
    , Model::Attr<AttrType::warnings, WarningType, Model::Flag::notifying>
    , Model::Attr<AttrType::processing, std::tuple<bool, float, bool, float>, Model::Flag::notifying>
    , Model::Attr<AttrType::focused, FocusInfo, Model::Flag::notifying>
    , Model::Attr<AttrType::grid, GridMode, Model::Flag::notifying>
    , Model::Attr<AttrType::hasPluginColourMap, bool, Model::Flag::notifying>
    >;
    
    using AcsrContainer = Model::Container
    < Model::Acsr<AcsrType::valueZoom, Zoom::Accessor, Model::Flag::saveable | Model::Flag::notifying, 1>
    , Model::Acsr<AcsrType::binZoom, Zoom::Accessor, Model::Flag::saveable | Model::Flag::notifying, 1>
    >;
    // clang-format on

    class Accessor
    : public Model::Accessor<Accessor, AttrContainer, AcsrContainer>
    , public Broadcaster<Accessor, SignalType>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer, AcsrContainer>::Accessor;

        // clang-format off
        Accessor()
        : Accessor(AttrContainer(  {""}
                                 , {""}
                                 , {}
                                 , {}
                                 , {}
                                 , {}
                                 , {}
                                 , {}
                                 , {}
                                 
                                 , {120}
                                 , {}
                                 , {juce::Font("Nunito Sans", 14.0f, juce::Font::plain)}
                                 , {}
                                 , {}
                                 , {std::vector<bool>{}}
                                 , {true}
                                 , {ZoomValueMode::undefined}
                                 , {true}
                                 , {}
                                 , {}
                                 
                                 , {}
                                 , {WarningType::none}
                                 , {{false, 0.0, false, 0.0}}
                                 , {}
                                 , {GridMode::partial}
                                 , {false}
                                 ))
        {
        }
        // clang-format on

        template <attr_enum_type type, typename value_v>
        void setAttr(value_v const& value, NotificationType notification)
        {
            if constexpr(type == AttrType::channelsLayout)
            {
                if(!value.empty() && std::none_of(value.cbegin(), value.cend(), [](auto const& state)
                                                  {
                                                      return state == true;
                                                  }))
                {
                    auto copy = value;
                    copy[0_z] = true;
                    Model::Accessor<Accessor, AttrContainer, AcsrContainer>::setAttr<type, value_v>(copy, notification);
                }
                else
                {
                    Model::Accessor<Accessor, AttrContainer, AcsrContainer>::setAttr<type, value_v>(value, notification);
                }
            }
            else
            {
                Model::Accessor<Accessor, AttrContainer, AcsrContainer>::setAttr<type, value_v>(value, notification);
            }
        }

        std::unique_ptr<juce::XmlElement> parseXml(juce::XmlElement const& xml, int version) override;
    };
} // namespace Track

namespace XmlParser
{
    template <>
    void toXml<Track::ColourSet>(juce::XmlElement& xml, juce::Identifier const& attributeName, Track::ColourSet const& value);

    template <>
    auto fromXml<Track::ColourSet>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Track::ColourSet const& defaultValue)
        -> Track::ColourSet;

    template <>
    void toXml<Track::LabelLayout>(juce::XmlElement& xml, juce::Identifier const& attributeName, Track::LabelLayout const& value);

    template <>
    auto fromXml<Track::LabelLayout>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Track::LabelLayout const& defaultValue)
        -> Track::LabelLayout;
} // namespace XmlParser

ANALYSE_FILE_END
