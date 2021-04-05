#include "AnlHMSmsField.h"

ANALYSE_FILE_BEGIN

juce::String HMSmsField::Field::filterNewText(juce::TextEditor& editor, juce::String const& newInput)
{
    auto const maxLength = suffix.getText() == "ms" ? 3 : 2;
    auto const diff = editor.getTotalNumChars() - editor.getHighlightedRegion().getLength();
    return newInput.retainCharacters("0123456789").substring(0, maxLength - diff);
}

void HMSmsField::Field::setValue(int value)
{
    juce::String text;
    if(suffix.getText() == "ms")
    {
        value = std::min(std::max(value, 0), 999);
        if(value < 100)
        {
            text += "0";
        }
    }
    else
    {
        value = std::min(std::max(value, 0), 99);
    }
    if(value < 10)
    {
        text += "0";
    }
    text += juce::String(value);
    entry.setText(text, juce::NotificationType::dontSendNotification);
}

int HMSmsField::Field::getValue() const
{
    if(entry.isBeingEdited())
    {
        auto* editor = entry.getCurrentTextEditor();
        if(editor != nullptr)
        {
            return editor->getText().getIntValue();
        }
    }
    return entry.getText().getIntValue();
}

HMSmsField::HMSmsField()
{
    mFields[0].suffix.setText("h", juce::NotificationType::dontSendNotification);
    mFields[1].suffix.setText("m", juce::NotificationType::dontSendNotification);
    mFields[2].suffix.setText("s", juce::NotificationType::dontSendNotification);
    mFields[3].suffix.setText("ms", juce::NotificationType::dontSendNotification);
    
    for(size_t i = 0; i < mFields.size(); ++i)
    {
        mFields[i].entry.setEditable(true, true, false);
        mFields[i].suffix.setEditable(false, false, false);

        mFields[i].entry.onEditorShow = [this, i]()
        {
            auto* editor = mFields[i].entry.getCurrentTextEditor();
            if(editor != nullptr)
            {
                editor->setInputFilter(&mFields[i], false);
                editor->setJustification(mFields[i].entry.getJustificationType());
                editor->setMultiLine(false);
                auto const font = mFields[i].entry.getFont();
                editor->setFont(font);
                editor->setIndents(0, 0);
                editor->setBorder(mFields[i].entry.getBorderSize());
                
                editor->onTextChange = [this, editor, i]()
                {
                    mNewValues[i] = editor->getText().getIntValue();
                    for(size_t j = i + 1; j < mFields.size(); ++j)
                    {
                        mFields[j].setValue(0);
                    }
                };
            }
            if(!mIsEdited)
            {
                for(size_t j = 0; j < mFields.size(); ++j)
                {
                    mNewValues[j] = mSavedValues[j] = mFields[i].getValue();
                }
            }
            mIsEdited = true;
        };

        mFields[i].entry.onTextChange = [this]()
        {
            setTime(getTime(), juce::NotificationType::sendNotification);
            for(size_t j = 0; j < mFields.size(); ++j)
            {
                mSavedValues[j] = mFields[j].getValue();
            }
        };

        mFields[i].entry.onEditorHide = [this, i]()
        {
            mFields[i].setValue(mNewValues[i]);
            triggerAsyncUpdate();
        };
        
        addAndMakeVisible(mFields[i].entry);
        addAndMakeVisible(mFields[i].suffix);
        mFields[i].entry.setRepaintsOnMouseActivity(true);
        mFields[i].entry.setMinimumHorizontalScale(1.0f);
        mFields[i].suffix.setMinimumHorizontalScale(1.0f);
        mFields[i].entry.setBorderSize({});
        mFields[i].suffix.setBorderSize({});
        mFields[i].entry.setJustificationType(juce::Justification::centredRight);
        mFields[i].suffix.setJustificationType(juce::Justification::centredLeft);
    }
    
    setEditable(true, true, false);
    setWantsKeyboardFocus(true);
}

