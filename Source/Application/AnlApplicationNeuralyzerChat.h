#pragma once

#include "AnlApplicationNeuralyzerAgent.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        class Chat
        : public juce::Component
        , private juce::AsyncUpdater
        , private juce::Timer
        {
        public:
            Chat(Accessor& accessor, Agent& agent);
            ~Chat() override;

            // juce::Component
            void paint(juce::Graphics& g) override;
            void resized() override;
            void colourChanged() override;
            void parentHierarchyChanged() override;
            void mouseDown(juce::MouseEvent const& event) override;

        private:
            // clang-format off
            enum class MessageType
            {
                  assistant
                , user
                , error
            };
            // clang-format on

            using Results = std::tuple<juce::Result, std::string>;
            using History = std::vector<std::tuple<MessageType, juce::String>>;

            // juce::AsyncUpdater
            void handleAsyncUpdate() override;

            // juce::Timer
            void timerCallback() override;

            void initializeSystem();
            void sendUserQuery();
            void stopUserQuery();
            void updateHistory();

            class QueryEditor
            : public juce::TextEditor
            , private juce::TextEditor::Listener
            {
            public:
                QueryEditor(History const& history);
                virtual ~QueryEditor() override = default;

                // juce::TextEditor
                bool keyPressed(juce::KeyPress const& key) override;

                void resetHistoryIndex();

            private:
                // juce::TextEditor::Listener
                void textEditorTextChanged(juce::TextEditor&) override;

                History const& mHistory;
                juce::String mSavedText;
                int mHistoryIndex = -1;
                bool mShouldChange = true;
            };

            Accessor& mAccessor;
            Accessor::Listener mListener{typeid(*this).name()};
            std::atomic<bool> mIsInitialized{false};
            Agent& mAgent;
            History mHistory;
            juce::TextEditor mHistoryEditor;
            ColouredPanel mSeparator1;
            juce::TextEditor mTempResponse;
            ColouredPanel mSeparator2;
            QueryEditor mQueryEditor{mHistory};
            juce::DrawableButton mMenuButton{"Menu", juce::DrawableButton::ButtonStyle::ImageFitted};
            Icon mSendButton;
            ColouredPanel mSeparator3;
            juce::Label mStatusLabel;
            std::future<Results> mRequestFuture;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Chat)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
