#include "AnlTrackSection.h"
#include "AnlTrackExporter.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Section::Section(Director& director, Zoom::Accessor& timeZoomAcsr, Transport::Accessor& transportAcsr)
: mDirector(director)
, mTimeZoomAccessor(timeZoomAcsr)
, mTransportAccessor(transportAcsr)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType type)
    {
        switch(type)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::file:
            case AttrType::key:
            case AttrType::input:
            case AttrType::description:
            case AttrType::results:
            case AttrType::edit:
            case AttrType::state:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::unit:
            case AttrType::channelsLayout:
            case AttrType::showInGroup:
            case AttrType::zoomLink:
            case AttrType::graphics:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::zoomAcsr:
            case AttrType::grid:
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

    addAndMakeVisible(mScrollBar);
    addAndMakeVisible(mRulerDecoration);
    addAndMakeVisible(mThumbnailDecoration);
    addAndMakeVisible(mSnapshotDecoration);
    addAndMakeVisible(mPlotDecoration);
    addAndMakeVisible(mResizerBar);
    setSize(80, 100);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::Section::~Section()
{
    mAccessor.removeListener(mListener);
}

juce::Rectangle<int> Track::Section::getPlotBounds() const
{
    return mPlot.getBounds();
}

juce::String Track::Section::getIdentifier() const
{
    return mAccessor.getAttr<AttrType::identifier>();
}

void Track::Section::setResizable(bool state)
{
    mResizerBar.setEnabled(state);
}

void Track::Section::resized()
{
    mResizerBar.setBounds(getLocalBounds().removeFromBottom(2).reduced(2, 0));

    auto bounds = getLocalBounds();
    bounds.removeFromLeft(24);
    mThumbnailDecoration.setBounds(bounds.removeFromLeft(24));
    mSnapshotDecoration.setBounds(bounds.removeFromLeft(36));
    mScrollBar.setBounds(bounds.removeFromRight(8));
    mRulerDecoration.setBounds(bounds.removeFromRight(16));
    mPlotDecoration.setBounds(bounds);
}

void Track::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
}

void Track::Section::mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel)
{
    mScrollHelper.mouseWheelMove(event, wheel);
    if(!mScrollHelper.getModifierKeys().isCtrlDown())
    {
        Component::mouseWheelMove(event, wheel);
        return;
    }
    if(!mScrollHelper.getModifierKeys().isShiftDown())
    {
        auto const delta = mScrollHelper.getOrientation() == ScrollHelper::Orientation::vertical ? wheel.deltaY : wheel.deltaX;
        mouseMagnify(event, 1.0f + delta);
    }
    else
    {
        auto const delta = mScrollHelper.getModifierKeys().isShiftDown() ? wheel.deltaY : wheel.deltaX;
        switch(Tools::getFrameType(mAccessor))
        {
            case Track::FrameType::label:
                break;
            case Track::FrameType::value:
            {
                auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
                auto const visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
                auto const offset = static_cast<double>(-delta) * visibleRange.getLength();
                zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange - offset, NotificationType::synchronous);
            }
            break;
            case Track::FrameType::vector:
            {
                auto& zoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
                auto const visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
                auto const offset = static_cast<double>(-delta) * visibleRange.getLength();
                zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange - offset, NotificationType::synchronous);
            }
            break;
        }
    }
}

void Track::Section::mouseMagnify(juce::MouseEvent const& event, float magnifyAmount)
{
    if(!event.mods.isCtrlDown())
    {
        Component::mouseMagnify(event, magnifyAmount);
        return;
    }

    switch(Tools::getFrameType(mAccessor))
    {
        case Track::FrameType::label:
            break;
        case Track::FrameType::value:
        {
            auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
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
        break;
        case Track::FrameType::vector:
        {
            auto& zoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
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
        break;
    }
}

ANALYSE_FILE_END
