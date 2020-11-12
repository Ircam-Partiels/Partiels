#pragma once

#include "../Tools/AnlModel.h"
#include "../Analyzer/AnlAnalyzerModel.h"
#include "../Zoom/AnlZoomStateModel.h"
#include "../Tools/AnlBroadcaster.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    using AttrFlag = Model::AttrFlag;
    
    enum AttrType : size_t
    {
        file,
        isLooping,
        gain,
        isPlaybackStarted,
        playheadPosition,
        //analyzers,
        timeZoom
    };
    
    enum class Signal
    {
        movePlayhead,
        audioInstanceChanged
    };
    
    using Container = Model::Container
    < Model::Attr<AttrType::file, juce::File, AttrFlag::basic>
    , Model::Attr<AttrType::isLooping, bool, AttrFlag::basic>
    , Model::Attr<AttrType::gain, double, AttrFlag::basic>
    , Model::Attr<AttrType::isPlaybackStarted, bool, AttrFlag::notifying>
    , Model::Attr<AttrType::playheadPosition, double, AttrFlag::notifying>
    //, Model::Attr<AttrType::analyzers, std::vector<std::unique_ptr<Analyzer::Accessor>>, AttrFlag::basic>
    , Model::Attr<AttrType::timeZoom, Zoom::State::Container, AttrFlag::model | AttrFlag::saveable>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, Container>
    , public Broadcaster<Accessor, Signal>
    , public Tools::AtomicManager<Vamp::Plugin>
    {
    public:
        using Model::Accessor<Accessor, Container>::Accessor;
        
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
        
    private:
        Zoom::State::Accessor zoomState {getValueRef<AttrType::timeZoom>()};
    };
}

ANALYSE_FILE_END
