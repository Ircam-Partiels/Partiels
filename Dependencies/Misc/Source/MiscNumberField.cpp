
#include "MiscNumberField.h"

MISC_FILE_BEGIN

NumberField::Label::Label()
{
    setRepaintsOnMouseActivity(true);
    setEditable(true);
    setMinimumHorizontalScale(1.0f);
    setBorderSize({});
    setRange({std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max()}, 0.0, juce::NotificationType::dontSendNotification);
    setKeyboardType(juce::TextInputTarget::VirtualKeyboardType::decimalKeyboard);
}

juce::TextEditor* NumberField::Label::createEditorComponent()
{
    auto* ed = juce::Label::createEditorComponent();
    if(ed != nullptr)
    {
        auto const ft = getFont();
        ed->setFont(ft);
        ed->setIndents(0, static_cast<int>(std::floor(ft.getDescent())));
        ed->setJustification(getJustificationType());
        ed->setBorder(getBorderSize());
        ed->setInputFilter(this, false);
        ed->setMultiLine(false);
    }
    return ed;
}

void NumberField::Label::editorShown(juce::TextEditor* ed)
{
    juce::Label::editorShown(ed);
    if(ed != nullptr)
    {
        auto const text = getText();
        if(mAcceptsOptional && (text.isEmpty() || text == mSubstitution))
        {
            ed->setText(mSubstitution, false);
        }
        else
        {
            ed->setText(getTextFromValue(mValue, true), false);
        }
        ed->moveCaretToTop(false);
        ed->moveCaretToEnd(true);
    }
}

void NumberField::Label::editorAboutToBeHidden(juce::TextEditor* ed)
{
    juce::Label::editorAboutToBeHidden(ed);
    setValue(getValue(), juce::NotificationType::dontSendNotification);
}

double NumberField::Label::getCorrespondingValue(double value) const
{
    MiscWeakAssert(!std::isnan(value) && std::isfinite(value));
    if(std::isnan(value) || !std::isfinite(value))
    {
        return mValue;
    }
    if(mInterval > 0.0)
    {
        value = std::round((value - mRange.getStart()) / mInterval) * mInterval + mRange.getStart();
    }
    return mRange.isEmpty() ? value : mRange.clipValue(value);
}

int NumberField::Label::getEffectiveNumDisplayedDecimmals() const
{
    return mNumDisplayedDecimals >= 0 ? mNumDisplayedDecimals : (mNumEditedDecimals > 0 ? mNumEditedDecimals : 0);
}

double NumberField::Label::getValueFromText(juce::String const& text) const
{
    if(valueFromString != nullptr)
    {
        return valueFromString(text);
    }
    return text.getDoubleValue();
}

juce::String NumberField::Label::getTextFromValue(double value, bool forEditor) const
{
    if(stringFromValue != nullptr)
    {
        return stringFromValue(value);
    }
    auto const toString = [&](int numDecimal)
    {
        return std::abs(value) < std::pow(10.0, static_cast<double>(-numDecimal)) ? juce::String(0.0, numDecimal) : juce::String(value, numDecimal);
    };
    if(forEditor)
    {
        return toString(mNumEditedDecimals);
    }
    auto const numDecimals = getEffectiveNumDisplayedDecimmals();
    return (numDecimals == 0 ? juce::String(static_cast<int>(value)) : toString(numDecimals)) + mSuffix;
}

void NumberField::Label::setValue(double value, juce::NotificationType const notification)
{
    value = getCorrespondingValue(value);
    if(std::abs(mValue - value) > std::numeric_limits<double>::epsilon())
    {
        mValue = value;
        setText(getTextFromValue(value, false), notification);
        if(auto* ed = getCurrentTextEditor())
        {
            ed->setText(getTextFromValue(value, true), notification == juce::NotificationType::dontSendNotification ? false : true);
        }
    }
    else
    {
        setText(getTextFromValue(value, false), juce::NotificationType::dontSendNotification);
        if(auto* ed = getCurrentTextEditor())
        {
            ed->setText(getTextFromValue(value, true), false);
        }
    }
}

