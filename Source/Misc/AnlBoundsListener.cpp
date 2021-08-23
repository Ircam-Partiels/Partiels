#include "AnlBoundsListener.h"

ANALYSE_FILE_BEGIN

BoundsListener::~BoundsListener()
{
    for(auto& content : mContents)
    {
        auto* component = content.getComponent();
        if(component != nullptr)
        {
            component->removeComponentListener(this);
        }
    }
}

void BoundsListener::attachTo(juce::Component& component)
{
    if(mContents.insert(juce::Component::SafePointer<juce::Component>(&component)).second)
    {
        component.addComponentListener(this);
    }
}

void BoundsListener::detachFrom(juce::Component& component)
{
    if(mContents.erase(&component) > 0)
    {
        component.removeComponentListener(this);
    }
}

void BoundsListener::componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized)
{
    if(wasMoved && onComponentMoved != nullptr)
    {
        onComponentMoved(component);
    }
    if(wasResized && onComponentResized != nullptr)
    {
        onComponentResized(component);
    }
}

void BoundsListener::componentBroughtToFront(juce::Component& component)
{
    if(onComponentBroughtToFront != nullptr)
    {
        onComponentBroughtToFront(component);
    }
}

void BoundsListener::componentVisibilityChanged(juce::Component& component)
{
    if(onComponentVisibilityChanged != nullptr)
    {
        onComponentVisibilityChanged(component);
    }
}

void BoundsListener::componentChildrenChanged(juce::Component& component)
{
    if(onComponentChildrenChanged != nullptr)
    {
        onComponentChildrenChanged(component);
    }
}

void BoundsListener::componentParentHierarchyChanged(juce::Component& component)
{
    if(onComponentParentHierarchyChanged != nullptr)
    {
        onComponentParentHierarchyChanged(component);
    }
}

void BoundsListener::componentNameChanged(juce::Component& component)
{
    if(onComponentNameChanged != nullptr)
    {
        onComponentNameChanged(component);
    }
}

void BoundsListener::componentBeingDeleted(juce::Component& component)
{
    if(onComponentBeingDeleted != nullptr)
    {
        onComponentBeingDeleted(component);
    }
}

void BoundsListener::componentEnablementChanged(juce::Component& component)
{
    if(onComponentEnablementChanged != nullptr)
    {
        onComponentEnablementChanged(component);
    }
}

ANALYSE_FILE_END
