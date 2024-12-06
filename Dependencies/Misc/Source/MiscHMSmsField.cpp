#include "MiscHMSmsField.h"

MISC_FILE_BEGIN

juce::String HMSmsField::Field::filterNewText(juce::TextEditor& editor, juce::String const& newInput)
{
    auto const extraChar = (canBeNegative && newInput.startsWith("-")) || (canBePositive && newInput.startsWith("+"));
    auto const maxLength = (suffix.getText() == "ms" ? 3 : 2) + (extraChar ? 1 : 0);
    auto const diff = editor.getTotalNumChars() - editor.getHighlightedRegion().getLength();
    auto const getPossibleCharacters = [this]()
    {
        if(canBeNegative && canBePositive)
        {
            return "-+0123456789";
        }
        if(canBeNegative)
        {
            return "-0123456789";
        }
        return "+0123456789";
    };
    return newInput.retainCharacters(getPossibleCharacters()).substring(0, maxLength - diff);
}

void HMSmsField::Field::setValue(int value)
{
    juce::String text;
    if(value < 0 && canBeNegative)
    {
        text += "-";
    }
    value = abs(value);
    if(suffix.getText() == "ms")
    {
        value = std::clamp(value, 0, 999);
        if(value < 100)
        {
            text += "0";
        }
    }
    else
    {
        value = std::clamp(value, 0, 99);
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
    return entry.getText(true).getIntValue();
}

HMSmsField::HMSmsField()
{
    mFields[0].suffix.setText("h", juce::NotificationType::dontSendNotification);
    mFields[1].suffix.setText("m", juce::NotificationType::dontSendNotification);
    mFields[2].suffix.setText("s", juce::NotificationType::dontSendNotification);
    mFields[3].suffix.setText("ms", juce::NotificationType::dontSendNotification);
    setRange({}, juce::NotificationType::dontSendNotification);

    juce::WeakReference<juce::Component> weakReference(this);
    for(size_t i = 0; i < mFields.size(); ++i)
    {
        mFields[i].entry.setEditable(true, true, false);
        mFields[i].suffix.setEditable(false, false, false);
        mFields[i].suffix.addMouseListener(&mFields[i].entry, true);

        mFields[i].entry.onEditorShow = [=, this]()
        {
            MiscWeakAssert(weakReference.get() != nullptr);
            if(weakReference.get() == nullptr)
            {
                return;
            }
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
                        mNewValues[j] = 0;
                    }
                };
            }
            if(!mIsEdited)
            {
                for(size_t j = 0; j < mFields.size(); ++j)
                {
                    mNewValues[j] = mSavedValues[j] = mFields[j].getValue();
                }
            }
            mIsEdited = true;
        };

        mFields[i].entry.onTextChange = [=, this]()
        {
            MiscWeakAssert(weakReference.get() != nullptr);
            if(weakReference.get() == nullptr)
            {
                return;
            }
            setTime(getTime(), juce::NotificationType::sendNotification);
            for(size_t j = 0; j < mFields.size(); ++j)
            {
                mSavedValues[j] = mFields[j].getValue();
            }
        };

        mFields[i].entry.onEditorHide = [=, this]()
        {
            MiscWeakAssert(weakReference.get() != nullptr);
            if(weakReference.get() == nullptr)
            {
                return;
            }
            mFields[i].setValue(mNewValues[i]);
            handleAsyncUpdate();
        };

        addAndMakeVisible(mFields[i].suffix);
        mFields[i].suffix.setColour(juce::Label::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
        mFields[i].entry.setRepaintsOnMouseActivity(true);
        mFields[i].entry.setMinimumHorizontalScale(1.0f);
        mFields[i].suffix.setMinimumHorizontalScale(1.0f);
        mFields[i].entry.setBorderSize({0, 0, 0, -1});
        mFields[i].suffix.setBorderSize({0, -1, 0, 0});
        mFields[i].entry.setJustificationType(juce::Justification::centredRight);
        mFields[i].suffix.setJustificationType(juce::Justification::centredLeft);
    }

    for(size_t i = 0; i < mFields.size(); ++i)
    {
        addAndMakeVisible(mFields[i].entry);
    }

    setEditable(true, true, false);
    setTime(0.0, juce::NotificationType::dontSendNotification);
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
        else
        {
            setTime(getTime(), juce::NotificationType::sendNotification);
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
                                               return accum + static_cast<double>(juce::GlyphArrangement::getStringWidth(font, text)) + caretSize;
                                           });

    auto bounds = getLocalBounds();
    auto const elementWidth = static_cast<double>(bounds.getWidth()) / fullwidth;
    for(auto& field : mFields)
    {
        auto const ratioEntry = static_cast<double>(juce::GlyphArrangement::getStringWidth(font, field.entry.getText())) + caretSize;
        field.entry.setBounds(bounds.removeFromLeft(static_cast<int>(std::round(ratioEntry * elementWidth))));
        auto const ratioSuffix = static_cast<double>(juce::GlyphArrangement::getStringWidth(font, field.suffix.getText()));
        field.suffix.setBounds(bounds.removeFromLeft(static_cast<int>(std::round(ratioSuffix * elementWidth))));
    }
}

