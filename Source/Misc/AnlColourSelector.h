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
, public juce::DragAndDropTarget
{
public:
    // clang-format off
    enum ColourIds
    {
          borderOffColourId = 0x2000000
        , borderOnColourId
    };
    // clang-format on

    ColourButton(juce::String const& title = "");
    ~ColourButton() override = default;

    void setTitle(juce::String const& title);
    juce::Colour getCurrentColour() const;
    void setCurrentColour(juce::Colour const& newColour, juce::NotificationType notificationType);

    std::function<void(juce::Colour const& colour)> onColourChanged = nullptr;

    // juce::Component
    void mouseDown(juce::MouseEvent const& event) override;
    void mouseDrag(juce::MouseEvent const& event) override;
    void mouseUp(juce::MouseEvent const& event) override;

private:
    // juce::Button
    using juce::Button::clicked;
    void clicked() override;
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    // juce::DragAndDropTarget
    bool isInterestedInDragSource(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
    void itemDragEnter(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
    void itemDragExit(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
    void itemDropped(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;

    juce::String mTitle;
    juce::Colour mColour;
    juce::Component::SafePointer<ColourButton> mDraggedColour;
};

ANALYSE_FILE_END
