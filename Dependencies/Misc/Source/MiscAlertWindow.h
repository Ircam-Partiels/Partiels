#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

class AlertWindow
{
public:
    // clang-format off
    enum class MessageType
    {
          unknown = static_cast<int>(juce::MessageBoxIconType::NoIcon)
        , question = static_cast<int>(juce::MessageBoxIconType::QuestionIcon)
        , warning = static_cast<int>(juce::MessageBoxIconType::WarningIcon)
        , info = static_cast<int>(juce::MessageBoxIconType::InfoIcon)
    };
    // clang-format on

#if JUCE_MODAL_LOOPS_PERMITTED

    // clang-format off
    enum class Answer
    {
          yes
        , no
        , cancel
    };
    // clang-format on

    static void showMessage(MessageType const type, juce::String const& title, juce::String const& message, std::initializer_list<std::tuple<juce::String, juce::String>> replacements = {});

    static bool showOkCancel(MessageType const type, juce::String const& title, juce::String const& message, std::initializer_list<std::tuple<juce::String, juce::String>> replacements = {});

    static Answer showYesNoCancel(MessageType const type, juce::String const& title, juce::String const& message, std::initializer_list<std::tuple<juce::String, juce::String>> replacements = {});
#endif

    class Catcher
    {
    public:
        using entry_t = std::tuple<MessageType, juce::String>;
        Catcher() = default;
        ~Catcher() = default;

        void postMessage(MessageType const type, juce::String const title, juce::String const& message);
        std::map<entry_t, juce::StringArray> getMessages() const;
        void clearMessages();
        void showAsync() const;

    private:
        std::map<entry_t, juce::StringArray> mMessages;
    };
};

MISC_FILE_END