double NumberField::Label::getValue() const
{
    return mValue;
}

void NumberField::Label::setRange(juce::Range<double> const& range, double interval, juce::NotificationType const notification)
{
    MiscWeakAssert(interval >= 0.0);
    interval = std::max(0.0, interval);
    if(range != mRange || std::abs(interval - mInterval) > std::numeric_limits<double>::epsilon())
    {
        mRange = range;
        mInterval = interval;

        auto getNumDecimals = [&]()
        {
            int numDecimalPlaces = 8;
            if(interval <= 0.0)
            {
                return numDecimalPlaces;
            }
            if(juce::approximatelyEqual(std::abs(interval - std::floor(interval)), 0.0))
            {
                return 0;
            }

            auto v = std::abs(juce::roundToInt(interval * static_cast<double>(pow(10, numDecimalPlaces))));
            while((v % 10) == 0 && numDecimalPlaces > 0)
            {
                --numDecimalPlaces;
                v /= 10;
            }

            return numDecimalPlaces;
        };
        mNumEditedDecimals = getNumDecimals();
        setValue(mValue, notification);
    }
}

juce::Range<double> NumberField::Label::getRange() const
{
    return mRange;
}

double NumberField::Label::getInterval() const
{
    return mInterval;
}

int NumberField::Label::getNumEditedDecimals() const
{
    return mNumEditedDecimals;
}

void NumberField::Label::setNumDecimalsDisplayed(int numDecimals)
{
    numDecimals = std::max(numDecimals, -1);
    if(mNumDisplayedDecimals != numDecimals)
    {
        mNumDisplayedDecimals = numDecimals;
        setValue(mValue, juce::NotificationType::dontSendNotification);
    }
}

int NumberField::Label::getNumDecimalsDisplayed() const
{
    return mNumDisplayedDecimals;
}

void NumberField::Label::setTextValueSuffix(juce::String const& suffix)
{
    if(mSuffix != suffix)
    {
        mSuffix = suffix;
        setValue(mValue, juce::NotificationType::dontSendNotification);
    }
}

juce::String NumberField::Label::getTextValueSuffix() const
{
    return mSuffix;
}

void NumberField::Label::textWasChanged()
{
    auto const text = getText(true);
    if(mAcceptsOptional && (text.isEmpty() || text == mSubstitution))
    {
        setText(mSubstitution, juce::NotificationType::dontSendNotification);
    }
    else
    {
        auto const numDecimals = getEffectiveNumDisplayedDecimmals();
        auto const epsilon = numDecimals >= 0 ? 1.0 / std::pow(10.0, static_cast<double>(numDecimals)) : std::numeric_limits<double>::epsilon();
        auto const newValue = getCorrespondingValue(getValueFromText(text));
        if(std::abs(newValue - mValue) >= epsilon)
        {
            mValue = newValue;
        }
    }
}

