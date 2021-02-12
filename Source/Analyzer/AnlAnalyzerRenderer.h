#pragma once

#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Renderer
    : private juce::AsyncUpdater
    {
    public:

        static juce::Image createImage(Accessor const& accessor, std::function<bool(void)> predicate = nullptr);
        
        Renderer(Accessor& accessor);
        ~Renderer() override;
        
        std::function<void(void)> onUpdated = nullptr;
        
        void paintFrame(juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
        void paintRange(juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
        
    private:
        
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
        Accessor::Listener mListener;
        
        std::atomic<ProcessState> mProcessState {ProcessState::available};
        std::future<juce::Image> mProcess;
        
        juce::Image mImage;
    };
}

ANALYSE_FILE_END
