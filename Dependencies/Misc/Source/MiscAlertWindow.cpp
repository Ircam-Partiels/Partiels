#include "MiscAlertWindow.h"

MISC_FILE_BEGIN

#if JUCE_MODAL_LOOPS_PERMITTED
void AlertWindow::showMessage(MessageType const type, juce::String const& title, juce::String const& message, std::initializer_list<std::tuple<juce::String, juce::String>> replacements)
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
        text = text.replace(std::get<0>(replacement), std::get<1>(replacement));
    }
    DBG("[MessageWindow][" << getTypeAsText() << "][" << juce::translate(title) << "][" << text << "]");
    juce::AlertWindow::showMessageBox(static_cast<juce::AlertWindow::AlertIconType>(type), juce::translate(title), text);
}

bool AlertWindow::showOkCancel(MessageType const type, juce::String const& title, juce::String const& message, std::initializer_list<std::tuple<juce::String, juce::String>> replacements)
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
        text = text.replace(std::get<0>(replacement), std::get<1>(replacement));
    }
    DBG("[MessageWindow][" << getTypeAsText() << "][" << juce::translate(title) << "][" << text << "]");
    return juce::AlertWindow::showOkCancelBox(static_cast<juce::AlertWindow::AlertIconType>(type), juce::translate(title), text);
}

AlertWindow::Answer AlertWindow::showYesNoCancel(MessageType const type, juce::String const& title, juce::String const& message, std::initializer_list<std::tuple<juce::String, juce::String>> replacements)
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
        text = text.replace(std::get<0>(replacement), std::get<1>(replacement));
    }
    DBG("[MessageWindow][" << getTypeAsText() << "][" << juce::translate(title) << "][" << text << "]");
    auto const result = juce::AlertWindow::showYesNoCancelBox(static_cast<juce::AlertWindow::AlertIconType>(type), juce::translate(title), text);
    switch(result)
    {
        case 1:
            return Answer::yes;
        case 0:
            return Answer::cancel;
        default:
            return Answer::no;
    }
}
#endif

void AlertWindow::Catcher::postMessage(MessageType const type, juce::String const title, juce::String const& message)
{
    mMessages[std::make_tuple(type, title)].add(message);
}

std::map<AlertWindow::Catcher::entry_t, juce::StringArray> AlertWindow::Catcher::getMessages() const
{
    return mMessages;
}

void AlertWindow::Catcher::clearMessages()
{
    mMessages.clear();
}

void AlertWindow::Catcher::showAsync() const
{
    for(auto const& message : getMessages())
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(static_cast<juce::MessageBoxIconType>(std::get<0>(message.first)))
                                 .withTitle(std::get<1>(message.first) + (message.second.size() > 1 ? "(" + juce::String(message.second.size()) + ")" : ""))
                                 .withMessage(message.second.joinIntoString("\n"))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    }
}

MISC_FILE_END
