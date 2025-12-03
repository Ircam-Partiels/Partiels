#pragma once

#include "AnlApplicationLlamaChat.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace CoAnalyzer
    {
        // clang-format off
        enum class AttrType : size_t
        {
              model
        };
        
        using AttrContainer = Model::Container
        < Model::Attr<AttrType::model, juce::File, Model::Flag::basic>
        >;
        // clang-format on

        class Accessor
        : public Model::Accessor<Accessor, AttrContainer>
        {
        public:
            using Model::Accessor<Accessor, AttrContainer>::Accessor;
            // clang-format off
            Accessor()
            : Accessor(AttrContainer({
                                          {}
                                    }))
            {
            }
            // clang-format on
        };

        class SettingsContent
        : public juce::Component
        {
        public:
            SettingsContent();
            ~SettingsContent() override;

            // juce::Component
            void resized() override;
            void parentHierarchyChanged() override;
            void visibilityChanged() override;

        private:
            Accessor::Listener mListener{typeid(*this).name()};
            PropertyList mModel;
            PropertyTextButton mResetState;
            std::vector<juce::File> mInstalledModels;
            TimerClock mTimerClock;
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsContent)
        };

        class SettingsPanel
        : public HideablePanel
        {
        public:
            SettingsPanel();
            ~SettingsPanel() override;

        private:
            SettingsContent mContent;
        };

        class Chat
        : public juce::Component
        , private juce::AsyncUpdater
        , private juce::Timer
        {
        public:
            Chat(Accessor& accessor);
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

            // juce::AsyncUpdater
            void handleAsyncUpdate() override;

            // juce::Timer
            void timerCallback() override;

            void initializeSystem();
            void sendUserQuery();
            void stopUserQuery();
            void updateHistory();

            using Results = std::tuple<juce::Result, juce::String, juce::String>;
            using History = std::vector<std::tuple<MessageType, juce::String>>;

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
            std::atomic<bool> mShouldQuit{false};
            Llama::Chat mChat{mShouldQuit};
            History mHistory;
            juce::TextEditor mHistoryEditor;
            juce::TextEditor mTempResponse;
            ColouredPanel mSeparator1;
            QueryEditor mQueryEditor{mHistory};
            Icon mSendButton;
            ColouredPanel mSeparator2;
            juce::Label mStatusLabel;
            std::future<Results> mRequestFuture;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Chat)
        };
    } // namespace CoAnalyzer
} // namespace Application

ANALYSE_FILE_END
