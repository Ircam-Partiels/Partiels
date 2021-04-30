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

    static void show(MessageType const type, juce::String const& title, juce::String const& message, std::initializer_list<std::tuple<juce::String, juce::String>> replacements = {});

    static bool showOkCancel(MessageType const type, juce::String const& title, juce::String const& message, std::initializer_list<std::tuple<juce::String, juce::String>> replacements = {});
};

ANALYSE_FILE_END
