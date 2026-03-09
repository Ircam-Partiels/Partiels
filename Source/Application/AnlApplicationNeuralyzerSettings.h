#pragma once

#include "AnlApplicationNeuralyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        class SettingsContent
        : public juce::Component
        {
        public:
            SettingsContent(Accessor& accessor);
            ~SettingsContent() override;

            // juce::Component
            void resized() override;
            void parentHierarchyChanged() override;
            void visibilityChanged() override;
            void handleCommandMessage(int commandId) override;

        private:
            class DownloadProcess
            : public juce::Component
            , private juce::AsyncUpdater
            , private juce::Timer
            {
            public:
                DownloadProcess(SettingsContent& owner, nlohmann::json description);
                ~DownloadProcess() override;

                bool isRunning() const;
                juce::File getModelFile() const;

                // juce::Component
                void resized() override;

            private:
                // juce::AsyncUpdater
                void handleAsyncUpdate() override;

                // juce::Timer
                void timerCallback() override;

                SettingsContent& mOwner;
                juce::File mModelFile;
                std::atomic<bool> mShouldContinue{true};
                std::atomic<float> mProcessProgress{0.0f};
                double mEffectiveProgress{0.0};
                std::future<juce::Result> mAsyncProcess;
                ColouredPanel mSeparator;
                juce::Label mNameLabel;
                juce::ProgressBar mProgressBar{mEffectiveProgress};
                Icon mCancelButton;
            };

            static auto constexpr gDownloadProcessMessageId = 0;

            Accessor& mAccessor;
            Accessor::Listener mListener{typeid(*this).name()};
            PropertyList mModel;
            PropertyList mContextSize;
            PropertyList mBatchSize;
            PropertyNumber mMinP;
            PropertyNumber mTemperature;
            ColouredPanel mSeparator;
            PropertyTextButton mModelsDirectory;
            TimerClock mTimerClock;
            std::vector<std::unique_ptr<DownloadProcess>> mDownloadProcesses;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsContent)
        };

        class SettingsPanel
        : public HideablePanel
        {
        public:
            SettingsPanel(Accessor& accessor);
            ~SettingsPanel() override;

        private:
            SettingsContent mContent;
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
