#pragma once

#include "../Tools/AnlModel.h"
#include "../Analyzer/AnlAnalyzerModel.h"
#include "../Zoom/AnlZoomModel.h"
#include "../Tools/AnlBroadcaster.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    enum AttrType : size_t
    {
        file,
        isLooping,
        gain,
        isPlaybackStarted,
        playheadPosition,
        timeZoom,
        analyzers
    };
    
    enum class Signal
    {
        movePlayhead,
        audioInstanceChanged
    };
    
    using Container = Model::Container
    < Model::Attr<AttrType::file, juce::File, Model::AttrFlag::basic>
    , Model::Attr<AttrType::isLooping, bool, Model::AttrFlag::basic>
    , Model::Attr<AttrType::gain, double, Model::AttrFlag::basic>
    , Model::Attr<AttrType::isPlaybackStarted, bool, Model::AttrFlag::notifying>
    , Model::Attr<AttrType::playheadPosition, double, Model::AttrFlag::notifying>
    , Model::Attr<AttrType::timeZoom, Zoom::Container, Model::AttrFlag::model | Model::AttrFlag::saveable>
    , Model::AttrNoCopy<AttrType::analyzers, std::vector<std::unique_ptr<int>>, Model::AttrFlag::model | Model::AttrFlag::saveable>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, Container>
    , public Broadcaster<Accessor, Signal>
    , public Tools::AtomicManager<Vamp::Plugin>
    {
    public:
        using Model::Accessor<Accessor, Container>::Accessor;
        
        template <enum_type type, typename value_v>
        void setValue(value_v const& value, NotificationType notification)
        {
            Model::Accessor<Accessor, Container>::setValue<type, value_v>(value, notification);
        }
        
        template <>
        void setValue<AttrType::analyzers, std::vector<std::unique_ptr<int>>>(std::vector<std::unique_ptr<int>> const& value, NotificationType notification)
        {
            std::cout << "aki\n";
        }
        
        template <AttrType type>
        auto& getAccessor() noexcept;
        
        template <AttrType type>
        auto const& getAccessor() const noexcept;

        template <>
        auto& getAccessor<AttrType::timeZoom>() noexcept
        {
            return zoomState;
        }
        
        template <>
        auto const& getAccessor<AttrType::timeZoom>() const noexcept
        {
            return zoomState;
        }
        
        template <>
        auto& getAccessor<AttrType::analyzers>() noexcept
        {
            return zoomState;
        }
        
        template <>
        auto const& getAccessor<AttrType::analyzers>() const noexcept
        {
            return zoomState;
        }
        
    private:
        Zoom::Accessor zoomState {getValueRef<AttrType::timeZoom>()};
    };
}

ANALYSE_FILE_END
