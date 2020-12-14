#include "AnlLayoutStrechableContainerSection.h"

ANALYSE_FILE_BEGIN

Layout::StrechableContainer::Section::Holder::ResizerBar::ResizerBar(Holder& owner)
: mOwner(owner)
{
    setRepaintsOnMouseActivity(true);
}

void Layout::StrechableContainer::Section::Holder::ResizerBar::paint(juce::Graphics& g)
{
    auto const isVertical = mOwner.mOwner.mOrientation == Orientation::vertical;
    setMouseCursor(isVertical ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::LeftRightResizeCursor);
    if(isMouseOverOrDragging())
    {
        g.fillAll(findColour(resizerActiveColourId));
    }
    else
    {
        g.fillAll(findColour(resizerInactiveColourId));
    }
}

void Layout::StrechableContainer::Section::Holder::ResizerBar::mouseDown(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    auto const isVertical = mOwner.mOwner.mOrientation == Orientation::vertical;
    mSavedSize = isVertical ? mOwner.getHeight() : mOwner.getWidth();
}

void Layout::StrechableContainer::Section::Holder::ResizerBar::mouseDrag(juce::MouseEvent const& event)
{
    auto const isVertical = mOwner.mOwner.mOrientation == Orientation::vertical;
    auto const offset = isVertical ? event.getDistanceFromDragStartY() : event.getDistanceFromDragStartX();
    auto const newSize = mSavedSize + offset;
    auto const index = mOwner.mIndex;
    auto& acsr = mOwner.mOwner.mAccessor;
    auto sizes = acsr.getAttr<AttrType::sizes>();
    anlStrongAssert(index < sizes.size());
    if(index < sizes.size())
    {
        sizes[index] = std::max(mOwner.mMinimumSize + 2, newSize);
        acsr.setAttr<AttrType::sizes>(sizes, NotificationType::synchronous);
    }
}
    
Layout::StrechableContainer::Section::Holder::Holder(Section& owner, size_t index)
: mOwner(owner)
, mIndex(index)
, mResizer(*this)
{
    addAndMakeVisible(mResizer);
}

void Layout::StrechableContainer::Section::Holder::resized()
{
    auto bounds = getLocalBounds();
    auto const isVertical = mOwner.mOrientation == Orientation::vertical;
    mResizer.setBounds(isVertical ? bounds.removeFromBottom(2) : bounds.removeFromRight(2));
    if(mContent != nullptr)
    {
        mContent->setBounds(bounds);
    }
}

void Layout::StrechableContainer::Section::Holder::setContent(juce::Component* content, int minimumSize)
{
    if(mContent != content)
    {
        if(mContent != nullptr)
        {
            removeChildComponent(mContent);
        }
        mContent = content;
        mMinimumSize = minimumSize;
        addAndMakeVisible(mContent);
        resized();
    }
}

Layout::StrechableContainer::Section::Section(Accessor& acessor, Orientation orientation)
: mAccessor(acessor)
, mOrientation(orientation)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::sizes:
            {
                auto const sizes = acsr.getAttr<AttrType::sizes>();
                mHolders.reserve(sizes.size());
                for(size_t index = mHolders.size(); index < sizes.size(); ++index)
                {
                    auto holder = std::make_unique<Holder>(*this, index);
                    if(holder != nullptr)
                    {
                        mInnerContainer.addAndMakeVisible(holder.get());
                    }
                    mHolders.push_back(std::move(holder));
                }
                mHolders.resize(sizes.size());
                resized();
            }
                break;
        }
    };
    auto const isVertical = mOrientation == Orientation::vertical;
    mViewport.setScrollBarsShown(isVertical, !isVertical);
    mViewport.setViewedComponent(&mInnerContainer, false);
    addAndMakeVisible(mViewport);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Layout::StrechableContainer::Section::~Section()
{
    mAccessor.removeListener(mListener);
}

Layout::StrechableContainer::Section::Orientation Layout::StrechableContainer::Section::getOrientation() const
{
    return mOrientation;
}

void Layout::StrechableContainer::Section::setContent(size_t index, juce::Component* content, int minimumSize)
{
    mListener.onAttrChanged(mAccessor, AttrType::sizes);
    //anlWeakAssert(index < mHolders.size() && mHolders[index] != nullptr);
    if(index < mHolders.size() && mHolders[index] != nullptr)
    {
        mHolders[index]->setContent(content, minimumSize);
    }
}

void Layout::StrechableContainer::Section::setOrientation(Orientation orientation)
{
    mOrientation = orientation;
    for(auto& holder : mHolders)
    {
        if(holder != nullptr)
        {
            holder->resized();
        }
    }
    auto const isVertical = mOrientation == Orientation::vertical;
    mViewport.setScrollBarsShown(isVertical, !isVertical);
    resized();
}

void Layout::StrechableContainer::Section::resized()
{
    auto const isVertical = mOrientation == Orientation::vertical;
    auto const sizes = mAccessor.getAttr<AttrType::sizes>();
    
    auto const savedViewportPosition = mViewport.getViewPosition();
    mViewport.setBounds(getLocalBounds());
    auto const fullSize = std::accumulate(sizes.cbegin(), sizes.cend(), 0);
    mInnerContainer.setBounds(0, 0, isVertical ? getWidth() : fullSize, isVertical ? fullSize : getHeight());
    auto const isCrollBarShown = isVertical ? mViewport.getVerticalScrollBar().isVisible() : mViewport.getHorizontalScrollBar().isVisible();
    mInnerContainer.setBounds(0, 0, isVertical ? (getWidth() - (isCrollBarShown ? 8 : 0)) : fullSize, isVertical ? fullSize : (getHeight() - (isCrollBarShown ? 8 : 0)));
    
    auto bounds = mInnerContainer.getLocalBounds();
    anlStrongAssert(sizes.size() == mHolders.size());
    for(size_t index = 0; index < mHolders.size() && index < sizes.size(); ++index)
    {
        anlStrongAssert(mHolders[index] != nullptr);
        if(mHolders[index] != nullptr)
        {
            auto const size = sizes[index];
            mHolders[index]->setBounds(isVertical ? bounds.removeFromTop(size) : bounds.removeFromLeft(size));
        }
    }
    mViewport.setViewPosition(savedViewportPosition);
}

ANALYSE_FILE_END