void HMSmsField::enablementChanged()
{
    for(size_t index = 0_z; index < mFields.size() - 1_z; ++index)
    {
        if(mFields[index].entry.isEnabled())
        {
            for(size_t next = index + 1_z; next < mFields.size(); ++next)
            {
                mFields[index].entry.removeMouseListener(&mFields[next].entry);
                mFields[index].entry.removeMouseListener(&mFields[next].suffix);
            }
        }
        else
        {
            for(size_t next = index + 1_z; next < mFields.size(); ++next)
            {
                if(mFields[next].entry.isEnabled())
                {
                    mFields[index].entry.addMouseListener(&mFields[next].entry, true);
                    mFields[index].suffix.addMouseListener(&mFields[next].entry, true);
                    break;
                }
            }
        }
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

void HMSmsField::setHideDisabledFields(bool shouldHide)
{
    mHideDisabledFields = shouldHide;
    for(auto& field : mFields)
    {
        field.entry.setVisible(field.entry.isEnabled() || !mHideDisabledFields);
        field.suffix.setVisible(field.entry.isEnabled() || !mHideDisabledFields);
    }
}

void HMSmsField::setRange(juce::Range<double> const& range, juce::NotificationType const notification)
{
    if(mRange == range)
    {
        return;
    }
    mRange = range;
    auto const maxInSeconds = std::max(std::abs(range.getStart()), std::abs(range.getEnd()));
    auto const hours = static_cast<int>(std::min(std::floor(maxInSeconds / 3600.0), 99.0));
    auto const minutes = static_cast<int>(std::min(std::floor(maxInSeconds / 60.0), 99.0));
    auto const seconds = static_cast<int>(std::min(std::floor(maxInSeconds), 99.0));
    auto const milliseconds = static_cast<int>(std::min(std::round(maxInSeconds * 1000.0), 999.0));

    auto const currentTime = getTime();
    mFields[0].maxValue = !mRange.isEmpty() ? hours : 99;
    mFields[0].entry.setEnabled(mFields[0].maxValue > 0);
    mFields[0].suffix.setEnabled(mFields[0].maxValue > 0);
    mFields[1].maxValue = !mRange.isEmpty() ? minutes : 99;
    mFields[1].entry.setEnabled(mFields[1].maxValue > 0);
    mFields[1].suffix.setEnabled(mFields[1].maxValue > 0);
    mFields[2].maxValue = !mRange.isEmpty() ? seconds : 99;
    mFields[2].entry.setEnabled(mFields[2].maxValue > 0);
    mFields[2].suffix.setEnabled(mFields[2].maxValue > 0);
    mFields[3].maxValue = !mRange.isEmpty() ? milliseconds : 999;
    mFields[3].entry.setEnabled(mFields[3].maxValue > 0);
    mFields[3].suffix.setEnabled(mFields[3].maxValue > 0);
    auto const canBeNegative = mRange.isEmpty() || mRange.getStart() < 0.0;
    auto const canBePositive = mRange.isEmpty() || mRange.getEnd() > 0.0;
    for(auto& field : mFields)
    {
        field.canBeNegative = canBeNegative;
        field.canBePositive = canBePositive;
    }
    setHideDisabledFields(mHideDisabledFields);
    enablementChanged();

    setTime(currentTime, notification);
}

void HMSmsField::setTime(double timeInSeconds, juce::NotificationType const notification)
{
    auto isNegative = mRange.getStart() < 0.0 && timeInSeconds < 0.0;
    timeInSeconds = !mRange.isEmpty() ? mRange.clipValue(timeInSeconds) : timeInSeconds;
    auto const hours = static_cast<int>(std::floor(std::abs(timeInSeconds) / 3600.0));
    auto const minutes = static_cast<int>(std::floor((std::abs(timeInSeconds) - (hours * 3600.0)) / 60.0));
    auto const seconds = static_cast<int>(std::floor(std::abs(timeInSeconds) - (hours * 3600.0) - (minutes * 60.0)));
    auto const milliseconds = static_cast<int>(std::round(std::fmod(std::abs(timeInSeconds) * 1000.0, 1000.0)));

    auto const parseValue = [&](auto& field, int value)
    {
        auto const canBeNegative = std::exchange(isNegative, field.entry.isVisible() ? false : isNegative);
        field.setValue(canBeNegative ? -value : value);
    };
    parseValue(mFields[0], hours);
    parseValue(mFields[1], minutes);
    parseValue(mFields[2], seconds);
    parseValue(mFields[3], milliseconds);

    if(std::abs(mSavedTime - timeInSeconds) < std::numeric_limits<double>::epsilon())
    {
        return;
    }
    mSavedTime = timeInSeconds;
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
    auto const time = std::abs(hours) + std::abs(minutes) + std::abs(seconds) + std::abs(milliseconds);
    auto const isNegative = hours < 0.0 || minutes < 0.0 || seconds < 0.0 || milliseconds < 0.0;
    return mRange.isEmpty() ? isNegative ? -time : time : mRange.clipValue(isNegative ? -time : time);
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

MISC_FILE_END
