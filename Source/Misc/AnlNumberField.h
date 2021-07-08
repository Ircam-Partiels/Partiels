#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

//! @brief A number field editor.
//! @details The number field has a text label that restricts the inputs to numbers  (integers or floating point number).
class NumberField
: public juce::Component
, public juce::SettableTooltipClient
{
public:
    //! @brief The constructor.
    NumberField();
    ~NumberField() override = default;

    void setValue(double value, juce::NotificationType const notification);
    double getValue() const;

    void setRange(juce::Range<double> const& range, double interval, juce::NotificationType const notification);
    juce::Range<double> getRange() const;
    double getInterval() const;

    void setTextValueSuffix(juce::String const& suffix);
    juce::String getTextValueSuffix() const;

    void setNumDecimalsDisplayed(int numDecimals);
    int getNumDecimalsDisplayed() const;

    void setJustificationType(juce::Justification newJustification);
    juce::Justification getJustificationType() const;

    std::function<void(double)> onValueChanged = nullptr;

    // Forward to internal juce::Label
    bool isBeingEdited() const;
    void setEditable(bool editOnSingleClick, bool editOnDoubleClick, bool lossOfFocusDiscards);
    juce::Font getFont() const noexcept;
    void setFont(juce::Font const& newFont);
    std::function<void()> onEditorShow = nullptr;
    std::function<void()> onEditorHide = nullptr;

    // juce::Component
    void resized() override;

    // juce::SettableTooltipClient
    void setTooltip(juce::String const& newTooltip) override;

    class Label
    : public juce::Label
    , private juce::TextEditor::InputFilter
    {
    public:
        Label();
        ~Label() override = default;

        // juce::Label
        juce::TextEditor* createEditorComponent() override;
        void textWasChanged() override;
        void editorShown(juce::TextEditor* editor) override;

        void setRange(juce::Range<double> const& range, double interval, juce::NotificationType const notification);
        juce::Range<double> getRange() const;
        double getInterval() const;
        void setNumDecimalsDisplayed(int numDecimals);
        int getNumDecimalsDisplayed() const;
        void setTextValueSuffix(juce::String const& suffix);
        juce::String getTextValueSuffix() const;

    private:
        // juce::TextEditor::InputFilter
        juce::String filterNewText(juce::TextEditor& editor, juce::String const& newInput) override;

        juce::Range<double> mRange{0.0, 0.0};
        double mInterval = 0.0;
        int mNumEditedDecimals = 0;
        int mNumDisplayedDecimals = -1;
        juce::String mSuffix;
    };

private:
    Label mLabel;
};

ANALYSE_FILE_END
