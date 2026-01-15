#pragma once

#include "../Document/AnlDocumentTools.h"
#include "AnlApplicationNeuralyzerChatElicitation.h"
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
            void sendElicitationAnswer();
            void stopUserQuery();
            void clearHistory();
            void updateHistory();
            void updateAttachedFilesButton();
            void updateSendButton();
            void updateElicitation();
            bool isConfigured();

            BackgroundAgent& mAgent;
            ChatHistory mHistoryEditor;
            ColouredPanel mSeparator1;
            juce::TextEditor mTempResponse;
            ColouredPanel mSeparator2;
            ChatElicitation mElicitationPanel;
            ChatQuery mQueryEditor;
            Icon mSendButton;
            Icon mAttachedFilesButton;
            ColouredPanel mSeparator3;
            juce::Label mStatusLabel;
            juce::Label mUsageLabel;
            juce::StringArray mDroppedFilePaths;
            std::optional<BackgroundAgent::Elicitation> mElicitation;
            bool mIsDragging{false};
            std::unique_ptr<juce::FileChooser> mFileChooser;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Chat)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
