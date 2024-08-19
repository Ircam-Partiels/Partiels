#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

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
    bool isOptional() const;

    void setRange(juce::Range<double> const& range, double interval, juce::NotificationType const notification);
    juce::Range<double> getRange() const;
    double getInterval() const;
    int getNumEditedDecimals() const;
    void setOptionalSupported(bool state, juce::String const& substitution);
    bool isOptionalSupported() const;

    void setConversionFunctions(std::function<juce::String(double value)> stringFromValue, std::function<double(juce::String const& text)> valueFromString);

    void setTextValueSuffix(juce::String const& suffix);
    juce::String getTextValueSuffix() const;

    void setNumDecimalsDisplayed(int numDecimals);
    int getNumDecimalsDisplayed() const;

    void setJustificationType(juce::Justification newJustification);
    juce::Justification getJustificationType() const;

    void setText(juce::String const& newText, juce::NotificationType notification);

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
        void editorAboutToBeHidden(juce::TextEditor* editor) override;

        void setValue(double value, juce::NotificationType const notification);
        double getValue() const;
        void setRange(juce::Range<double> const& range, double interval, juce::NotificationType const notification);
        juce::Range<double> getRange() const;
        double getInterval() const;
        int getNumEditedDecimals() const;
        void setNumDecimalsDisplayed(int numDecimals);
        int getNumDecimalsDisplayed() const;
        void setTextValueSuffix(juce::String const& suffix);
        juce::String getTextValueSuffix() const;

        void loadProperties(juce::NamedValueSet const& properties, juce::NotificationType const notification);
        static void storeProperties(juce::NamedValueSet& properties, juce::Range<double> const& range, double interval, int numDecimals, juce::String const& suffix);
        void setAcceptsOptional(bool state, juce::String const& substitution);
        bool isOptionalAccepted() const;
        juce::String getOptionalSubstitution() const;

        std::function<juce::String(double value)> stringFromValue = nullptr;
        std::function<double(juce::String const& text)> valueFromString = nullptr;

    private:
        // juce::TextEditor::InputFilter
        juce::String filterNewText(juce::TextEditor& editor, juce::String const& newInput) override;

        double getCorrespondingValue(double value) const;
        int getEffectiveNumDisplayedDecimmals() const;

        double getValueFromText(juce::String const& text) const;
        juce::String getTextFromValue(double value, bool forEditor) const;

        double mValue{0.0};
        juce::Range<double> mRange{0.0, 0.0};
        double mInterval = 0.0;
        int mNumEditedDecimals = 0;
        int mNumDisplayedDecimals = -1;
        juce::String mSuffix;
        bool mAcceptsOptional = false;
        juce::String mSubstitution;
    };

private:
    Label mLabel;
};

MISC_FILE_END
