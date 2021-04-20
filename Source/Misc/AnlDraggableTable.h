#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class DraggableTable
: public juce::Component
, public juce::SettableTooltipClient
, public juce::DragAndDropTarget
, private juce::ComponentListener
{
public:
    using ComponentRef = std::reference_wrapper<juce::Component>;

    DraggableTable(juce::String const type, juce::String const& tooltip = {});
    ~DraggableTable() override = default;

    void setComponents(std::vector<ComponentRef> const& component);
    std::vector<juce::Component::SafePointer<juce::Component>> getComponents();

    std::function<void(juce::String const& identifier, size_t index)> onComponentDropped = nullptr;

    // juce::Component
    void resized() override;

    static juce::var createDescription(juce::MouseEvent const& event, juce::String const& type, juce::String const& identifier, int height, std::function<void(void)> onEnter = nullptr, std::function<void(void)> onExit = nullptr);

private:
    struct Description
    : public juce::DynamicObject
    {
        std::function<void(void)> onEnter = nullptr;
        std::function<void(void)> onExit = nullptr;
    };

    // juce::ComponentListener
    void componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized) override;

    // juce::DragAndDropTarget
    bool isInterestedInDragSource(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
    void itemDragEnter(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
    void itemDragMove(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
    void itemDragExit(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
    void itemDropped(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;

    std::vector<juce::Component::SafePointer<juce::Component>> mContents;

    juce::String const mType;
    bool mIsDragging{false};
};

ANALYSE_FILE_END
