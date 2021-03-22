#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Graphics
    : private juce::AsyncUpdater
    {
    public:
        Graphics() = default;
        ~Graphics() override;
        
        void runRendering(Accessor const& accessor);
        void stopRendering();
        
        std::function<void(std::vector<juce::Image> const& images)> onRenderingEnded = nullptr;
        std::function<void(void)> onRenderingAborted = nullptr;
        
        static juce::Image createImage(std::vector<Plugin::Result> const& results, ColourMap const colourMap, Zoom::Range const valueRange, std::function<bool(void)> predicate = nullptr);
    private:
        
        void abortRendering();
        
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;
       
        enum class ProcessState
        {
              available
            , aborted
            , running
            , ended
        };
        
        std::atomic<ProcessState> mRenderingState {ProcessState::available};
        std::future<std::vector<juce::Image>> mRenderingProcess;
        std::mutex mRenderingMutex;
        Chrono mChrono {"Track", "graphics rerndering ended"};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Graphics)
    };
}

ANALYSE_FILE_END