juce::String NumberField::Label::filterNewText(juce::TextEditor& ed, juce::String const& newInput)
{
    juce::ignoreUnused(ed);
    if(valueFromString != nullptr)
    {
        return newInput;
    }
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

void NumberField::Label::loadProperties(juce::NamedValueSet const& prop, juce::NotificationType const notification)
{
    auto const& suffix = prop.getWithDefault("suffix", mSuffix);
    MiscWeakAssert(suffix.isString());
    if(suffix.isString())
    {
        setTextValueSuffix(suffix.toString());
    }
    auto const& numDisplayDecimals = prop.getWithDefault("numDisplayDecimals", mNumDisplayedDecimals);
    MiscWeakAssert(numDisplayDecimals.isInt());
    if(numDisplayDecimals.isInt())
    {
        setNumDecimalsDisplayed(static_cast<int>(numDisplayDecimals));
    }
    auto const& rangeMin = prop.getWithDefault("rangeMin", mRange.getStart());
    auto const& rangeMax = prop.getWithDefault("rangeMax", mRange.getEnd());
    auto const& interval = prop.getWithDefault("interval", mInterval);
    MiscWeakAssert(rangeMin.isDouble() && rangeMax.isDouble() && interval.isDouble());
    if(rangeMin.isDouble() && rangeMax.isDouble() && interval.isDouble())
    {
        setRange({static_cast<double>(rangeMin), static_cast<double>(rangeMax)}, static_cast<double>(interval), notification);
    }
}

void NumberField::Label::storeProperties(juce::NamedValueSet& prop, juce::Range<double> const& range, double interval, int numDecimals, juce::String const& suffix)
{
    prop.set("rangeMin", range.getStart());
    prop.set("rangeMax", range.getEnd());
    prop.set("interval", interval);
    prop.set("numDisplayDecimals", numDecimals);
    prop.set("suffix", suffix);
}

void NumberField::Label::setAcceptsOptional(bool state, juce::String const& substitution)
{
    mAcceptsOptional = state;
    mSubstitution = substitution;
}

bool NumberField::Label::isOptionalAccepted() const
{
    return mAcceptsOptional;
}

juce::String NumberField::Label::getOptionalSubstitution() const
{
    return mSubstitution;
}

NumberField::NumberField()
{
    setValue(0.0, juce::NotificationType::dontSendNotification);
    mLabel.onEditorShow = [this]()
    {
        if(onEditorShow != nullptr)
        {
            onEditorShow();
        }
    };
    mLabel.onEditorHide = [this]()
    {
        if(onEditorHide != nullptr)
        {
            onEditorHide();
        }
    };
    mLabel.onTextChange = [this]()
    {
        if(onValueChanged != nullptr)
        {
            onValueChanged(getValue());
        }
    };

    addAndMakeVisible(mLabel);
}

void NumberField::setValue(double value, juce::NotificationType const notification)
{
    mLabel.setValue(value, notification);
}

double NumberField::getValue() const
{
    return mLabel.getValue();
}

bool NumberField::isOptional() const
{
    if(!mLabel.isOptionalAccepted())
    {
        return false;
    }
    return mLabel.getText() == mLabel.getOptionalSubstitution();
}

void NumberField::setRange(juce::Range<double> const& range, double interval, juce::NotificationType const notification)
{
    mLabel.setRange(range, interval, notification);
}

juce::Range<double> NumberField::getRange() const
{
    return mLabel.getRange();
}

double NumberField::getInterval() const
{
    return mLabel.getInterval();
}

int NumberField::getNumEditedDecimals() const
{
    return mLabel.getNumEditedDecimals();
}

void NumberField::setTextValueSuffix(juce::String const& suffix)
{
    mLabel.setTextValueSuffix(suffix);
}

juce::String NumberField::getTextValueSuffix() const
{
    return mLabel.getTextValueSuffix();
}

void NumberField::setNumDecimalsDisplayed(int numDecimals)
{
    mLabel.setNumDecimalsDisplayed(numDecimals);
}

int NumberField::getNumDecimalsDisplayed() const
{
    return mLabel.getNumDecimalsDisplayed();
}

void NumberField::setOptionalSupported(bool state, juce::String const& substitution)
{
    mLabel.setAcceptsOptional(state, substitution);
}

bool NumberField::isOptionalSupported() const
{
    return mLabel.isOptionalAccepted();
}

void NumberField::setJustificationType(juce::Justification newJustification)
{
    mLabel.setJustificationType(newJustification);
}

juce::Justification NumberField::getJustificationType() const
{
    return mLabel.getJustificationType();
}

void NumberField::setText(juce::String const& newText, juce::NotificationType notification)
{
    mLabel.setText(newText, notification);
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

void NumberField::setConversionFunctions(std::function<juce::String(double value)> stringFromValue, std::function<double(juce::String const& text)> valueFromString)
{
    mLabel.stringFromValue = stringFromValue;
    mLabel.valueFromString = valueFromString;
    setValue(getValue(), juce::NotificationType::dontSendNotification);
}

MISC_FILE_END
