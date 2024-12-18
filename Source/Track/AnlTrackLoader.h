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

        void loadAnalysis(FileInfo const& fileInfo);

        std::function<void(Results const& results)> onLoadingSucceeded = nullptr;
        std::function<void(juce::String const& message)> onLoadingFailed = nullptr;
        std::function<void(void)> onLoadingAborted = nullptr;

        bool isRunning() const;
        float getAdvancement() const;

        static std::variant<Results, juce::String> loadFromFile(FileInfo const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromJson(FileInfo const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromBinary(FileInfo const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromCsv(FileInfo const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromCue(FileInfo const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromSdif(FileInfo const& fileInfo, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static juce::String getWildCardForAllFormats();

        class ArgumentSelector
        : public juce::Component
        {
        public:
            ArgumentSelector();
            ~ArgumentSelector() override = default;

            // juce::Component
            void resized() override;

            bool setFile(juce::File const& file, double sampleRate, std::function<void(FileInfo)> callback);

        private:
            PropertyText mPropertyName;
            PropertyList mPropertyColumnSeparator;
            SdifConverter::Panel mSdifPanel;
            PropertyTextButton mLoadButton;
        };

    private:
        static std::variant<Results, juce::String> loadFromJson(std::istream& stream, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromBinary(std::istream& stream, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
        static std::variant<Results, juce::String> loadFromCsv(std::istream& stream, char const separator, bool useEndTime, std::atomic<bool> const& shouldAbort, std::atomic<float>& advancement);
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
        Chrono mChrono{"Track", "loading file ended"};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Loader)

    public:
        class UnitTest;
    };
} // namespace Track

ANALYSE_FILE_END
