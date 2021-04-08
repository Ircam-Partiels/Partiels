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
: public juce::ShapeButton
{
public:
    ColourButton(juce::Colour const& colour = juce::Colours::white, juce::String const& title = "");
    ~ColourButton() override = default;
    
    void setTitle(juce::String const& title);
    juce::Colour getCurrentColour() const;
    void setCurrentColour(juce::Colour const& newColour, juce::NotificationType notificationType);
    
    std::function<void(juce::Colour const& colour)> onColourChanged = nullptr;
    
    // juce::Component
    void resized() override;
    
private:
    // juce::ShapeButton
    void clicked() override;
    
    juce::String mTitle;
    juce::Colour mColour;
};

ANALYSE_FILE_END
