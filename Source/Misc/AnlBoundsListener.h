#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class BoundsListener
: private juce::ComponentListener
{
public:
    BoundsListener() = default;
    ~BoundsListener() override;

    void attachTo(juce::Component& component);
    void detachFrom(juce::Component& component);

    std::function<void(juce::Component&)> onComponentMoved = nullptr;
    std::function<void(juce::Component&)> onComponentResized = nullptr;
    std::function<void(juce::Component&)> onComponentBroughtToFront = nullptr;
    std::function<void(juce::Component&)> onComponentVisibilityChanged = nullptr;
    std::function<void(juce::Component&)> onComponentChildrenChanged = nullptr;
    std::function<void(juce::Component&)> onComponentParentHierarchyChanged = nullptr;
    std::function<void(juce::Component&)> onComponentNameChanged = nullptr;
    std::function<void(juce::Component&)> onComponentBeingDeleted = nullptr;
    std::function<void(juce::Component&)> onComponentEnablementChanged = nullptr;

private:
    // juce::ComponentListener
    void componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized) override;
    void componentBroughtToFront(juce::Component& component) override;
    void componentVisibilityChanged(juce::Component& component) override;
    void componentChildrenChanged(juce::Component& component) override;
    void componentParentHierarchyChanged(juce::Component& component) override;
    void componentNameChanged(juce::Component& component) override;
    void componentBeingDeleted(juce::Component& component) override;
    void componentEnablementChanged(juce::Component& component) override;

    std::set<juce::Component::SafePointer<juce::Component>> mContents;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoundsListener)
};

ANALYSE_FILE_END
