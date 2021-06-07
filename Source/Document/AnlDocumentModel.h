#pragma once

#include "../Group/AnlGroupModel.h"
#include "../Misc/AnlMisc.h"
#include "../Track/AnlTrackModel.h"
#include "../Transport/AnlTransportModel.h"
#include "../Zoom/AnlZoomModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    // clang-format off
    enum class AttrType : size_t
    {
          files
        , layout
        , viewport
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
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::files, std::vector<juce::File>, Model::Flag::basic>
    , Model::Attr<AttrType::layout, std::vector<juce::String>, Model::Flag::basic>
    , Model::Attr<AttrType::viewport, juce::Point<int>, Model::Flag::saveable>
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
        : Accessor(AttrContainer({}, {}, {}))
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
            if(copy != nullptr && version < ProjectInfo::versionNumber)
            {
                auto const file = XmlParser::fromXml(*copy.get(), "file", juce::File{});
                XmlParser::toXml(*copy.get(), "files", std::vector<juce::File>{file});
            }
            return copy;
        }
    };
} // namespace Document

ANALYSE_FILE_END
