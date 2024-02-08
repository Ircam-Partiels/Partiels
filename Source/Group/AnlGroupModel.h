#pragma once

#include "../Misc/AnlMisc.h"
#include "../Track/AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    using FocusInfo = Track::FocusInfo;

    // clang-format off
    enum class ChannelVisibilityState
    {
          hidden
        , visible
        , both
    };
    // clang-format on

    using TrackList = std::vector<std::reference_wrapper<Track::Accessor>>;

    // clang-format off
    enum class AttrType : size_t
    {
          identifier
        , name
        , height
        , colour
        , expanded
        , layout
        , tracks
        , focused
        , referenceid
    };

    enum class AcsrType : size_t
    {
          zoom
    };

    enum class SignalType
    {
          showProperties
    };

    using AttrContainer = Model::Container
    < Model::Attr<AttrType::identifier, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::name, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::height, int, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::colour, juce::Colour, Model::Flag::basic>
    , Model::Attr<AttrType::expanded, bool, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::layout, std::vector<juce::String>, Model::Flag::basic>
    , Model::Attr<AttrType::tracks, TrackList, Model::Flag::notifying>
    , Model::Attr<AttrType::focused, FocusInfo, Model::Flag::notifying>
    , Model::Attr<AttrType::referenceid, juce::String, Model::Flag::basic>
    >;

    using AcsrContainer = Model::Container
    < Model::Acsr<AcsrType::zoom, Zoom::Accessor, Model::Flag::saveable | Model::Flag::notifying, 1>
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
        : Accessor(AttrContainer(  {}
                                 , {}
                                 , {144}
                                 , {juce::Colours::transparentBlack}
                                 , {true}
                                 , {}
                                 , {}
                                 , {}
                                 , {""}))
        // clang-format on
        {
        }

        template <acsr_enum_type type>
        bool insertAcsr(size_t index, NotificationType const notification)
        {
            if constexpr(type == AcsrType::zoom)
            {
                return Model::Accessor<Accessor, AttrContainer, AcsrContainer>::insertAcsr<AcsrType::zoom>(index, std::make_unique<Zoom::Accessor>(Zoom::Range{0.0, 1.0}), notification);
            }
        }

        std::unique_ptr<juce::XmlElement> parseXml(juce::XmlElement const& xml, int version) override;
    };
} // namespace Group

ANALYSE_FILE_END
