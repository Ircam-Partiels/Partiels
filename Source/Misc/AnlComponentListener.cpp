#include "AnlComponentListener.h"

ANALYSE_FILE_BEGIN

ComponentListener::~ComponentListener()
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

void ComponentListener::attachTo(juce::Component& component)
{
    if(mContents.insert(juce::Component::SafePointer<juce::Component>(&component)).second)
    {
        component.addComponentListener(this);
    }
}

void ComponentListener::detachFrom(juce::Component& component)
{
    if(mContents.erase(&component) > 0)
    {
        component.removeComponentListener(this);
    }
}

void ComponentListener::componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized)
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

void ComponentListener::componentBroughtToFront(juce::Component& component)
{
    if(onComponentBroughtToFront != nullptr)
    {
        onComponentBroughtToFront(component);
    }
}

void ComponentListener::componentVisibilityChanged(juce::Component& component)
{
    if(onComponentVisibilityChanged != nullptr)
    {
        onComponentVisibilityChanged(component);
    }
}

void ComponentListener::componentChildrenChanged(juce::Component& component)
{
    if(onComponentChildrenChanged != nullptr)
    {
        onComponentChildrenChanged(component);
    }
}

void ComponentListener::componentParentHierarchyChanged(juce::Component& component)
{
    if(onComponentParentHierarchyChanged != nullptr)
    {
        onComponentParentHierarchyChanged(component);
    }
}

void ComponentListener::componentNameChanged(juce::Component& component)
{
    if(onComponentNameChanged != nullptr)
    {
        onComponentNameChanged(component);
    }
}

void ComponentListener::componentBeingDeleted(juce::Component& component)
{
    if(onComponentBeingDeleted != nullptr)
    {
        onComponentBeingDeleted(component);
    }
}

void ComponentListener::componentEnablementChanged(juce::Component& component)
{
    if(onComponentEnablementChanged != nullptr)
    {
        onComponentEnablementChanged(component);
    }
}

ANALYSE_FILE_END
