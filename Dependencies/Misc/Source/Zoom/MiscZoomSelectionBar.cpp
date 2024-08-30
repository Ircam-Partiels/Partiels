#include "MiscZoomSelectionBar.h"
#include "MiscZoomTools.h"

MISC_FILE_BEGIN

Zoom::SelectionBar::SelectionBar(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType const attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::minimumLength:
            case AttrType::anchor:
            case AttrType::globalRange:
                break;
            case AttrType::visibleRange:
            {
                repaint();
            }
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Zoom::SelectionBar::~SelectionBar()
{
    mAccessor.removeListener(mListener);
}

bool Zoom::SelectionBar::getState() const
{
    return mActive;
}

void Zoom::SelectionBar::setState(bool active)
{
    if(mActive != active)
    {
        mActive = active;
        repaint();
    }
}

void Zoom::SelectionBar::setDefaultMouseCursor(juce::MouseCursor const& c)
{
    mDefaultMouseCursor = c;
}

std::tuple<juce::Range<double>, Zoom::SelectionBar::Anchor> Zoom::SelectionBar::getRange() const
{
    return {mRange, mAnchor};
}

void Zoom::SelectionBar::setRange(juce::Range<double> const& range, Anchor anchor, juce::NotificationType notification)
{
    if(mRange == range && mAnchor == anchor)
    {
        return;
    }
    repaintRange();
    mRange = range;
    mAnchor = anchor;
    repaintRange();
    if(notification == juce::NotificationType::dontSendNotification)
    {
        return;
    }
    else if(notification == juce::NotificationType::sendNotificationAsync)
    {
        juce::WeakReference<juce::Component> weakReference(this);
        juce::MessageManager::callAsync([=, this]()
                                        {
                                            if(weakReference.get() == nullptr)
                                            {
                                                return;
                                            }
                                            changed();
                                        });
    }
    else
    {
        changed();
    }
}

void Zoom::SelectionBar::setMarkers(std::set<double> const& markers)
{
    mMarkers = markers;
}

void Zoom::SelectionBar::changed()
{
    if(onRangeChanged != nullptr)
    {
        onRangeChanged(mRange, mAnchor);
    }
}

void Zoom::SelectionBar::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
    auto const x1 = Tools::getScaledXFromValue(mAccessor, *this, mRange.getStart());
    auto const x2 = Tools::getScaledXFromValue(mAccessor, *this, mRange.getEnd());
    if(x1 >= x2)
    {
        return;
    }
    g.setColour(findColour(ColourIds::thumbCoulourId));
    juce::Rectangle<float> const rectangle(static_cast<float>(x1), 0.0f, static_cast<float>(x2 - x1), static_cast<float>(getHeight()));
    if(mActive)
    {
        g.fillRoundedRectangle(rectangle, 2.0f);
    }
    else
    {
        g.drawRoundedRectangle(rectangle, 2.0f, 1.0f);
    }
}

void Zoom::SelectionBar::mouseMove(juce::MouseEvent const& event)
{
    auto constexpr pixelEpsilon = 2.0;
    auto const value = Tools::getScaledValueFromWidth(mAccessor, *this, event.x);
    auto const valueRange = mAccessor.getAttr<Zoom::AttrType::visibleRange>();
    auto const valueEpsilon = static_cast<double>(pixelEpsilon) * valueRange.getLength() / static_cast<double>(getWidth());
    if(!mRange.isEmpty() && std::abs(mRange.getStart() - value) <= valueEpsilon)
    {
        setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
    }
    else if(!mRange.isEmpty() && std::abs(mRange.getEnd() - value) <= valueEpsilon)
    {
        setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
    }
    else
    {
        setMouseCursor(mDefaultMouseCursor);
    }
}

void Zoom::SelectionBar::mouseDown(juce::MouseEvent const& event)
{
    auto constexpr pixelEpsilon = 2.0;
    auto const value = Tools::getScaledValueFromWidth(mAccessor, *this, event.x);
    auto const valueRange = mAccessor.getAttr<Zoom::AttrType::visibleRange>();
    auto const valueEpsilon = static_cast<double>(pixelEpsilon) * valueRange.getLength() / static_cast<double>(getWidth());
    if(!mRange.isEmpty() && std::abs(mRange.getStart() - value) <= valueEpsilon)
    {
        setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
        mCickPosition = mRange.getEnd();
        if(onMouseDown != nullptr)
        {
            onMouseDown(mRange.getStart());
        }
    }
    else if(!mRange.isEmpty() && std::abs(mRange.getEnd() - value) <= valueEpsilon)
    {
        setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
        mCickPosition = mRange.getStart();
        if(onMouseDown != nullptr)
        {
            onMouseDown(mRange.getEnd());
        }
    }
    else
    {
        setMouseCursor(mDefaultMouseCursor);
        mCickPosition = Tools::getNearestValue(mAccessor, value, mMarkers);
        if(onMouseDown != nullptr)
        {
            onMouseDown(mCickPosition);
        }
    }
}

void Zoom::SelectionBar::mouseDrag(juce::MouseEvent const& event)
{
    if(!event.mouseWasDraggedSinceMouseDown())
    {
        return;
    }
    auto const value = Tools::getScaledValueFromWidth(mAccessor, *this, event.x);
    auto const usedValue = Tools::getNearestValue(mAccessor, value, mMarkers);
    auto const anchor = usedValue > mCickPosition ? Anchor::start : Anchor::end;
    setMouseCursor(usedValue > mCickPosition ? juce::MouseCursor::RightEdgeResizeCursor : juce::MouseCursor::LeftEdgeResizeCursor);
    setRange(juce::Range<double>::between(mCickPosition, usedValue), anchor, juce::NotificationType::sendNotificationSync);
}

void Zoom::SelectionBar::mouseUp(juce::MouseEvent const& event)
{
    setMouseCursor(mDefaultMouseCursor);
    if(!event.mouseWasDraggedSinceMouseDown())
    {
        return;
    }
    if(onMouseUp != nullptr)
    {
        auto const time = Tools::getScaledValueFromWidth(mAccessor, *this, event.x);
        onMouseUp(Tools::getNearestValue(mAccessor, time, mMarkers));
    }
}

void Zoom::SelectionBar::mouseDoubleClick(juce::MouseEvent const& event)
{
    auto const time = Tools::getScaledValueFromWidth(mAccessor, *this, event.x);
    if(onDoubleClick != nullptr)
    {
        onDoubleClick(time);
    }
    else
    {
        if(event.mods.isCommandDown())
        {
            setRange(juce::Range<double>{}, Anchor::start, juce::NotificationType::sendNotificationSync);
        }
        else
        {
            setRange(Tools::getNearestRange(mAccessor, time, mMarkers), Anchor::start, juce::NotificationType::sendNotificationSync);
        }
    }
}

void Zoom::SelectionBar::colourChanged()
{
    repaintRange();
}

void Zoom::SelectionBar::repaintRange()
{
    auto const x1 = static_cast<int>(std::floor(Tools::getScaledXFromValue(mAccessor, *this, mRange.getStart())));
    auto const x2 = static_cast<int>(std::ceil(Tools::getScaledXFromValue(mAccessor, *this, mRange.getEnd())));
    repaint(x1 - 1, 0, x2 - x1 + 2, getHeight());
}

MISC_FILE_END
