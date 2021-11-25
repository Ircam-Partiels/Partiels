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
            auto const x = static_cast<int>(std::floor(Zoom::Tools::getScaledXFromValue(mZoomAccessor, *this, position)));
            repaint(x - 1, 0, 3, getHeight());
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
            case AttrType::stopAtLoopEnd:
            case AttrType::gain:
            case AttrType::markers:
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
    auto const start = Zoom::Tools::getScaledXFromValue(mZoomAccessor, *this, mStartPlayhead);
    g.setColour(findColour(ColourIds::startPlayheadColourId));
    g.fillRect(static_cast<float>(start), 0.0f, 1.0f, static_cast<float>(getHeight()));

    auto const running = Zoom::Tools::getScaledXFromValue(mZoomAccessor, *this, mRunningPlayhead);
    g.setColour(findColour(ColourIds::runningPlayheadColourId));
    g.fillRect(static_cast<float>(running), 0.0f, 1.0f, static_cast<float>(getHeight()));
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
