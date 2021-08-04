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
        // clang-format off
        enum class ProcessState
        {
              available
            , aborted
            , running
            , ended
        };
        // clang-format on

        static Results loadFromJson(std::istream& stream, std::atomic<ProcessState>& loadingState, std::atomic<float>& advancement);
        static Results loadFromBinary(std::istream& stream, std::atomic<ProcessState>& loadingState, std::atomic<float>& advancement);
        static Results loadFromCsv(std::istream& stream, std::atomic<ProcessState>& loadingState, std::atomic<float>& advancement);
        void abortLoading();

        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        std::atomic<ProcessState> mLoadingState{ProcessState::available};
        std::future<Results> mLoadingProcess;
        std::mutex mLoadingMutex;
        std::atomic<float> mAdvancement{0.0f};
        Chrono mChrono{"Track", "loading file ended"};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Loader)

    public:
        class UnitTest;
    };
} // namespace Track

ANALYSE_FILE_END
