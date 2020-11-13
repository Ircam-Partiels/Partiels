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
    , Model::AttrNoCopy<AttrType::analyzers, std::vector<std::unique_ptr<Analyzer::Container>>, Model::AttrFlag::model | Model::AttrFlag::basic>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, Container>
    , public Broadcaster<Accessor, Signal>
    , public Tools::AtomicManager<Vamp::Plugin>
    {
    public:
        using Model::Accessor<Accessor, Container>::Accessor;
        
        template <AttrType type>
        auto& getAccessor(size_t) noexcept;
        
        template <AttrType type>
        auto const& getAccessor(size_t) const noexcept;

        template <>
        auto& getAccessor<AttrType::timeZoom>(size_t) noexcept
        {
            return mZoomState;
        }
        
        template <>
        auto const& getAccessor<AttrType::timeZoom>(size_t) const noexcept
        {
            return mZoomState;
        }
        
        template <>
        auto& getAccessor<AttrType::analyzers>(size_t index) noexcept
        {
            while(mAnalyzers.size() <= index)
            {
                mAnalyzers.push_back(std::make_unique<Analyzer::Accessor>(*getValueRef<AttrType::analyzers>()[mAnalyzers.size()].get()));
            }
            return *mAnalyzers[index].get();
        }
        
        template <>
        auto const& getAccessor<AttrType::analyzers>(size_t index) const noexcept
        {
            return *mAnalyzers[index].get();
        }
        
    private:
        Zoom::Accessor mZoomState {getValueRef<AttrType::timeZoom>()};
        std::vector<std::unique_ptr<Analyzer::Accessor>> mAnalyzers;
    };
}

ANALYSE_FILE_END
