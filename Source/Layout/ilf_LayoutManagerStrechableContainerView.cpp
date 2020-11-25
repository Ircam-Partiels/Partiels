
#include "ilf_LayoutManagerStrechableContainerView.h"
#include <numeric>

ILF_WARNING_BEGIN
ILF_NAMESPACE_BEGIN

LayoutManager::Strechable::Container::View::Holder::ResizerBar::ResizerBar(Holder& owner)
: mOwner(owner)
{
    setRepaintsOnMouseActivity(true);
}

void LayoutManager::Strechable::Container::View::Holder::ResizerBar::paint(juce::Graphics& g)
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

void LayoutManager::Strechable::Container::View::Holder::ResizerBar::mouseDown(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    auto const isVertical = mOwner.mOwner.mOrientation == Orientation::vertical;
    mSavedSize = isVertical ? mOwner.getHeight() : mOwner.getWidth();
}

void LayoutManager::Strechable::Container::View::Holder::ResizerBar::mouseDrag(juce::MouseEvent const& event)
{
    auto const isVertical = mOwner.mOwner.mOrientation == Orientation::vertical;
    auto const offset = isVertical ? event.getDistanceFromDragStartY() : event.getDistanceFromDragStartX();
    auto const newSize = mSavedSize + offset;
    auto const index = mOwner.mIndex;
    auto& ctrl = mOwner.mOwner.mController;
    auto sizes = ctrl.getSizes();
    ilfStrongAssert(index < sizes.size());
    if(index < sizes.size())
    {
        sizes[index] = std::max(mOwner.mMinimumSize + 2, newSize);
        ctrl.setSizes(sizes, juce::NotificationType::sendNotification);
    }
}
    
LayoutManager::Strechable::Container::View::Holder::Holder(View& owner, size_t index)
: mOwner(owner)
, mIndex(index)
, mResizer(*this)
{
    addAndMakeVisible(mResizer);
}

void LayoutManager::Strechable::Container::View::Holder::resized()
{
    auto bounds = getLocalBounds();
    auto const isVertical = mOwner.mOrientation == Orientation::vertical;
    mResizer.setBounds(isVertical ? bounds.removeFromBottom(2) : bounds.removeFromRight(2));
    if(mContent != nullptr)
    {
        mContent->setBounds(bounds);
    }
}

void LayoutManager::Strechable::Container::View::Holder::setContent(juce::Component* content, int minimumSize)
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

LayoutManager::Strechable::Container::View::View(Controller& controller, Orientation orientation)
: mController(controller)
, mOrientation(orientation)
{
    auto const isVertical = mOrientation == Orientation::vertical;
    mViewport.setScrollBarsShown(isVertical, !isVertical);
    mViewport.setViewedComponent(&mInnerContainer, false);
    addAndMakeVisible(mViewport);
    mController.addListener(this, juce::NotificationType::sendNotification);
}

LayoutManager::Strechable::Container::View::~View()
{
    mController.removeListener(this);
}

LayoutManager::Strechable::Container::View::Orientation LayoutManager::Strechable::Container::View::getOrientation() const
{
    return mOrientation;
}

void LayoutManager::Strechable::Container::View::setContent(size_t index, juce::Component* content, int minimumSize)
{
    using AttributeType = Controller::Listener::AttributeType;
    auto const sizes = mController.getSizes();
    layoutManagerStrechableContainerAttributeChanged(&mController, AttributeType::sizes, {sizes.data(), sizes.size()});
    ilfStrongAssert(index < mHolders.size() && mHolders[index] != nullptr);
    if(index < mHolders.size() && mHolders[index] != nullptr)
    {
        mHolders[index]->setContent(content, minimumSize);
    }
}

void LayoutManager::Strechable::Container::View::setOrientation(Orientation orientation)
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

void LayoutManager::Strechable::Container::View::resized()
{
    auto const isVertical = mOrientation == Orientation::vertical;
    auto const sizes = mController.getSizes();
    
    auto const savedViewportPosition = mViewport.getViewPosition();
    mViewport.setBounds(getLocalBounds());
    auto const fullSize = std::accumulate(sizes.cbegin(), sizes.cend(), 0);
    mInnerContainer.setBounds(0, 0, isVertical ? getWidth() : fullSize, isVertical ? fullSize : getHeight());
    
    auto bounds = mInnerContainer.getLocalBounds();
    ilfStrongAssert(sizes.size() == mHolders.size());
    for(size_t index = 0; index < mHolders.size() && index < sizes.size(); ++index)
    {
        ilfStrongAssert(mHolders[index] != nullptr);
        if(mHolders[index] != nullptr)
        {
            auto const size = sizes[index];
            mHolders[index]->setBounds(isVertical ? bounds.removeFromTop(size) : bounds.removeFromLeft(size));
        }
    }
    mViewport.setViewPosition(savedViewportPosition);
}

void LayoutManager::Strechable::Container::View::layoutManagerStrechableContainerAttributeChanged(Controller* controller, Controller::Listener::AttributeType type, juce::var const& value)
{
    juce::ignoreUnused(controller, value);
    using AttributeType = Controller::Listener::AttributeType;
    switch(type)
    {
        case AttributeType::sizes:
        {
            auto const sizes = mController.getSizes();
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
}

ILF_NAMESPACE_END
ILF_WARNING_END
