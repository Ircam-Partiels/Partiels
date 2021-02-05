#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

//! @brief A number field editor.
//! @details The number field has a text label that restricts the inputs to numbers  (integers or floating point number).
class NumberField
: public juce::Component
, public juce::SettableTooltipClient
, private juce::TextEditor::InputFilter
{
public:
    //! @brief The constructor.
    NumberField();
    ~NumberField() override = default;
    
    void setValue(double value, juce::NotificationType const notification);
    double getValue() const;
    
    void setRange(juce::Range<double> const& range, double newInterval, juce::NotificationType const notification);
    juce::Range<double> getRange() const;
    double getInterval() const;

    void setTextValueSuffix(juce::String const& suffix);
    juce::String getTextValueSuffix() const;
    
    void setNumDecimalsDisplayed(size_t numDecimals);
    size_t getNumDecimalsDisplayed() const;
    
    void setJustificationType(juce::Justification newJustification);
    juce::Justification getJustificationType() const;

    std::function<void(double)> onValueChanged = nullptr;
    
    // juce::Component
    void resized() override;
    
    // juce::SettableTooltipClient
    void setTooltip(juce::String const& newTooltip) override;
    
private:
    
    // juce::TextEditor::InputFilter
    juce::String filterNewText(juce::TextEditor& editor, juce::String const& newInput) override;
    
    juce::Label mLabel;
    juce::String mSuffix;
    juce::Range<double> mRange = {std::numeric_limits<double>::min(), std::numeric_limits<double>::max()};
    double mInterval = 0.0;
    size_t mNumDisplayedDecimals = 0_z;
    size_t mNumEditedDecimals = 0_z;
    double mValue;
};

ANALYSE_FILE_END
