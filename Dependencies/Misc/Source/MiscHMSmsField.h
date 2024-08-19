#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

class HMSmsField
: public juce::Component
, public juce::SettableTooltipClient
, private juce::AsyncUpdater
{
public:
    HMSmsField();
    ~HMSmsField() override = default;

    void setRange(juce::Range<double> const& range, juce::NotificationType const notification);

    void setTime(double timeInSeconds, juce::NotificationType const notification);
    double getTime() const;

    void setFont(juce::Font const& newFont);
    void setEditable(bool editOnSingleClick, bool editOnDoubleClick, bool lossOfFocusDiscards);
    void setJustificationType(juce::Justification newJustification);
    void setHideDisabledFields(bool shouldHide);

    std::function<void(double)> onTimeChanged = nullptr;

    // juce::Component
    void resized() override;
    void enablementChanged() override;

    // juce::SettableTooltipClient
    void setTooltip(juce::String const& newTooltip) override;

private:
    // juce::AsyncUpdater
    void handleAsyncUpdate() override;

    struct Field
    : public juce::TextEditor::InputFilter
    {
        juce::Label entry;
        juce::Label suffix;
        int maxValue = 0;
        bool canBeNegative = false;
        bool canBePositive = true;

        void setValue(int value);
        int getValue() const;
        juce::String filterNewText(juce::TextEditor& editor, juce::String const& newInput) override;
    };

    static juce::String toText(int value);

    juce::Range<double> mRange;
    std::array<Field, 4> mFields;
    std::array<int, 4> mSavedValues;
    std::array<int, 4> mNewValues;
    bool mIsEdited = false;
    bool mLossOfFocusDiscards = true;
    bool mHideDisabledFields = false;
    double mSavedTime;
};

MISC_FILE_END
