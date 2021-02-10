#include "AnlZoomPlayhead.h"

ANALYSE_FILE_BEGIN

Zoom::Playhead::Playhead(Accessor& accessor, juce::BorderSize<int> const& borderSize)
: mAccessor(accessor)
, mBorderSize(borderSize)
{
    setInterceptsMouseClicks(false, false);
    mListener.onAttrChanged = [&](Zoom::Accessor const& acsr, Zoom::AttrType const attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::minimumLength:
                break;
            case Zoom::AttrType::visibleRange:
            {
                parentSizeChanged();
            }
                break;
        }
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Zoom::Playhead::~Playhead()
{
    mAccessor.removeListener(mListener);
}

void Zoom::Playhead::setBorderSize(juce::BorderSize<int> const& borderSize)
{
    mBorderSize = borderSize;
    parentSizeChanged();
}

void Zoom::Playhead::setPosition(double position)
{
    mPosition = position;
    parentSizeChanged();
}

void Zoom::Playhead::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::playheadColourId));
}

void Zoom::Playhead::parentSizeChanged()
{
    auto const range = mAccessor.getAttr<Zoom::AttrType::visibleRange>();
    auto const width = getParentWidth() - mBorderSize.getLeftAndRight() - 1;
    auto const height = getParentHeight() - mBorderSize.getTopAndBottom();
    if(width <= 0 || range.isEmpty())
    {
        return;
    }
    auto const position = range.clipValue(mPosition);
    auto const x = std::round((position - range.getStart()) / range.getLength() * static_cast<double>(width));
    setBounds(static_cast<int>(x) + mBorderSize.getLeft(), mBorderSize.getTop(), 1, height);
}

void Zoom::Playhead::colourChanged()
{
    setOpaque(findColour(ColourIds::playheadColourId).isOpaque());
}

ANALYSE_FILE_END
