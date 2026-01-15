#pragma once

#include "AnlApplicationNeuralyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        class SettingsContent
        : public juce::Component
        , private juce::AsyncUpdater
        {
        public:
            SettingsContent(Accessor& accessor);
            ~SettingsContent() override;

            // juce::Component
            void resized() override;
            void parentHierarchyChanged() override;
            void visibilityChanged() override;

        private:
            // juce::AsyncUpdater
            void handleAsyncUpdate() override;

            Accessor& mAccessor;
            Accessor::Listener mListener{typeid(*this).name()};
            PropertyList mModel;
            PropertyList mContextSize;
            PropertyList mBatchSize;
            TimerClock mTimerClock;
            std::atomic<bool> mShouldQuitDownload{false};
            std::future<juce::Result> mDownloadProcess;
            juce::File mFutureModel;
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
