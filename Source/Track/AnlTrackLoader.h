#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Loader
    : private juce::AsyncUpdater
    {
    public:
        Loader() = default;
        ~Loader() override;

        juce::Result loadAnalysis(Accessor const& accessor, juce::File const& file);

        std::function<void(Results const& results)> onLoadingEnded = nullptr;
        std::function<void(void)> onLoadingAborted = nullptr;

        bool isRunning() const;
        float getAdvancement() const;

    private:
        Results loadFromJson(juce::File const& file, std::ifstream stream);
        Results loadFromBinary(juce::File const& file, std::ifstream stream);
        void abortLoading();

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

        std::atomic<ProcessState> mLoadingState{ProcessState::available};
        std::future<Results> mLoadingProcess;
        std::mutex mLoadingMutex;
        std::atomic<float> mAdvancement{0.0f};
        Chrono mChrono{"Track", "loading file ended"};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Loader)
    };
} // namespace Track

ANALYSE_FILE_END