#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class AlertWindow
{
public:
    // clang-format off
    enum class MessageType
    {
          unknown = juce::AlertWindow::AlertIconType::NoIcon
        , question = juce::AlertWindow::AlertIconType::QuestionIcon
        , warning = juce::AlertWindow::AlertIconType::WarningIcon
        , info = juce::AlertWindow::AlertIconType::InfoIcon
    };
    // clang-format on

    static void showMessage(MessageType const type, juce::String const& title, juce::String const& message, std::initializer_list<std::tuple<juce::String, juce::String>> replacements = {});

    static bool showOkCancel(MessageType const type, juce::String const& title, juce::String const& message, std::initializer_list<std::tuple<juce::String, juce::String>> replacements = {});

    // clang-format off
    enum class Answer
    {
          yes
        , no
        , cancel
    };
    // clang-format on

    static Answer showYesNoCancel(MessageType const type, juce::String const& title, juce::String const& message, std::initializer_list<std::tuple<juce::String, juce::String>> replacements = {});

    class Catcher
    {
    public:
        using entry_t = std::tuple<MessageType, juce::String>;
        Catcher() = default;
        ~Catcher() = default;

        void postMessage(MessageType const type, juce::String const title, juce::String const& message);
        std::map<entry_t, juce::StringArray> getMessages() const;
        void clearMessages();

    private:
        std::map<entry_t, juce::StringArray> mMessages;
    };
};

ANALYSE_FILE_END
