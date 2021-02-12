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

ANALYSE_FILE_END
