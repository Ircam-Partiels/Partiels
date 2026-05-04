#pragma once

#include "../Document/AnlDocumentTools.h"
#include "AnlApplicationNeuralyzerBackgroundAgent.h"
#include "AnlApplicationNeuralyzerRag.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        class Chat
        : public juce::Component
        , private juce::AsyncUpdater
        , private juce::Timer
        , private juce::ChangeListener
        {
        public:
            Chat(Accessor& accessor, BackgroundAgent& agent, Rag::Engine& ragEngine);
            ~Chat() override;

            // juce::Component
            void paint(juce::Graphics& g) override;
            void paintOverChildren(juce::Graphics& g) override;
            void resized() override;
            void colourChanged() override;
            void parentHierarchyChanged() override;
            void mouseDown(juce::MouseEvent const& event) override;

        private:
            using MessageType = Agent::MessageType;
            using History = Agent::History;

            // juce::AsyncUpdater
            void handleAsyncUpdate() override;

            // juce::Timer
            void timerCallback() override;

            // juce::ChangeListener
            void changeListenerCallback(juce::ChangeBroadcaster* source) override;

            void initializeSystem();
            void sendUserQuery();
            void stopUserQuery();
            void clearHistory();
            void updateHistory();

            class QueryEditor
            : public juce::TextEditor
            , private juce::TextEditor::Listener
            , public juce::FileDragAndDropTarget
            {
            public:
                QueryEditor(History const& history);
                virtual ~QueryEditor() override = default;

                std::function<void(juce::StringArray const&)> onFilesDragEnter;
                std::function<void()> onFilesDragExit;
                std::function<void(juce::StringArray const&)> onFilesDropped;

                // juce::TextEditor
                bool keyPressed(juce::KeyPress const& key) override;

                void resetHistoryIndex();

                // juce::FileDragAndDropTarget
                bool isInterestedInFileDrag(juce::StringArray const& files) override;
                void fileDragEnter(juce::StringArray const& files, int x, int y) override;
                void fileDragExit(juce::StringArray const& files) override;
                void filesDropped(juce::StringArray const& files, int x, int y) override;

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
            BackgroundAgent& mAgent;
            Rag::Engine& mRagEngine;
            History mHistory;
            juce::TextEditor mHistoryEditor;
            ColouredPanel mSeparator1;
            juce::TextEditor mTempResponse;
            ColouredPanel mSeparator2;
            QueryEditor mQueryEditor{mHistory};
            juce::DrawableButton mMenuButton{"Menu", juce::DrawableButton::ButtonStyle::ImageFitted};
            Icon mSendButton;
            Icon mSelectionStateButton;
            Icon mAttachedFilesButton;
            ColouredPanel mSeparator3;
            juce::Label mStatusLabel;
            juce::Label mUsageLabel;
            juce::StringArray mDroppedFilePaths;
            static juce::File mFullHistoryFile;
            bool mIsDragging{false};
            std::unique_ptr<juce::FileChooser> mFileChooser;
            Document::LayoutNotifier mSelectionLayoutNotifier;
            Transport::Accessor::Listener mTransportListener{typeid(*this).name()};
            Document::Accessor mLastDocumentAccessorState;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Chat)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
