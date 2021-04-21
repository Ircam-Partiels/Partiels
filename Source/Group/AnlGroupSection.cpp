#include "AnlGroupSection.h"
#include "../Track/AnlTrackSection.h"
#include "../Zoom/AnlZoomTools.h"
#include "AnlGroupStrechableSection.h"

ANALYSE_FILE_BEGIN

Group::Section::Section(Accessor& accessor, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr)
: mAccessor(accessor)
, mTransportAccessor(transportAcsr)
, mTimeZoomAccessor(timeZoomAcsr)
{
    mRuler.onDoubleClick = [&]()
    {
        auto& zoomAcsr = mAccessor.getAcsr<AcsrType::zoom>();
        auto const& range = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
    };

    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType type)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::colour:
            case AttrType::expanded:
            case AttrType::layout:
            case AttrType::tracks:
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
        }
    };

    mResizerBar.onMoved = [&](int size)
    {
        mAccessor.setAttr<AttrType::height>(size, NotificationType::synchronous);
    };

    addAndMakeVisible(mRuler);
    addAndMakeVisible(mScrollBar);
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

void Group::Section::resized()
{
    mResizerBar.setBounds(getLocalBounds().removeFromBottom(2).reduced(2, 0));

    auto bounds = getLocalBounds();
    mThumbnailDecoration.setBounds(bounds.removeFromLeft(48));
    mSnapshotDecoration.setBounds(bounds.removeFromLeft(48));

    mScrollBar.setBounds(bounds.removeFromRight(8));
    mRuler.setBounds(bounds.removeFromRight(16));
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

void Group::Section::colourChanged()
{
    setOpaque(findColour(ColourIds::backgroundColourId).isOpaque());
}

bool Group::Section::isInterestedInDragSource(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    auto* source = dragSourceDetails.sourceComponent.get();
    auto* obj = dragSourceDetails.description.getDynamicObject();
    auto* parent = findParentComponentOfClass<StrechableSection>();
    if(source == nullptr || obj == nullptr || dynamic_cast<Track::Section*>(source) == nullptr || source->findParentComponentOfClass<StrechableSection>() == parent)
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
    if(obj == nullptr || source == nullptr)
    {
        mIsItemDragged = false;
        repaint();
        return;
    }
    source->setAlpha(0.4f);
    mIsItemDragged = true;
    repaint();
}

void Group::Section::itemDragMove(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    juce::ignoreUnused(dragSourceDetails);
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
    if(onTrackInserted != nullptr)
    {
        onTrackInserted(obj->getProperty("identifier"));
    }
}

void Group::Section::mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel)
{
    if(!event.mods.isCommandDown())
    {
        Component::mouseWheelMove(event, wheel);
        return;
    }

    auto& zoomAcsr = mAccessor.getAcsr<AcsrType::zoom>();
    auto const visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const offset = static_cast<double>(-wheel.deltaY) * visibleRange.getLength();
    zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange - offset, NotificationType::synchronous);
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
                                            mAccessor.setAttr<AttrType::focused>(hasKeyboardFocus(true), NotificationType::synchronous);
                                        }
                                    });
}

ANALYSE_FILE_END
