#include "AnlGroupSection.h"
#include "../Track/AnlTrackSection.h"
#include "../Zoom/AnlZoomTools.h"
#include "AnlGroupStrechableSection.h"

ANALYSE_FILE_BEGIN

Group::Section::Section(Director& director, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr)
: mDirector(director)
, mTransportAccessor(transportAcsr)
, mTimeZoomAccessor(timeZoomAcsr)
, mLayoutNotifier(mAccessor, [this]()
                  {
                      updateContent();
                  })
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType type)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::colour:
            case AttrType::expanded:
            case AttrType::tracks:
            case AttrType::layout:
                break;
            case AttrType::height:
            {
                auto const size = mAccessor.getAttr<AttrType::height>();
                setSize(getWidth(), size + 1);
            }
            break;
            case AttrType::focused:
            {
                auto const focused = mAccessor.getAttr<AttrType::focused>();
                mThumbnailDecoration.setHighlighted(focused);
                mSnapshotDecoration.setHighlighted(focused);
                mPlotDecoration.setHighlighted(focused);
            }
            break;
        }
    };

    mResizerBar.onMoved = [&](int size)
    {
        mAccessor.setAttr<AttrType::height>(size, NotificationType::synchronous);
        if(auto* viewport = findParentComponentOfClass<juce::Viewport>())
        {
            if(viewport->canScrollVertically())
            {
                auto const point = viewport->getLocalPoint(this, juce::Point<int>{0, size + 1});
                viewport->autoScroll(point.x, point.y, 20, 10);
            }
        }
    };

    addAndMakeVisible(mThumbnailDecoration);
    addAndMakeVisible(mSnapshotDecoration);
    addAndMakeVisible(mPlotDecoration);
    addAndMakeVisible(mResizerBar);
    setSize(80, 100);
    setWantsKeyboardFocus(true);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::Section::~Section()
{
    mAccessor.removeListener(mListener);
}

juce::Rectangle<int> Group::Section::getPlotBounds() const
{
    return mPlot.getBounds();
}

void Group::Section::resized()
{
    mResizerBar.setBounds(getLocalBounds().removeFromBottom(2).reduced(2, 0));

    auto bounds = getLocalBounds();
    mThumbnailDecoration.setBounds(bounds.removeFromLeft(48));
    mSnapshotDecoration.setBounds(bounds.removeFromLeft(36));

    auto const scrollBarBounds = bounds.removeFromRight(8);
    if(mScrollBar != nullptr)
    {
        mScrollBar->setBounds(scrollBarBounds);
    }
    auto const rulerBounds = bounds.removeFromRight(16);
    if(mDecoratorRuler != nullptr)
    {
        mDecoratorRuler->setBounds(rulerBounds);
    }
    mPlotDecoration.setBounds(bounds);
}

void Group::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
}

void Group::Section::paintOverChildren(juce::Graphics& g)
{
    if(mIsItemDragged)
    {
        g.fillAll(findColour(ColourIds::highlightedColourId));
    }
}

bool Group::Section::isInterestedInDragSource(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    auto* source = dragSourceDetails.sourceComponent.get();
    auto* obj = dragSourceDetails.description.getDynamicObject();
    if(source == nullptr || obj == nullptr || dynamic_cast<Track::Section*>(source) == nullptr)
    {
        return false;
    }
    return obj->getProperty("type").toString() == "Track";
}

void Group::Section::itemDragEnter(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    auto* source = dragSourceDetails.sourceComponent.get();
    auto* obj = dragSourceDetails.description.getDynamicObject();
    anlWeakAssert(obj != nullptr && source != nullptr);
    auto* parent = findParentComponentOfClass<StrechableSection>();
    if(obj == nullptr || source == nullptr || source->findParentComponentOfClass<StrechableSection>() == parent)
    {
        mIsItemDragged = false;
        repaint();
        return;
    }
    auto const isCopy = juce::Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isCtrlDown();
    source->setAlpha(isCopy ? 1.0f : 0.4f);
    mIsItemDragged = true;
    repaint();
}

void Group::Section::itemDragMove(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    if(auto* viewport = findParentComponentOfClass<juce::Viewport>())
    {
        if(viewport->canScrollVertically())
        {
            auto const point = viewport->getLocalPoint(this, dragSourceDetails.localPosition);
            viewport->autoScroll(point.x, point.y, 20, 10);
        }
    }
}

void Group::Section::itemDragExit(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    auto* source = dragSourceDetails.sourceComponent.get();
    anlWeakAssert(source != nullptr);
    if(source != nullptr)
    {
        source->setAlpha(1.0f);
    }
    mIsItemDragged = false;
    repaint();
}

