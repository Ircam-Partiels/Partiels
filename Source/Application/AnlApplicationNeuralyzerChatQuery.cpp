#include "AnlApplicationNeuralyzerChatQuery.h"

ANALYSE_FILE_BEGIN

Application::Neuralyzer::ChatQuery::ChatQuery()
{
    addListener(this);
}

bool Application::Neuralyzer::ChatQuery::keyPressed(juce::KeyPress const& key)
{
    using MessageType = Agent::MessageType;
    auto const numUserMessages = std::count_if(mHistory.cbegin(), mHistory.cend(), [](auto const& message)
                                               {
                                                   return std::get<0_z>(message) == MessageType::user;
                                               });
    if(key.isKeyCode(juce::KeyPress::upKey) && getCaretPosition() == 0 && mHistoryIndex < numUserMessages)
    {
        int index = 0;
        for(auto historyIt = mHistory.crbegin(); historyIt != mHistory.crend(); ++historyIt)
        {
            if(std::get<0_z>(*historyIt) == MessageType::user)
            {
                if(index == mHistoryIndex + 1)
                {
                    ++mHistoryIndex;
                    mShouldChange = false;
                    setText(std::get<1_z>(*historyIt), true);
                    return true;
                }
                ++index;
            }
        }
    }
    else if(key.isKeyCode(juce::KeyPress::downKey) && getCaretPosition() == getTotalNumChars() && mHistoryIndex > -1)
    {
        if(mHistoryIndex > 0)
        {
            int index = 0;
            for(auto historyIt = mHistory.crbegin(); historyIt != mHistory.crend(); ++historyIt)
            {
                if(std::get<0_z>(*historyIt) == MessageType::user)
                {
                    if(index == mHistoryIndex - 1)
                    {
                        --mHistoryIndex;
                        mShouldChange = false;
                        setText(std::get<1_z>(*historyIt), true);
                        return true;
                    }
                    ++index;
                }
            }
        }
        else
        {
            mHistoryIndex = -1;
            mShouldChange = false;
            setText(mSavedText, true);
            return true;
        }
    }
    return juce::TextEditor::keyPressed(key);
}

void Application::Neuralyzer::ChatQuery::setHistory(Agent::History const& history)
{
    mHistory = history;
}

void Application::Neuralyzer::ChatQuery::resetHistoryIndex()
{
    mHistoryIndex = -1;
}

void Application::Neuralyzer::ChatQuery::textEditorTextChanged(juce::TextEditor&)
{
    if(std::exchange(mShouldChange, true))
    {
        mSavedText = getText();
    }
}

bool Application::Neuralyzer::ChatQuery::isInterestedInFileDrag(juce::StringArray const& files)
{
    return isInterestedInFile != nullptr ? std::any_of(files.begin(), files.end(), isInterestedInFile) : false;
}

void Application::Neuralyzer::ChatQuery::fileDragEnter(juce::StringArray const& files, [[maybe_unused]] int x, [[maybe_unused]] int y)
{
    if(onFilesDragEnter != nullptr)
    {
        onFilesDragEnter(files);
    }
}

void Application::Neuralyzer::ChatQuery::fileDragExit([[maybe_unused]] juce::StringArray const& files)
{
    if(onFilesDragExit != nullptr)
    {
        onFilesDragExit();
    }
}

void Application::Neuralyzer::ChatQuery::filesDropped(juce::StringArray const& files, [[maybe_unused]] int x, [[maybe_unused]] int y)
{
    if(onFilesDropped != nullptr)
    {
        onFilesDropped(files);
    }
}

ANALYSE_FILE_END
