#pragma once

#include "../Misc/AnlMisc.h"
#include "../Plugin/AnlPluginModel.h"
#include "../Zoom/AnlZoomModel.h"
#include <tinycolormap/tinycolormap.hpp>

ANALYSE_FILE_BEGIN

namespace Track
{
    using ColourMap = tinycolormap::ColormapType;

    class Results
    {
    public:
        using Marker = std::tuple<double, double, std::string>;
        using Point = std::tuple<double, double, std::optional<float>>;
        using Column = std::tuple<double, double, std::vector<float>>;

        using Markers = std::vector<Marker>;
        using Points = std::vector<Point>;
        using Columns = std::vector<Column>;

        using SharedMarkers = std::shared_ptr<const std::vector<Markers>>;
        using SharedPoints = std::shared_ptr<const std::vector<Points>>;
        using SharedColumns = std::shared_ptr<const std::vector<Columns>>;

        Results() = default;
        Results(Results const& rhs) = default;
        ~Results() = default;

        explicit Results(juce::File const& f)
        : file(f)
        {
        }

        explicit Results(SharedMarkers ptr)
        : mResults(ptr)
        {
        }

        explicit Results(SharedPoints ptr)
        : mResults(ptr)
        {
        }

        explicit Results(SharedColumns ptr)
        : mResults(ptr)
        {
        }

        inline SharedMarkers getMarkers() const noexcept
        {
            if(auto const* markersPtr = std::get_if<SharedMarkers>(&mResults))
            {
                return *markersPtr;
            }
            return nullptr;
        }

        inline SharedPoints getPoints() const noexcept
        {
            if(auto const* pointsPtr = std::get_if<SharedPoints>(&mResults))
            {
                return *pointsPtr;
            }
            return nullptr;
        }

        inline SharedColumns getColumns() const noexcept
        {
            if(auto const* columnsPtr = std::get_if<SharedColumns>(&mResults))
            {
                return *columnsPtr;
            }
            return nullptr;
        }

        inline bool isEmpty() const noexcept
        {
            if(auto const* markersPtr = std::get_if<SharedMarkers>(&mResults))
            {
                auto const markers = *markersPtr;
                return markers == nullptr || markers->empty() || std::all_of(markers->cbegin(), markers->cend(), [](auto const& channel)
                                                                             {
                                                                                 return channel.empty();
                                                                             });
            }
            else if(auto const* pointsPtr = std::get_if<SharedPoints>(&mResults))
            {
                auto const points = *pointsPtr;
                return points == nullptr || points->empty() || std::all_of(points->cbegin(), points->cend(), [](auto const& channel)
                                                                           {
                                                                               return channel.empty();
                                                                           });
            }
            else if(auto const* columnsPtr = std::get_if<SharedColumns>(&mResults))
            {
                auto const columns = *columnsPtr;
                return columns == nullptr || columns->empty() || std::all_of(columns->cbegin(), columns->cend(), [](auto const& channel)
                                                                             {
                                                                                 return channel.empty();
                                                                             });
            }
            return true;
        }

        inline bool operator==(Results const& rhd) const noexcept
        {
            return getMarkers() == rhd.getMarkers() &&
                   getPoints() == rhd.getPoints() &&
                   getColumns() == rhd.getColumns() &&
                   file == rhd.file;
        }

        inline bool operator!=(Results const& rhd) const noexcept
        {
            return !(*this == rhd);
        }

        juce::File file{};

    private:
        std::variant<SharedMarkers, SharedPoints, SharedColumns> mResults{SharedPoints(nullptr)};
    };

    void to_json(nlohmann::json& j, Results const& results);
    void from_json(nlohmann::json const& j, Results& results);

    using Images = std::vector<std::vector<juce::Image>>;

    struct ColourSet
    {
        ColourMap map = ColourMap::Inferno;
        juce::Colour background = juce::Colours::black;
        juce::Colour foreground = juce::Colours::white;
        juce::Colour text = juce::Colours::white;
        juce::Colour shadow = juce::Colours::black;
        juce::Colour grid = juce::Colours::black.withAlpha(0.7f);

        inline bool operator==(ColourSet const& rhd) const noexcept
        {
            return map == rhd.map &&
                   background == rhd.background &&
                   foreground == rhd.foreground &&
                   text == rhd.text &&
                   shadow == rhd.shadow &&
                   grid == rhd.grid;
        }

        inline bool operator!=(ColourSet const& rhd) const noexcept
        {
            return !(*this == rhd);
        }
    };

    void to_json(nlohmann::json& j, ColourSet const& colourSet);
    void from_json(nlohmann::json const& j, ColourSet& colourSet);

    // clang-format off
    enum class WarningType
    {
          none
        , plugin
        , state
        , file
    };

    enum class AttrType : size_t
    {
          identifier
        , name
        , results
        , key
        , description
        , state
        
        , height
        , colours
        , channelsLayout
        , zoomLink
        , zoomAcsr
        
        , graphics
        , warnings
        , processing
        , focused
    };

    enum class AcsrType : size_t
    {
          valueZoom
        , binZoom
    };

    enum class SignalType
    {
          showProperties
    };

    using AttrContainer = Model::Container
    < Model::Attr<AttrType::identifier, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::name, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::results, Results, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::key, Plugin::Key, Model::Flag::basic>
    , Model::Attr<AttrType::description, Plugin::Description, Model::Flag::notifying>
    , Model::Attr<AttrType::state, Plugin::State, Model::Flag::basic>
    
    , Model::Attr<AttrType::height, int, Model::Flag::basic>
    , Model::Attr<AttrType::colours, ColourSet, Model::Flag::basic>
    , Model::Attr<AttrType::channelsLayout, std::vector<bool>, Model::Flag::basic>
    , Model::Attr<AttrType::zoomLink, bool, Model::Flag::basic>
    , Model::Attr<AttrType::zoomAcsr, std::optional<std::reference_wrapper<Zoom::Accessor>>, Model::Flag::notifying>
    
    , Model::Attr<AttrType::graphics, Images, Model::Flag::notifying>
    , Model::Attr<AttrType::warnings, WarningType, Model::Flag::notifying>
    , Model::Attr<AttrType::processing, std::tuple<bool, float, bool, float>, Model::Flag::notifying>
    , Model::Attr<AttrType::focused, bool, Model::Flag::notifying>
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
                                 
                                 , {120}
                                 , {}
                                 , {std::vector<bool>{}}
                                 , {true}
                                 , {}
                                 
                                 , {}
                                 , {WarningType::none}
                                 , {{false, 0.0, false, 0.0}}
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
    void toXml<Track::Results>(juce::XmlElement& xml, juce::Identifier const& attributeName, Track::Results const& value);

    template <>
    auto fromXml<Track::Results>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Track::Results const& defaultValue)
        -> Track::Results;
} // namespace XmlParser

ANALYSE_FILE_END
