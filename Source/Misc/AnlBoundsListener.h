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

private:
    // juce::ComponentListener
    void componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized) override;

    std::set<juce::Component::SafePointer<juce::Component>> mContents;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoundsListener)
};

ANALYSE_FILE_END
