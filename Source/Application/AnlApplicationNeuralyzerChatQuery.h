#pragma once

#include "AnlApplicationNeuralyzerBackgroundAgent.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        class ChatQuery
        : public juce::TextEditor
        , private juce::TextEditor::Listener
        , public juce::FileDragAndDropTarget
        {
        public:
            ChatQuery();
            virtual ~ChatQuery() override = default;

            std::function<bool(juce::String const&)> isInterestedInFile;
            std::function<void(juce::StringArray const&)> onFilesDragEnter;
            std::function<void()> onFilesDragExit;
            std::function<void(juce::StringArray const&)> onFilesDropped;

            void setHistory(Agent::History const& history);
            void resetHistoryIndex();

            // juce::TextEditor
            bool keyPressed(juce::KeyPress const& key) override;

            // juce::FileDragAndDropTarget
            bool isInterestedInFileDrag(juce::StringArray const& files) override;
            void fileDragEnter(juce::StringArray const& files, int x, int y) override;
            void fileDragExit(juce::StringArray const& files) override;
            void filesDropped(juce::StringArray const& files, int x, int y) override;

        private:
            // juce::TextEditor::Listener
            void textEditorTextChanged(juce::TextEditor&) override;

            Agent::History mHistory;
            juce::String mSavedText;
            int mHistoryIndex = -1;
            bool mShouldChange = true;
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChatQuery)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
