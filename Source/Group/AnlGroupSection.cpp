#include "AnlGroupSection.h"
#include "../Track/AnlTrackSection.h"
#include "AnlGroupStrechableSection.h"

ANALYSE_FILE_BEGIN

Group::Section::Section(Director& director, juce::ApplicationCommandManager& commandManager, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr)
: mDirector(director)
, mTransportAccessor(transportAcsr)
, mTimeZoomAccessor(timeZoomAcsr)
, mPlotOverlay(mPlot, commandManager)
, mLayoutNotifier(mAccessor, [this]()
                  {
                      updateContent();
                  },
                  {Track::AttrType::identifier, Track::AttrType::showInGroup})
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType type)
    {
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
                setSize(getWidth(), acsr.getAttr<AttrType::height>() + 1);
            }
            break;
            case AttrType::focused:
            {
                mThumbnailDecoration.setHighlighted(Tools::isSelected(acsr));
            }
            break;
            case AttrType::zoomid:
            {
                updateContent();
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
    commandManager.registerAllCommandsForTarget(std::addressof(mPlotOverlay));
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

juce::String Group::Section::getIdentifier() const
{
    return mAccessor.getAttr<AttrType::identifier>();
}

void Group::Section::setResizable(bool state)
{
    mResizerBar.setEnabled(state);
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
        g.fillAll(findColour(ColourIds::overflyColourId));
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

void Group::Section::updateContent()
{
    auto const layout = mAccessor.getAttr<AttrType::layout>();
    if(layout.empty())
    {
        mScrollBar.reset();
        mDecoratorRuler.reset();
        mRuler.reset();
        mGridIdentifier.clear();
    }
    else
    {
        auto const zoomTrackAcsr = Tools::getZoomTrackAcsr(mAccessor);
        if(!zoomTrackAcsr.has_value())
        {
            mScrollBar.reset();
            mDecoratorRuler.reset();
            mRuler.reset();
            mGridIdentifier.clear();
        }
        else if(mGridIdentifier != zoomTrackAcsr.value().get().getAttr<Track::AttrType::identifier>())
        {
            mRuler = std::make_unique<Track::Ruler>(zoomTrackAcsr.value());
            if(mRuler != nullptr)
            {
                mDecoratorRuler = std::make_unique<Decorator>(*mRuler.get());
                addAndMakeVisible(mDecoratorRuler.get());
            }
            mScrollBar = std::make_unique<Track::ScrollBar>(zoomTrackAcsr.value());
            if(mScrollBar != nullptr)
            {
                addAndMakeVisible(mScrollBar.get());
            }
            mGridIdentifier = zoomTrackAcsr.value().get().getAttr<Track::AttrType::identifier>();
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
