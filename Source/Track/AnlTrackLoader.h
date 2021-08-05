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

        void loadAnalysis(Accessor const& accessor, juce::File const& file);

        std::function<void(Results const& results)> onLoadingSucceeded = nullptr;
        std::function<void(juce::String const& message)> onLoadingFailed = nullptr;
        std::function<void(void)> onLoadingAborted = nullptr;

        bool isRunning() const;
        float getAdvancement() const;

    private:
        static std::variant<Results, juce::String> loadFromFile(juce::File const& file, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromJson(std::istream& stream, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromBinary(std::istream& stream, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromCsv(std::istream& stream, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        void abortLoading();

        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        std::atomic<bool> mShouldAbort{false};
        std::atomic<float> mAdvancement{0.0f};
        std::mutex mLoadingMutex;
        std::future<std::variant<Results, juce::String>> mLoadingProcess;
        Chrono mChrono{"Track", "loading file ended"};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Loader)

    public:
        class UnitTest;
    };
} // namespace Track

ANALYSE_FILE_END
