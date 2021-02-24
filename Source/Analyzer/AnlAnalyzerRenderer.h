#pragma once

#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Renderer
    : private juce::AsyncUpdater
    {
    public:
        
        enum class Type
        {
              frame
            , range
        };

        static juce::Image createImage(Accessor const& accessor, std::function<bool(void)> predicate = nullptr);
        
        Renderer(Accessor& accessor, Type type);
        ~Renderer() override;
        
        juce::String getIdentifier() const;
        
        std::function<void(void)> onUpdated = nullptr;
        
        void paint(juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
        
    private:
        
        enum class DisplayMode
        {
              unsupported
            , surface
            , bar
            , segment
            , matrix
        };
        
        void paintFrame(juce::Graphics& g, juce::Rectangle<int> const& bounds);
        void paintRange(juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
        
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;
        
        enum class ProcessState
        {
              available
            , aborted
            , running
            , ended
        };
        
        Accessor& mAccessor;
        Type const mType;
        Accessor::Listener mListener;
        
        std::atomic<ProcessState> mProcessState {ProcessState::available};
        std::future<juce::Image> mProcess;
        
        juce::Image mImage;
    };
}

ANALYSE_FILE_END
