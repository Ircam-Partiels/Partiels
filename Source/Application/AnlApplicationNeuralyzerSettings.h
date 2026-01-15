#pragma once

#include "AnlApplicationNeuralyzerDownloader.h"
#include "AnlApplicationNeuralyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        class SettingsContent
        : public juce::Component
        , private juce::ChangeListener
        {
        public:
            SettingsContent(Accessor& accessor);
            ~SettingsContent() override;

            // juce::Component
            void resized() override;
            void broughtToFront() override;
            void handleCommandMessage(int commandId) override;

        private:
            // juce::ChangeListener
            void changeListenerCallback(juce::ChangeBroadcaster* source) override;

            void showModelMenu();
            void showDownloadAlert(juce::File const& file, juce::Result const& result);

            Accessor& mAccessor;
            Accessor::Listener mListener{typeid(*this).name()};
            PropertyList mBackend;
            ColouredPanel mBackendSeparator;
            PropertyText mRemoteUrl;
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
            PropertyTextButton mModelsDirectory;
            PropertyTextButton mRagEngineDownload;
            std::vector<std::unique_ptr<Neuralyzer::Downloader::View>> mDownloadViews;
            ColouredPanel mMcpSeparator;
            PropertyToggle mMcpForClaudeApp;
            PropertyToggle mMcpForCopilotApp;

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
