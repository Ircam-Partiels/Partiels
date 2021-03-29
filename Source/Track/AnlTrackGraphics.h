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
        bool isRunning() const;
        float getAdvancement() const;
        
        std::function<void(std::vector<juce::Image> images)> onRenderingUpdated = nullptr;
        std::function<void(std::vector<juce::Image> images)> onRenderingEnded = nullptr;
        std::function<void(void)> onRenderingAborted = nullptr;
        
        static float valueToPixel(float value, juce::Range<double> const& valueRange, juce::Rectangle<float> const& bounds);
        static float secondsToPixel(double seconds, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds);
        static double pixelToSeconds(float position, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds);
        static double realTimeToSeconds(Vamp::RealTime const& rt);
        static Vamp::RealTime secondsToRealTime(double seconds);
        static Vamp::RealTime getEndRealTime(Plugin::Result const& rt);

        static juce::String getMarkerText(std::vector<Plugin::Result> const& results, double time);
        static juce::String getSegmentText(std::vector<Plugin::Result> const& results, double time);
        static juce::String getGridText(std::vector<Plugin::Result> const& results, double time);
        
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
        
        std::mutex mMutex;
        std::vector<juce::Image> mImages;
        std::atomic<ProcessState> mRenderingState {ProcessState::available};
        std::thread mRenderingProcess;
        std::mutex mRenderingMutex;
        std::atomic<float> mAdvancement {0.0f};
        Chrono mChrono {"Track", "graphics rendering ended"};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Graphics)
    };
}

ANALYSE_FILE_END
