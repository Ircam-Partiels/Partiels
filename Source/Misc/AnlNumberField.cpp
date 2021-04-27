
#include "AnlNumberField.h"

ANALYSE_FILE_BEGIN

NumberField::NumberField()
{
    mLabel.setRepaintsOnMouseActivity(true);
    mLabel.setEditable(true);
    mLabel.setMinimumHorizontalScale(1.0f);
    mLabel.setBorderSize({});
    mLabel.onEditorShow = [&]()
    {
        if(auto* editor = mLabel.getCurrentTextEditor())
        {
            editor->setText("", false);
            auto const font = mLabel.getFont();
            editor->setFont(font);
            editor->setIndents(0, static_cast<int>(std::floor(font.getDescent())));
            editor->setJustification(mLabel.getJustificationType());
            editor->setBorder(mLabel.getBorderSize());
            editor->setInputFilter(this, false);
            editor->setMultiLine(false);
            editor->setText(juce::String(mValue, mNumEditedDecimals), false);
            editor->moveCaretToTop(false);
            editor->moveCaretToEnd(true);
        }
        if(onEditorShow != nullptr)
        {
            onEditorShow();
        }
    };
    mLabel.onEditorHide = [&]()
    {
        if(onEditorHide != nullptr)
        {
            onEditorHide();
        }
    };
    mLabel.onTextChange = [&]()
    {
        setValue(mLabel.getText().getDoubleValue(), juce::NotificationType::sendNotificationSync);
    };

    addAndMakeVisible(mLabel);
}

void NumberField::setValue(double value, juce::NotificationType const notification)
{
    anlWeakAssert(!std::isnan(value) && std::isfinite(value));
    if(mInterval > 0.0)
    {
        value = std::round((value - mRange.getStart()) / mInterval) * mInterval + mRange.getStart();
    }
    value = mRange.clipValue(value);
    auto const numDecimals = mNumDisplayedDecimals >= 0 ? mNumDisplayedDecimals : (mNumEditedDecimals > 0 ? 2 : 0);
    mLabel.setText(juce::String(value, numDecimals) + mSuffix, juce::NotificationType::dontSendNotification);
    if(std::abs(value - mValue) > std::numeric_limits<double>::epsilon())
    {
        mValue = value;
        if(notification == juce::NotificationType::sendNotification || notification == juce::NotificationType::sendNotificationSync)
        {
            if(onValueChanged != nullptr)
            {
                onValueChanged(getValue());
            }
        }
        else if(notification == juce::NotificationType::sendNotificationAsync)
        {
            juce::WeakReference<juce::Component> target(this);
            juce::MessageManager::callAsync([=]
                                            {
                                                if(target.get() != nullptr && onValueChanged != nullptr)
                                                {
                                                    onValueChanged(getValue());
                                                }
                                            });
        }
    }
}

double NumberField::getValue() const
{
    return mValue;
}

void NumberField::setRange(juce::Range<double> const& range, double interval, juce::NotificationType const notification)
{
    anlWeakAssert(interval >= 0.0);
    interval = std::max(0.0, interval);
    if(range != mRange || std::abs(interval - mInterval) > std::numeric_limits<double>::epsilon())
    {
        mRange = range;
        mInterval = interval;
        setValue(mValue, notification);

        auto getNumDecimals = [&]()
        {
            if(interval <= 0.0)
            {
                return 8;
            }
            auto numDecimals = 0;
            auto startRange = range.getStart();
            while((std::fmod(interval, 1.0) > std::numeric_limits<double>::epsilon() || std::fmod(startRange, 1.0) > std::numeric_limits<double>::epsilon()) && numDecimals < 8)
            {
                interval *= 10.0;
                startRange *= 10.0;
                ++numDecimals;
            }
            return numDecimals;
        };
        auto const numDecimals = getNumDecimals();
        if(mNumEditedDecimals != numDecimals)
        {
            mNumEditedDecimals = numDecimals;
            if(auto* editor = mLabel.getCurrentTextEditor())
            {
                editor->setText(juce::String(mValue, mNumEditedDecimals), false);
            }
        }
    }
}

juce::Range<double> NumberField::getRange() const
{
    return mRange;
}

double NumberField::getInterval() const
{
    return mInterval;
}

void NumberField::setTextValueSuffix(juce::String const& suffix)
{
    if(mSuffix != suffix)
    {
        mSuffix = suffix;
        setValue(mValue, juce::NotificationType::dontSendNotification);
    }
}

juce::String NumberField::getTextValueSuffix() const
{
    return mSuffix;
}

void NumberField::setNumDecimalsDisplayed(int numDecimals)
{
    numDecimals = std::max(numDecimals, -1);
    if(mNumDisplayedDecimals != numDecimals)
    {
        mNumDisplayedDecimals = numDecimals;
        setValue(mValue, juce::NotificationType::dontSendNotification);
    }
}

int NumberField::getNumDecimalsDisplayed() const
{
    return mNumDisplayedDecimals;
}

void NumberField::setJustificationType(juce::Justification newJustification)
{
    mLabel.setJustificationType(newJustification);
}

juce::Justification NumberField::getJustificationType() const
{
    return mLabel.getJustificationType();
}

juce::String NumberField::filterNewText(juce::TextEditor& editor, juce::String const& newInput)
{
    juce::ignoreUnused(editor);
    if(mNumEditedDecimals <= 0)
    {
        if(!newInput.containsOnly("-0123456789"))
        {
            getLookAndFeel().playAlertSound();
        }
        return newInput.retainCharacters("-0123456789");
    }
    if(!newInput.containsOnly("-0123456789."))
    {
        getLookAndFeel().playAlertSound();
    }
    return newInput.retainCharacters("-0123456789.");
}

bool NumberField::isBeingEdited() const
{
    return mLabel.isBeingEdited();
}

void NumberField::setEditable(bool editOnSingleClick, bool editOnDoubleClick, bool lossOfFocusDiscards)
{
    mLabel.setEditable(editOnSingleClick, editOnDoubleClick, lossOfFocusDiscards);
}

juce::Font NumberField::getFont() const noexcept
{
    return mLabel.getFont();
}

void NumberField::setFont(juce::Font const& newFont)
{
    mLabel.setFont(newFont);
}

void NumberField::resized()
{
    mLabel.setBounds(getLocalBounds());
}

void NumberField::setTooltip(juce::String const& newTooltip)
{
    juce::SettableTooltipClient::setTooltip(newTooltip);
    mLabel.setTooltip(newTooltip);
}

ANALYSE_FILE_END
