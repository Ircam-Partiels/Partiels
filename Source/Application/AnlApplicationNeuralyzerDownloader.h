#pragma once

#include "AnlApplicationNeuralyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        namespace Downloader
        {
            class Process
            : public juce::ChangeBroadcaster
            , private juce::AsyncUpdater
            {
            public:
                Process(juce::File destinationFile, juce::URL sourceUrl, std::function<void(Process const&)> callback);
                ~Process() override;

                juce::File getOutputFile() const;
                double getProgress() const;
                bool isRunning() const;
                juce::Result getResult() const;

                void cancel();

            private:
                // juce::AsyncUpdater
                void handleAsyncUpdate() override;

                juce::File mOutputFile;
                std::function<void(Process const&)> mCallback;
                std::atomic<bool> mShouldContinue{true};
                std::atomic<float> mProcessProgress{0.0f};
                std::atomic<bool> mIsRunning{false};
                juce::Result mResult{juce::Result::ok()};
                std::future<juce::Result> mAsyncProcess;
            };

            class Manager
            : public juce::ChangeBroadcaster
            , private juce::ChangeListener
            {
            public:
                Manager() = default;
                ~Manager() override;

                bool start(juce::File file, juce::URL url, std::function<void(Process const&)> callback);
                std::vector<std::reference_wrapper<Process>> getProcesses();

                bool isDownloading(juce::File const& output) const;

            private:
                // juce::ChangeListener
                void changeListenerCallback(juce::ChangeBroadcaster* source) override;

                std::vector<std::unique_ptr<Process>> mProcesses;
            };

            class View
            : public juce::Component
            , private juce::Timer
            {
            public:
                explicit View(Process& process);
                ~View() override;

                Process& getProcess() const;

                // juce::Component
                void resized() override;

            private:
                // juce::Timer
                void timerCallback() override;

                Process& mProcess;
                double mEffectiveProgress{0.0};
                ColouredPanel mSeparator;
                juce::Label mNameLabel;
                juce::ProgressBar mProgressBar{mEffectiveProgress};
                Icon mCancelButton;
            };
        } // namespace Downloader
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
