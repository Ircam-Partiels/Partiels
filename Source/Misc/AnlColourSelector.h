#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class ColourSelector
: public juce::ColourSelector
, private juce::ChangeListener
{
public:
    ColourSelector();
    ~ColourSelector() override;
    
    std::function<void(juce::Colour const& colour)> onColourChanged = nullptr;
    
private:
    // juce::ChangeListener
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
};

class ColourButton
: public juce::Button
{
public:
    
    enum ColourIds
    {
          borderOffColourId = 0x2000000
        , borderOnColourId
    };
    
    ColourButton(juce::String const& title = "");
    ~ColourButton() override = default;
    
    void setTitle(juce::String const& title);
    juce::Colour getCurrentColour() const;
    void setCurrentColour(juce::Colour const& newColour, juce::NotificationType notificationType);
    
    std::function<void(juce::Colour const& colour)> onColourChanged = nullptr;
    
private:
    // juce::Button
    void clicked() override;
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
    juce::String mTitle;
    juce::Colour mColour;
};

ANALYSE_FILE_END