void Group::Section::itemDropped(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    mIsItemDragged = false;
    repaint();

    auto* source = dragSourceDetails.sourceComponent.get();
    auto* obj = dragSourceDetails.description.getDynamicObject();
    anlWeakAssert(obj != nullptr && source != nullptr);
    if(obj == nullptr || source == nullptr)
    {
        return;
    }

    source->setAlpha(1.0f);
    auto const isCopy = juce::Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isCtrlDown();
    if(onTrackInserted != nullptr)
    {
        onTrackInserted(obj->getProperty("identifier"), isCopy);
    }
}

void Group::Section::mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel)
{
    if(!event.mods.isCommandDown())
    {
        Component::mouseWheelMove(event, wheel);
        return;
    }
    if(mScrollHelper.mouseWheelMove(wheel) == ScrollHelper::Orientation::horizontal)
    {
        mouseMagnify(event, 1.0f + wheel.deltaX);
    }
    else
    {
        auto& zoomAcsr = mAccessor.getAcsr<AcsrType::zoom>();
        auto const visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
        auto const offset = static_cast<double>(-wheel.deltaY) * visibleRange.getLength();
        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange - offset, NotificationType::synchronous);
    }
}

void Group::Section::mouseMagnify(juce::MouseEvent const& event, float magnifyAmount)
{
    if(!event.mods.isCommandDown())
    {
        Component::mouseMagnify(event, magnifyAmount);
        return;
    }

    auto& zoomAcsr = mAccessor.getAcsr<AcsrType::zoom>();
    auto const globalRange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
    auto const amount = static_cast<double>(1.0f - magnifyAmount) / 5.0 * globalRange.getLength();
    auto const visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();

    auto const anchor = Zoom::Tools::getScaledValueFromHeight(zoomAcsr, *this, event.y);
    auto const amountLeft = (anchor - visibleRange.getStart()) / visibleRange.getEnd() * amount;
    auto const amountRight = (visibleRange.getEnd() - anchor) / visibleRange.getEnd() * amount;

    auto const minDistance = zoomAcsr.getAttr<Zoom::AttrType::minimumLength>() / 2.0;
    auto const start = std::min(anchor - minDistance, visibleRange.getStart() - amountLeft);
    auto const end = std::max(anchor + minDistance, visibleRange.getEnd() + amountRight);

    zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(Zoom::Range{start, end}, NotificationType::synchronous);
}

void Group::Section::focusOfChildComponentChanged(juce::Component::FocusChangeType cause)
{
    juce::ignoreUnused(cause);
    juce::WeakReference<juce::Component> target(this);
    juce::MessageManager::callAsync([=, this]
                                    {
                                        if(target.get() != nullptr)
                                        {
                                            auto const hasFocus = hasKeyboardFocus(true) || getCurrentlyFocusedComponent() == nullptr;
                                            mAccessor.setAttr<AttrType::focused>(hasFocus, NotificationType::synchronous);
                                        }
                                    });
}

void Group::Section::updateContent()
{
    auto const layout = mAccessor.getAttr<AttrType::layout>();
    if(layout.empty())
    {
        mScrollBar.reset();
        mDecoratorRuler.reset();
        mRuler.reset();
        mGridIdentier.clear();
    }
    else
    {
        auto trackAcrs = Tools::getTrackAcsr(mAccessor, layout.front());
        if(!trackAcrs.has_value())
        {
            mScrollBar.reset();
            mDecoratorRuler.reset();
            mRuler.reset();
            mGridIdentier.clear();
        }
        else if(mGridIdentier != layout.front())
        {
            mRuler = std::make_unique<Track::Ruler>(*trackAcrs);
            if(mRuler != nullptr)
            {
                mDecoratorRuler = std::make_unique<Decorator>(*mRuler.get());
                addAndMakeVisible(mDecoratorRuler.get());
            }
            mScrollBar = std::make_unique<Track::ScrollBar>(*trackAcrs);
            if(mScrollBar != nullptr)
            {
                addAndMakeVisible(mScrollBar.get());
            }
            mGridIdentier = layout.front();
        }
    }

    if(mRuler == nullptr)
    {
        mRuler = std::make_unique<juce::Component>();
        if(mRuler != nullptr)
        {
            mDecoratorRuler = std::make_unique<Decorator>(*mRuler.get());
            addAndMakeVisible(mDecoratorRuler.get());
        }
    }
    resized();
}

ANALYSE_FILE_END
