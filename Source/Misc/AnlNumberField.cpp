
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
            editor->clear();
            auto const font = mLabel.getFont();
            editor->setFont(font);
            editor->setIndents(0, static_cast<int>(std::floor(font.getDescent())));
            editor->setJustification(mLabel.getJustificationType());
            editor->setBorder(mLabel.getBorderSize());
            editor->setInputFilter(this, false);
            editor->setMultiLine(false);
            editor->setText(juce::String(mValue, static_cast<int>(mNumEditedDecimals)));
            editor->moveCaretToTop(false);
            editor->moveCaretToEnd(true);
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
    if(mInterval > 0.0)
    {
        value = std::round((value - mRange.getStart()) / mInterval) * mInterval + mRange.getStart();
    }
    value = mRange.clipValue(value);
    mLabel.setText(juce::String(value, static_cast<int>(mNumDisplayedDecimals)) + mSuffix, juce::NotificationType::dontSendNotification);
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
            juce::MessageManager::callAsync([=, this]
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
                return 8_z;
            }
            auto numDecimals = 0_z;
            while(std::abs(interval - std::ceil(interval)) > std::numeric_limits<double>::epsilon() && numDecimals < 8)
            {
                interval *= 10.0;
                ++numDecimals;
            }
            return numDecimals;
        };
        auto numDecimals = getNumDecimals();
        if(mNumEditedDecimals != numDecimals)
        {
            mNumEditedDecimals = numDecimals;
            if(auto* editor = mLabel.getCurrentTextEditor())
            {
                editor->setText(juce::String(mValue, static_cast<int>(mNumEditedDecimals)));
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

void NumberField::setNumDecimalsDisplayed(size_t numDecimals)
{
    if(mNumDisplayedDecimals != numDecimals)
    {
        mNumDisplayedDecimals = numDecimals;
        setValue(mValue, juce::NotificationType::dontSendNotification);
    }
}

size_t NumberField::getNumDecimalsDisplayed() const
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
    if(mNumEditedDecimals == 0)
    {
        if(!newInput.containsOnly("-0123456789"))
        {
            getLookAndFeel().playAlertSound();
        }
        return newInput.retainCharacters("-0123456789");
    }
    auto const point = newInput.indexOfIgnoreCase(".");
    auto const numDecimals = point >= 0 ? newInput.length() - point : 0;
    if(!newInput.containsOnly("-0123456789.") || static_cast<size_t>(numDecimals) > mNumEditedDecimals)
    {
        getLookAndFeel().playAlertSound();
    }
    if(point >= 0)
    {
        return newInput.substring(0, point + static_cast<int>(mNumEditedDecimals)).retainCharacters("-0123456789");
    }
    return newInput.retainCharacters("-0123456789");
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
