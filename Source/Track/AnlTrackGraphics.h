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

        bool runRendering(Accessor const& accessor, std::unique_ptr<Ive::PluginWrapper> plugin);
        void stopRendering();
        bool isRunning() const;
        float getAdvancement() const;

        std::function<void(Images images)> onRenderingUpdated = nullptr;
        std::function<void(Images images)> onRenderingEnded = nullptr;
        std::function<void(void)> onRenderingAborted = nullptr;

    private:
        void abortRendering();
        void performRendering(std::vector<Track::Result::Data::Columns> const columns, ColourMap const colourMap, juce::Range<double> const valueRange, bool logScale, double sampleRate, int width, int height, Ive::PluginWrapper* plugin, int featureIndex, bool hasDuration);

        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        // clang-format off
        enum class ProcessState
        {
              available
            , aborted
            , running
            , ended
        };
        // clang-format on

        std::mutex mMutex;
        Images mImages;
        Result::Data mData;
        std::unique_ptr<Result::Access> mAccess;
        std::atomic<ProcessState> mRenderingState{ProcessState::available};
        std::thread mRenderingProcess;
        std::mutex mRenderingMutex;
        std::atomic<float> mAdvancement{0.0f};
        Chrono mChrono{"Track", "graphics rendering ended"};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Graphics)
    };
} // namespace Track

ANALYSE_FILE_END
