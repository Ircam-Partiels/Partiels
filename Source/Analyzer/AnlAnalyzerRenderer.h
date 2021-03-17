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
        static void renderImage(juce::Graphics& g, juce::Rectangle<int> const& bounds, juce::Image const& image, Zoom::Accessor const& xZoomAcsr, Zoom::Accessor const& yZoomAcsr);
        
        Renderer(Accessor& accessor, Type type);
        ~Renderer() override;
        
        std::function<void(void)> onUpdated = nullptr;
        
        void prepareRendering();
        bool isPreparing() const;
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
        
        std::atomic<ProcessState> mProcessState {ProcessState::available};
        std::future<juce::Image> mProcess;
        
        std::vector<juce::Image> mImages;
        juce::Time mRenderingStartTime;
    };
}

ANALYSE_FILE_END
