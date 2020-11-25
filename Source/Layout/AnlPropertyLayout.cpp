#include "AnlPropertyLayout.h"

ANALYSE_FILE_BEGIN

void Layout::PropertyLayout::Separator::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::separatorColourId, true));
}

Layout::PropertyLayout::PropertyLayout()
{
    mViewport.setScrollBarThickness(6);
    mViewport.setViewedComponent(&mContent, false);
    addAndMakeVisible(mViewport);
}

void Layout::PropertyLayout::resized()
{
    auto constexpr separatorSize = 2;
    auto constexpr maxSize = std::numeric_limits<int>::max();
    auto const isLeft = mPositioning == Positioning::left;
    auto bounds = isLeft ? getLocalBounds().withHeight(maxSize) : getLocalBounds().withWidth(maxSize);
    for(auto& container : mContainers)
    {
        anlStrongAssert(container != nullptr);
        if(container != nullptr)
        {
            auto const panel = std::get<0>(*container.get());
            auto const containerSize = isLeft ? panel.get().getHeight() : panel.get().getWidth();
            panel.get().setBounds(isLeft ? bounds.removeFromTop(containerSize) : bounds.removeFromLeft(containerSize));
            std::get<1>(*container.get()).setBounds(isLeft ? bounds.removeFromTop(separatorSize) : bounds.removeFromLeft(separatorSize));
        }
    }
    mContent.setSize(isLeft ? getWidth() : bounds.getX() - separatorSize, !isLeft ? getHeight() : bounds.getY() - separatorSize);
    mViewport.setBounds(getLocalBounds());
}

void Layout::PropertyLayout::setPanels(std::vector<PanelRef> const& panels, Positioning positioning)
{
    mContent.removeAllChildren();
    mPositioning = positioning;
    
    mContainers.clear();
    mContainers.reserve(panels.size());
    for(auto panel : panels)
    {
        auto container = std::make_unique<Container>(panel);
        anlStrongAssert(container != nullptr);
        if(container != nullptr)
        {
            anlWeakAssert(std::get<0>(*container.get()).get().positioning == mPositioning);
            std::get<0>(*container.get()).get().positioning = mPositioning;
            mContent.addAndMakeVisible(std::get<0>(*container.get()));
            mContent.addAndMakeVisible(std::get<1>(*container.get()));
            mContainers.push_back(std::move(container));
        }
    }
    auto const isLeft = mPositioning == Positioning::left;
    mViewport.setScrollBarsShown(isLeft, !isLeft, isLeft, !isLeft);
    resized();
}

ANALYSE_FILE_END
