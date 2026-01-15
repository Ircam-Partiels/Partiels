#pragma once

#include "AnlApplicationNeuralyzerBackgroundAgent.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        // clang-format off
        enum class MessageType
        {
              assistant
            , user
            , system
            , tool
            , warning
        };
        // clang-format on

        using History = std::vector<std::pair<MessageType, juce::String>>;

        class ChatHistory
        : public juce::TextEditor
        {
        public:
            ChatHistory();
            ~ChatHistory() override = default;

            void setHistory(History const& history);

            std::function<void(juce::PopupMenu& menu)> onShowPopupMenu = nullptr;

            // juce::TextEditor
            void mouseMove(juce::MouseEvent const& event) override;
            void mouseDown(juce::MouseEvent const& event) override;
            void addPopupMenuItems(juce::PopupMenu& menu, juce::MouseEvent const* mouseClickEvent) override;

        private:
            using TextRange = juce::Range<int>;
            struct TableContent
            {
                using Row = std::vector<juce::String>;
                Row headers;
                std::vector<Row> rows;

                juce::String toCsv() const;
            };
            class TableView;
            struct ParseState;

            std::vector<std::pair<TextRange, juce::URL>> mLinks;
            std::vector<std::pair<TextRange, TableContent>> mTables;
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChatHistory)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
