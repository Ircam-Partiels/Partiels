#include "AnlTransportPlayheadContainer.h"

ANALYSE_FILE_BEGIN

Transport::PlayheadContainer::PlayheadContainer(Accessor& accessor, Zoom::Accessor& zoomAcsr)
: mAccessor(accessor)
, mZoomAccessor(zoomAcsr)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType const attribute)
    {
        switch(attribute)
        {
            case AttrType::playback:
                break;
            case AttrType::startPlayhead:
            {
                repaint(toPixel(mStartPlayhead), 0, 1, getHeight());
                mStartPlayhead = acsr.getAttr<AttrType::startPlayhead>();
                repaint(toPixel(mStartPlayhead), 0, 1, getHeight());
            }
                break;
            case AttrType::runningPlayhead:
            {
                repaint(toPixel(mRunningPlayhead), 0, 1, getHeight());
                mRunningPlayhead = acsr.getAttr<AttrType::runningPlayhead>();
                repaint(toPixel(mRunningPlayhead), 0, 1, getHeight());
            }
                break;
            case AttrType::looping:
            case AttrType::loopRange:
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

Transport::PlayheadContainer::~PlayheadContainer()
{
    mZoomAccessor.removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Transport::PlayheadContainer::paint(juce::Graphics& g)
{
    g.setColour(findColour(ColourIds::startPlayheadColourId));
    g.fillRect(toPixel(mStartPlayhead), 0, 1, getHeight());
    g.setColour(findColour(ColourIds::runningPlayheadColourId));
    g.fillRect(toPixel(mRunningPlayhead), 0, 1, getHeight());
}

void Transport::PlayheadContainer::mouseDown(juce::MouseEvent const& event)
{
    auto const relEvent = event.getEventRelativeTo(this);
    mAccessor.setAttr<AttrType::startPlayhead>(toTime(relEvent.x), NotificationType::synchronous);
}

int Transport::PlayheadContainer::toPixel(double time) const
{
    auto const range = mZoomAccessor.getAttr<Zoom::AttrType::visibleRange>();
    auto const width = getWidth();
    if(width <= 0 || range.isEmpty())
    {
        return 0;
    }
    return static_cast<int>(std::round((time - range.getStart()) / range.getLength() * static_cast<double>(width)));
}

double Transport::PlayheadContainer::toTime(int pixel) const
{
    auto const range = mZoomAccessor.getAttr<Zoom::AttrType::visibleRange>();
    auto const width = getWidth();
    if(width <= 0 || range.isEmpty())
    {
        return 0.0;
    }
    return static_cast<double>(pixel) / static_cast<double>(width) * range.getLength() + range.getStart();
}

ANALYSE_FILE_END