void HMSmsField::handleAsyncUpdate()
{
    if(mIsEdited && std::none_of(mFields.cbegin(), mFields.cend(), [](auto const& field)
    {
        return field.entry.isBeingEdited();
    }))
    {
        mIsEdited = false;
        if(mLossOfFocusDiscards)
        {
            for(size_t j = 0; j < mFields.size(); ++j)
            {
                mFields[j].setValue(mSavedValues[j]);
            }
        }
    }
    else if(!mIsEdited)
    {
        if(onTimeChanged != nullptr)
        {
            onTimeChanged(getTime());
        }
    }
}

void HMSmsField::resized()
{
    auto constexpr caretSize = 3.0;
    auto const font = mFields.front().entry.getFont();
    auto const fullwidth = std::accumulate(mFields.cbegin(), mFields.cend(), 0.0, [&](auto const& accum, auto const& rhs)
    {
        auto const text = rhs.entry.getText() + rhs.suffix.getText();
        return accum + static_cast<double>(font.getStringWidthFloat(text)) + caretSize;
    });
    
    auto bounds = getLocalBounds();
    auto const elementWidth = static_cast<double>(bounds.getWidth()) / fullwidth;
    for(auto& field : mFields)
    {
        auto const ratioEntry = static_cast<double>(font.getStringWidthFloat(field.entry.getText())) + caretSize;
        field.entry.setBounds(bounds.removeFromLeft(static_cast<int>(std::round(ratioEntry * elementWidth))));
        auto const ratioSuffix = static_cast<double>(font.getStringWidthFloat(field.suffix.getText()));
        field.suffix.setBounds(bounds.removeFromLeft(static_cast<int>(std::round(ratioSuffix * elementWidth))));
    }
}

void HMSmsField::setFont(juce::Font const& newFont)
{
    for(auto& field : mFields)
    {
        field.entry.setFont(newFont);
        field.suffix.setFont(newFont);
    }
}

void HMSmsField::setEditable(bool editOnSingleClick, bool editOnDoubleClick, bool lossOfFocusDiscards)
{
    mLossOfFocusDiscards = lossOfFocusDiscards;
    for(auto& field : mFields)
    {
        field.entry.setEditable(editOnSingleClick, editOnDoubleClick, true);
    }
}

void HMSmsField::setJustificationType(juce::Justification newJustification)
{
    for(auto& field : mFields)
    {
        field.entry.setJustificationType(newJustification);
    }
}

void HMSmsField::setTime(double const timeInSeconds, juce::NotificationType const notification)
{
    auto const hours = static_cast<int>(std::floor(timeInSeconds / 3600.0));
    auto const minutes = static_cast<int>(std::floor((timeInSeconds - (hours * 3600.0)) / 60.0));
    auto const seconds = static_cast<int>(std::floor(timeInSeconds  - (hours * 3600.0) - (minutes * 60.0)));
    auto const milliseconds = static_cast<int>(std::round(std::fmod(timeInSeconds * 1000.0, 1000.0)));

    mFields[0].setValue(hours);
    mFields[1].setValue(minutes);
    mFields[2].setValue(seconds);
    mFields[3].setValue(milliseconds);
    
    if(notification != juce::NotificationType::dontSendNotification)
    {
        if(notification == juce::NotificationType::sendNotificationAsync)
        {
            triggerAsyncUpdate();
        }
        else if(onTimeChanged != nullptr)
        {
            onTimeChanged(getTime());
        }
    }
}

double HMSmsField::getTime() const
{
    auto const hours = static_cast<double>(mFields[0].getValue()) * 3600.0;
    auto const minutes = static_cast<double>(mFields[1].getValue()) * 60.0;
    auto const seconds = static_cast<double>(mFields[2].getValue());
    auto const milliseconds = static_cast<double>(mFields[3].getValue()) / 1000.0;
    return hours + minutes + seconds + milliseconds;
}

void HMSmsField::setTooltip(juce::String const& newTooltip)
{
    juce::SettableTooltipClient::setTooltip(newTooltip);
    for(auto& field : mFields)
    {
        field.entry.setTooltip(newTooltip);
        field.suffix.setTooltip(newTooltip);
    }
}

ANALYSE_FILE_END
