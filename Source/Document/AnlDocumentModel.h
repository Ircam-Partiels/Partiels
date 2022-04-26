#pragma once

#include "../Group/AnlGroupModel.h"
#include "../Misc/AnlMisc.h"
#include "../Track/AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    using GridMode = Track::GridMode;

    // clang-format off
    enum class EditMode
    {
          items
        , frames
    };
    
    enum class AttrType : size_t
    {
          reader
        , layout
        , viewport
        , path
        , grid
        , autoresize
        , samplerate
        , editMode
    };
    
    enum class AcsrType : size_t
    {
          timeZoom
        , transport
        , groups
        , tracks
    };
    
    enum class SignalType
    {
          viewport
        , showReaderPanel
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::reader, std::vector<AudioFileLayout>, Model::Flag::basic>
    , Model::Attr<AttrType::layout, std::vector<juce::String>, Model::Flag::basic>
    , Model::Attr<AttrType::viewport, juce::Point<int>, Model::Flag::saveable>
    , Model::Attr<AttrType::path, juce::File, Model::Flag::saveable | Model::Flag::notifying>
    , Model::Attr<AttrType::grid, GridMode, Model::Flag::saveable | Model::Flag::notifying>
    , Model::Attr<AttrType::autoresize, bool, Model::Flag::saveable | Model::Flag::notifying>
    , Model::Attr<AttrType::samplerate, double, Model::Flag::saveable | Model::Flag::notifying>
    , Model::Attr<AttrType::editMode, EditMode, Model::Flag::notifying>
    >;
    
    using AcsrContainer = Model::Container
    < Model::Acsr<AcsrType::timeZoom, Zoom::Accessor, Model::Flag::saveable | Model::Flag::notifying, 1>
    , Model::Acsr<AcsrType::transport, Transport::Accessor, Model::Flag::saveable | Model::Flag::notifying, 1>
    , Model::Acsr<AcsrType::groups, Group::Accessor, Model::Flag::basic, Model::resizable>
    , Model::Acsr<AcsrType::tracks, Track::Accessor, Model::Flag::basic, Model::resizable>
    >;
    // clang-format on

    class Accessor
    : public Model::Accessor<Accessor, AttrContainer, AcsrContainer>
    , public Broadcaster<Accessor, SignalType>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer, AcsrContainer>::Accessor;

        Accessor()
        : Accessor(AttrContainer({}, {}, {}, {}, {GridMode::partial}, {false}, {0.0}, {EditMode::items}))
        {
        }

        template <acsr_enum_type type>
        size_t getAcsrPosition(typename std::tuple_element<static_cast<size_t>(type), acsr_container_type>::type::accessor_type const& other) const
        {
            if constexpr(type == AcsrType::groups)
            {
                auto const identifier = other.template getAttr<Group::AttrType::identifier>();
                auto const groupAcsrs = getAcsrs<AcsrType::groups>();
                auto const it = std::find_if(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](auto const& groupAcsr)
                                             {
                                                 return groupAcsr.get().template getAttr<Group::AttrType::identifier>() == identifier;
                                             });
                if(it == groupAcsrs.cend())
                {
                    return npos;
                }
                return static_cast<size_t>(std::distance(groupAcsrs.cbegin(), it));
            }
            else if constexpr(type == AcsrType::tracks)
            {
                auto const identifier = other.template getAttr<Track::AttrType::identifier>();
                auto const trackAcsrs = getAcsrs<AcsrType::tracks>();
                auto const it = std::find_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                                             {
                                                 return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
                                             });
                if(it == trackAcsrs.cend())
                {
                    return npos;
                }
                return static_cast<size_t>(std::distance(trackAcsrs.cbegin(), it));
            }
        }

        std::unique_ptr<juce::XmlElement> parseXml(juce::XmlElement const& xml, int version) override
        {
            auto copy = std::make_unique<juce::XmlElement>(xml);
            if(copy != nullptr && version <= 0x7)
            {
                if(copy->hasAttribute("file"))
                {
                    auto const file = XmlParser::fromXml(*copy.get(), "file", juce::File{});
                    XmlParser::toXml(*copy.get(), "reader", std::vector<AudioFileLayout>{{file}});
                }
            }
            return copy;
        }
    };
} // namespace Document

ANALYSE_FILE_END
