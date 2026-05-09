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
            void broughtToFront() override;
            void handleCommandMessage(int commandId) override;

        private:
            void showModelMenu();
            void checkForUpdatedModels();

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
            static auto constexpr gModelListUpdatedMessageId = 1;

            Accessor& mAccessor;
            Accessor::Listener mListener{typeid(*this).name()};
            PropertyList mBackend;
            PropertyTextButton mModelsDirectory;
            PropertyText mRemoteUrl;
            PropertyNumber mRemotePort;
            ColouredPanel mBackendSeparator;
            PropertyList mModel;
            PropertyNumber mContextSize;
            PropertyNumber mBatchSize;
            PropertyNumber mMinP;
            PropertyNumber mTemperature;
            PropertyNumber mTopP;
            PropertyNumber mTopK;
            PropertyNumber mPresencePenalty;
            PropertyNumber mRepetitionPenalty;
            ColouredPanel mDownloadSeparator;
            std::vector<std::unique_ptr<DownloadProcess>> mDownloadProcesses;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsContent)
        };

        class SettingsPanel
        : public HideablePanel
        {
        public:
            SettingsPanel(Accessor& accessor);
            ~SettingsPanel() override;

            // juce::Component
            void broughtToFront() override;

        private:
            SettingsContent mContent;
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
