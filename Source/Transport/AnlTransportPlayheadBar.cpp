#include "AnlTransportPlayheadBar.h"
#include "../Zoom/AnlZoomTools.h"

ANALYSE_FILE_BEGIN

Transport::PlayheadBar::PlayheadBar(Accessor& accessor, Zoom::Accessor& zoomAcsr)
: mAccessor(accessor)
, mZoomAccessor(zoomAcsr)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType const attribute)
    {
        auto clearPosition = [&](double position)
        {
            auto const x = static_cast<int>(std::round(Zoom::Tools::getScaledXFromValue(mZoomAccessor, *this, position)));
            repaint(x, 0, 1, getHeight());
        };
        
        switch(attribute)
        {
            case AttrType::playback:
                break;
            case AttrType::startPlayhead:
            {
                clearPosition(mStartPlayhead);
                mStartPlayhead = acsr.getAttr<AttrType::startPlayhead>();
                clearPosition(mStartPlayhead);
            }
            break;
            case AttrType::runningPlayhead:
            {
                clearPosition(mRunningPlayhead);
                mRunningPlayhead = acsr.getAttr<AttrType::runningPlayhead>();
                clearPosition(mRunningPlayhead);
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
            case Zoom::AttrType::gridInfo:
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
    auto const start = static_cast<int>(std::round(Zoom::Tools::getScaledXFromValue(mZoomAccessor, *this, mStartPlayhead)));
    g.fillRect(start, 0, 1, getHeight());
    g.setColour(findColour(ColourIds::runningPlayheadColourId));
    auto const running = static_cast<int>(std::round(Zoom::Tools::getScaledXFromValue(mZoomAccessor, *this, mRunningPlayhead)));
    g.fillRect(running, 0, 1, getHeight());
}

void Transport::PlayheadBar::mouseDown(juce::MouseEvent const& event)
{
    mouseDrag(event);
}

void Transport::PlayheadBar::mouseDrag(juce::MouseEvent const& event)
{
    auto const relEvent = event.getEventRelativeTo(this);
    auto const x = getLocalBounds().getHorizontalRange().clipValue(relEvent.x);
    mAccessor.setAttr<AttrType::startPlayhead>(Zoom::Tools::getScaledValueFromWidth(mZoomAccessor, *this, x), NotificationType::synchronous);
}

ANALYSE_FILE_END
