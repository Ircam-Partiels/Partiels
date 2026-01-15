#pragma once

#include "../Document/AnlDocumentTools.h"
#include "AnlApplicationNeuralyzerChatHistory.h"
#include "AnlApplicationNeuralyzerChatQuery.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        class Chat
        : public juce::Component
        , private juce::Timer
        , private juce::ChangeListener
        {
        public:
            Chat(BackgroundAgent& agent);
            ~Chat() override;

            // juce::Component
            void paint(juce::Graphics& g) override;
            void paintOverChildren(juce::Graphics& g) override;
            void resized() override;
            void colourChanged() override;
            void parentHierarchyChanged() override;
            void mouseDown(juce::MouseEvent const& event) override;

        private:
            void updated();

            // juce::Timer
            void timerCallback() override;

            // juce::ChangeListener
            void changeListenerCallback(juce::ChangeBroadcaster* source) override;

            void sendUserQuery();
            void stopUserQuery();
            void clearHistory();
            void updateHistory();
            bool isConfigured();

            BackgroundAgent& mAgent;
            Agent::History mHistory;
            std::vector<juce::String> mTables;
            ChatHistory mHistoryEditor;
            ColouredPanel mSeparator1;
            juce::TextEditor mTempResponse;
            ColouredPanel mSeparator2;
            ChatQuery mQueryEditor;
            Icon mSendButton;
            Icon mAttachedFilesButton;
            ColouredPanel mSeparator3;
            juce::Label mStatusLabel;
            juce::Label mUsageLabel;
            juce::StringArray mDroppedFilePaths;
            bool mIsDragging{false};
            std::unique_ptr<juce::FileChooser> mFileChooser;
            Document::Accessor mLastDocumentAccessorState;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Chat)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
