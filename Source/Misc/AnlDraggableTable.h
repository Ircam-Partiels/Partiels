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
    
    DraggableTable(juce::String const& tooltip = {});
    ~DraggableTable() override = default;
    
    void setComponents(std::vector<ComponentRef> const& component);
    
    std::function<void(size_t, size_t)> onComponentDragged = nullptr;
    
    // juce::Component
    void resized() override;

private:
    
    // juce::ComponentListener
    void componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized) override;
    
    // juce::DragAndDropTarget
    bool isInterestedInDragSource(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
    void itemDragEnter(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
    void itemDragMove(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
    void itemDragExit(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
    void itemDropped(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
    
    std::vector<juce::Component::SafePointer<juce::Component>> mContents;
};

ANALYSE_FILE_END
