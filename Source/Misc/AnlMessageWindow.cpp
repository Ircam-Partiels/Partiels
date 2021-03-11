#include "AnlMessageWindow.h"

ANALYSE_FILE_BEGIN

void MessageWindow::show(MessageType const type, juce::String const& title, juce::String const& message, std::initializer_list<std::tuple<juce::String, juce::String>> replacements)
{
#ifdef JUCE_DEBUG
    auto getTypeAsText = [&]()
    {
        switch(type)
        {
            case MessageType::unknown:
                return "Unknown";
            case MessageType::question:
                return "Question";
            case MessageType::warning:
                return "Warning";
            case MessageType::info:
                return "Info";
        }
        return "Unknown";
    };
#endif
    auto text = juce::translate(message);
    for(auto const& replacement : replacements)
    {
        text.replace(std::get<0>(replacement), std::get<1>(replacement));
    }
    DBG("[MessageWindow][" << getTypeAsText() << "][" << juce::translate(title) << "][" << text << "]");
    juce::AlertWindow::showMessageBox(static_cast<juce::AlertWindow::AlertIconType>(type), juce::translate(title), text);
}

ANALYSE_FILE_END
