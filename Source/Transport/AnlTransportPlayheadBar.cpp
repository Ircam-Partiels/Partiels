#include "AnlTransportPlayheadBar.h"
#include "../Zoom/AnlZoomTools.h"

ANALYSE_FILE_BEGIN

Transport::PlayheadBar::PlayheadBar(Accessor& accessor, Zoom::Accessor& zoomAcsr)
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
                repaint(Zoom::Tools::getScaledXFromValue(mZoomAccessor, *this, mStartPlayhead), 0, 1, getHeight());
                mStartPlayhead = acsr.getAttr<AttrType::startPlayhead>();
                repaint(Zoom::Tools::getScaledXFromValue(mZoomAccessor, *this, mStartPlayhead), 0, 1, getHeight());
            }
                break;
            case AttrType::runningPlayhead:
            {
                repaint(Zoom::Tools::getScaledXFromValue(mZoomAccessor, *this, mRunningPlayhead), 0, 1, getHeight());
                mRunningPlayhead = acsr.getAttr<AttrType::runningPlayhead>();
                repaint(Zoom::Tools::getScaledXFromValue(mZoomAccessor, *this, mRunningPlayhead), 0, 1, getHeight());
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

Transport::PlayheadBar::~PlayheadBar()
{
    mZoomAccessor.removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Transport::PlayheadBar::paint(juce::Graphics& g)
{
    g.setColour(findColour(ColourIds::startPlayheadColourId));
    g.fillRect(Zoom::Tools::getScaledXFromValue(mZoomAccessor, *this, mStartPlayhead), 0, 1, getHeight());
    g.setColour(findColour(ColourIds::runningPlayheadColourId));
    g.fillRect(Zoom::Tools::getScaledXFromValue(mZoomAccessor, *this, mRunningPlayhead), 0, 1, getHeight());
}

void Transport::PlayheadBar::mouseDown(juce::MouseEvent const& event)
{
    mouseDrag(event);
}

void Transport::PlayheadBar::mouseDrag(juce::MouseEvent const& event)
{
    auto const relEvent = event.getEventRelativeTo(this);
    mAccessor.setAttr<AttrType::startPlayhead>(Zoom::Tools::getScaledValueFromWidth(mZoomAccessor, *this, relEvent.x), NotificationType::synchronous);
}

ANALYSE_FILE_END
