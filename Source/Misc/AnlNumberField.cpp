
#include "AnlNumberField.h"

ANALYSE_FILE_BEGIN

NumberField::Label::Label()
{
    setRepaintsOnMouseActivity(true);
    setEditable(true);
    setMinimumHorizontalScale(1.0f);
    setBorderSize({});
    setRange({std::numeric_limits<double>::min(), std::numeric_limits<double>::max()}, 0.0, juce::NotificationType::dontSendNotification);
    setKeyboardType(juce::TextInputTarget::VirtualKeyboardType::decimalKeyboard);
}

juce::TextEditor* NumberField::Label::createEditorComponent()
{
    auto* editor = juce::Label::createEditorComponent();
    if(editor != nullptr)
    {
        auto const font = getFont();
        editor->setFont(font);
        editor->setIndents(0, static_cast<int>(std::floor(font.getDescent())));
        editor->setJustification(getJustificationType());
        editor->setBorder(getBorderSize());
        editor->setInputFilter(this, false);
        editor->setMultiLine(false);
    }
    return editor;
}

void NumberField::Label::editorShown(juce::TextEditor* editor)
{
    if(editor != nullptr)
    {
        editor->setText(juce::String(getText().getDoubleValue(), mNumEditedDecimals), false);
        editor->moveCaretToTop(false);
        editor->moveCaretToEnd(true);
    }
}

void NumberField::Label::setRange(juce::Range<double> const& range, double interval, juce::NotificationType const notification)
{
    anlWeakAssert(interval >= 0.0);
    interval = std::max(0.0, interval);
    if(range != mRange || std::abs(interval - mInterval) > std::numeric_limits<double>::epsilon())
    {
        mRange = range;
        mInterval = interval;
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
        mNumEditedDecimals = getNumDecimals();
        if(auto* editor = getCurrentTextEditor())
        {
            editor->setText(juce::String(getText().getDoubleValue(), mNumEditedDecimals), notification == juce::NotificationType::dontSendNotification ? false : true);
        }
        else
        {
            setText(getText(), notification);
        }
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

void NumberField::Label::setNumDecimalsDisplayed(int numDecimals)
{
    numDecimals = std::max(numDecimals, -1);
    if(mNumDisplayedDecimals != numDecimals)
    {
        mNumDisplayedDecimals = numDecimals;
        setText(getText(), juce::NotificationType::dontSendNotification);
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
        setText(getText(), juce::NotificationType::dontSendNotification);
    }
}

juce::String NumberField::Label::getTextValueSuffix() const
{
    return mSuffix;
}

void NumberField::Label::textWasChanged()
{
    auto value = getText().getDoubleValue();
    anlWeakAssert(!std::isnan(value) && std::isfinite(value));
    if(mInterval > 0.0)
    {
        value = std::round((value - mRange.getStart()) / mInterval) * mInterval + mRange.getStart();
    }
    value = mRange.clipValue(value);
    auto const numDecimals = mNumDisplayedDecimals >= 0 ? mNumDisplayedDecimals : (mNumEditedDecimals > 0 ? 2 : 0);
    setText(juce::String(value, numDecimals) + mSuffix, juce::NotificationType::dontSendNotification);
}

juce::String NumberField::Label::filterNewText(juce::TextEditor& editor, juce::String const& newInput)
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

void NumberField::Label::loadProperties(juce::NamedValueSet const& properties, juce::NotificationType const notification)
{
    auto const& suffix = properties.getWithDefault("suffix", mSuffix);
    anlWeakAssert(suffix.isString());
    if(suffix.isString())
    {
        setTextValueSuffix(suffix.toString());
    }
    auto const& numDisplayDecimals = properties.getWithDefault("numDisplayDecimals", mNumDisplayedDecimals);
    anlWeakAssert(numDisplayDecimals.isInt());
    if(numDisplayDecimals.isInt())
    {
        setNumDecimalsDisplayed(static_cast<int>(numDisplayDecimals));
    }
    auto const& rangeMin = properties.getWithDefault("rangeMin", mRange.getStart());
    auto const& rangeMax = properties.getWithDefault("rangeMax", mRange.getEnd());
    auto const& interval = properties.getWithDefault("interval", mInterval);
    anlWeakAssert(rangeMin.isDouble() && rangeMax.isDouble() && interval.isDouble());
    if(rangeMin.isDouble() && rangeMax.isDouble() && interval.isDouble())
    {
        setRange({static_cast<double>(rangeMin), static_cast<double>(rangeMax)}, static_cast<double>(interval), notification);
    }
}

void NumberField::Label::storeProperties(juce::NamedValueSet& properties, juce::Range<double> const& range, double interval, int numDecimals, juce::String const& suffix)
{
    properties.set("rangeMin", range.getStart());
    properties.set("rangeMax", range.getEnd());
    properties.set("interval", interval);
    properties.set("numDisplayDecimals", numDecimals);
    properties.set("suffix", suffix);
}

NumberField::NumberField()
{
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
    mLabel.setText(juce::String(value), notification);
}

double NumberField::getValue() const
{
    return mLabel.getText().getDoubleValue();
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

void NumberField::setJustificationType(juce::Justification newJustification)
{
    mLabel.setJustificationType(newJustification);
}

juce::Justification NumberField::getJustificationType() const
{
    return mLabel.getJustificationType();
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
