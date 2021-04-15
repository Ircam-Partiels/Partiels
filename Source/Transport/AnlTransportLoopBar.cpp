#include "AnlTransportLoopBar.h"
#include "../Zoom/AnlZoomTools.h"

ANALYSE_FILE_BEGIN

Transport::LoopBar::LoopBar(Accessor& accessor, Zoom::Accessor& zoomAcsr)
: mAccessor(accessor)
, mZoomAccessor(zoomAcsr)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType const attribute)
    {
        auto clearCurrentRange = [&]()
        {
            auto const x1 = Zoom::Tools::getScaledXFromValue(zoomAcsr, *this, mLoopRange.getStart());
            auto const x2 = Zoom::Tools::getScaledXFromValue(zoomAcsr, *this, mLoopRange.getEnd());
            repaint(x1, 0, x2 - x1, getHeight());
        };
        switch(attribute)
        {
            case AttrType::playback:
            case AttrType::startPlayhead:
            case AttrType::runningPlayhead:
                break;
            case AttrType::looping:
            {
                clearCurrentRange();
            }
                break;
            case AttrType::loopRange:
            {
                clearCurrentRange();
                mLoopRange = acsr.getAttr<AttrType::loopRange>();
                auto const x1 = Zoom::Tools::getScaledXFromValue(zoomAcsr, *this, mLoopRange.getStart());
                auto const x2 = Zoom::Tools::getScaledXFromValue(zoomAcsr, *this, mLoopRange.getEnd());
                repaint(x1, 0, x2 - x1, getHeight());
            }
            case AttrType::gain:
                break;
        }
    };
    
    mZoomListener.onAttrChanged = [&](Zoom::Accessor const& acsr, Zoom::AttrType const attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::minimumLength:
            case Zoom::AttrType::anchor:
                break;
            case Zoom::AttrType::visibleRange:
            {
                repaint();
            }
                break;
        }
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
}

Transport::LoopBar::~LoopBar()
{
    mZoomAccessor.removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Transport::LoopBar::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
    g.setColour(findColour(ColourIds::thumbCoulourId));
    auto const x1 = Zoom::Tools::getScaledXFromValue(mZoomAccessor, *this, mLoopRange.getStart());
    auto const x2 = Zoom::Tools::getScaledXFromValue(mZoomAccessor, *this, mLoopRange.getEnd());
    if(x1 <= x2)
    {
        return;
    }
    if(mAccessor.getAttr<AttrType::looping>())
    {
        g.fillRoundedRectangle(static_cast<float>(x1), 0.0f, static_cast<float>(x2 - x1), static_cast<float>(getHeight()), 2.0f);
    }
    else
    {
        g.drawRoundedRectangle(static_cast<float>(x1 + 1), 1.0f, static_cast<float>(std::max(x2 - x1 - 2, 0)), static_cast<float>(std::max(getHeight() - 2, 0)), 2.0f, 1.0f);
    }
}

void Transport::LoopBar::mouseMove(juce::MouseEvent const& event)
{
    mSavedRange = mAccessor.getAttr<AttrType::loopRange>();
    mCickTime = Zoom::Tools::getScaledValueFromWidth(mZoomAccessor, *this, event.x);
    if(!event.mods.isShiftDown())
    {
        auto constexpr pixelEpsilon = 2.0;
        auto const timeRange = mZoomAccessor.getAttr<Zoom::AttrType::visibleRange>();
        auto const timeEpsilon = static_cast<double>(pixelEpsilon) * timeRange.getLength() / static_cast<double>(getWidth());
        if(!mSavedRange.isEmpty() && std::abs(mSavedRange.getStart() - mCickTime) <= timeEpsilon)
        {
            mEditMode = EditMode::resizeLeft;
            setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
        }
        else if(!mSavedRange.isEmpty() && std::abs(mSavedRange.getEnd() - mCickTime) <= timeEpsilon)
        {
            mEditMode = EditMode::resizeRight;
            setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
        }
        else
        {
            mEditMode = EditMode::select;
            setMouseCursor(juce::MouseCursor::NormalCursor);
        }
    }
    else if(mSavedRange.contains({mCickTime, mCickTime}))
    {
        mEditMode = EditMode::drag;
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
    else
    {
        mEditMode = EditMode::none;
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void Transport::LoopBar::mouseDown(juce::MouseEvent const& event)
{
    mouseMove(event);
    mouseDrag(event);
}

void Transport::LoopBar::mouseDrag(juce::MouseEvent const& event)
{
    switch(mEditMode)
    {
        case EditMode::none:
            break;
        case EditMode::select:
        {
            auto const time = Zoom::Tools::getScaledValueFromWidth(mZoomAccessor, *this, event.x);
            mAccessor.setAttr<AttrType::loopRange>(juce::Range<double>::between(mCickTime, time), NotificationType::synchronous);
        }
            break;
        case EditMode::drag:
        {
            auto const timeRange = mZoomAccessor.getAttr<Zoom::AttrType::visibleRange>();
            auto const time = Zoom::Tools::getScaledValueFromWidth(mZoomAccessor, *this, event.getDistanceFromDragStartX()) - timeRange.getStart();
            mAccessor.setAttr<AttrType::loopRange>(mSavedRange + time, NotificationType::synchronous);
        }
            break;
        case EditMode::resizeLeft:
        {
            auto const time = Zoom::Tools::getScaledValueFromWidth(mZoomAccessor, *this, event.x);
            mAccessor.setAttr<AttrType::loopRange>(mSavedRange.withStart(time), NotificationType::synchronous);
        }
            break;
        case EditMode::resizeRight:
        {
            auto const time = Zoom::Tools::getScaledValueFromWidth(mZoomAccessor, *this, event.x);
            mAccessor.setAttr<AttrType::loopRange>(mSavedRange.withEnd(time), NotificationType::synchronous);
        }
            break;
    }
    
}

void Transport::LoopBar::mouseUp(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

ANALYSE_FILE_END
