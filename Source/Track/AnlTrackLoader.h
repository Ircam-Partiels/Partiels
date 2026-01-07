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

        void loadAnalysis(FileDescription const& fd);

        std::function<void(Results const& results)> onLoadingSucceeded = nullptr;
        std::function<void(juce::String const& message)> onLoadingFailed = nullptr;
        std::function<void(void)> onLoadingAborted = nullptr;

        bool isRunning() const;
        float getAdvancement() const;

        static std::variant<Results, juce::String> loadFromFile(FileDescription const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromJson(FileDescription const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromBinary(FileDescription const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromCsv(FileDescription const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromReaper(FileDescription const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromCue(FileDescription const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromSdif(FileDescription const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static juce::String getWildCardForAllFormats();

        static std::tuple<juce::Result, FileDescription> getFileDescription(juce::File const& file, double sampleRate);

    private:
        static std::variant<Results, juce::String> loadFromJson(std::istream& stream, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromBinary(std::istream& stream, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromCsv(std::istream& stream, char const separator, bool useEndTime, char const lineBreakSeparator, bool prependLineIndex, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromReaper(std::istream& stream, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromCue(std::istream& stream, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromSdif(juce::File const& file, uint32_t frameId, uint32_t matrixId, std::optional<size_t> row, std::optional<size_t> column, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);

        void abortLoading();

        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        std::atomic<bool> mShouldAbort{false};
        std::atomic<float> mAdvancement{0.0f};
        std::mutex mLoadingMutex;
        std::future<std::variant<Results, juce::String>> mLoadingProcess;
        Chrono mChrono{"Track"};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Loader)

    public:
        class UnitTest;
    };
} // namespace Track

ANALYSE_FILE_END
