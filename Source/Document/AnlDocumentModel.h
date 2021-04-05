#pragma once

#include "../Misc/AnlMisc.h"
#include "../Track/AnlTrackModel.h"
#include "../Zoom/AnlZoomModel.h"
#include "../Misc/AnlBroadcaster.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    struct Group
    {
        juce::String identifier;                    //!< The identifier of the group
        juce::String name;                          //!< The name of the group
        int height = 144;                           //!< The height of the group
        juce::Colour colour = juce::Colours::black; //!< The colour of the group
        bool open = true;                           //!< The open/closed state of the group
        std::vector<juce::String> layout;           //!< The layout of the group
        
        inline bool operator==(Group const& rhd) const noexcept
        {
            return identifier == rhd.identifier && name == rhd.name && height == rhd.height && colour == rhd.colour && open == rhd.open && layout == rhd.layout;
        }
        
        inline bool operator!=(Group const& rhd) const noexcept
        {
            return !(*this == rhd);
        }
    };
    
    enum class AttrType : size_t
    {
          file
        , isLooping
        , gain
        , isPlaybackStarted
        , startPlayheadPosition
        , runningPlayheadPosition
        , layoutHorizontal
        , layoutVertical
        , layout
        , expanded
    };
    
    enum class AcsrType : size_t
    {
          timeZoom
        , tracks
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::file, juce::File, Model::Flag::basic>
    , Model::Attr<AttrType::isLooping, bool, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::gain, double, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::isPlaybackStarted, bool, Model::Flag::notifying>
    , Model::Attr<AttrType::startPlayheadPosition, double, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::runningPlayheadPosition, double, Model::Flag::notifying>
    , Model::Attr<AttrType::layoutHorizontal, int, Model::Flag::basic>
    , Model::Attr<AttrType::layoutVertical, int, Model::Flag::basic>
    , Model::Attr<AttrType::layout, std::vector<juce::String>, Model::Flag::basic>
    , Model::Attr<AttrType::expanded, bool, Model::Flag::basic>
    >;
    
    using AcsrContainer = Model::Container
    < Model::Acsr<AcsrType::timeZoom, Zoom::Accessor, Model::Flag::basic, 1>
    , Model::Acsr<AcsrType::tracks, Track::Accessor, Model::Flag::basic, Model::resizable>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, AttrContainer, AcsrContainer>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer, AcsrContainer>::Accessor;
        
        Accessor()
        : Accessor(AttrContainer(  {juce::File{}}
                                 , {false}
                                 , {1.0}
                                 , {false}
                                 , {0.0}
                                 , {0.0}
                                 , {144}
                                 , {144}
                                 , {}
                                 , {true}))
        {
        }
    };
}

namespace XmlParser
{
    template<>
    void toXml<Document::Group>(juce::XmlElement& xml, juce::Identifier const& attributeName, Document::Group const& value);
    
    template<>
    auto fromXml<Document::Group>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Document::Group const& defaultValue)
    -> Document::Group;
}

ANALYSE_FILE_END
