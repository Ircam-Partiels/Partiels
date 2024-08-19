#include "MiscTransportBars.h"
#include "MiscTransportTools.h"

MISC_FILE_BEGIN

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
            case AttrType::autoScroll:
            case AttrType::gain:
            case AttrType::markers:
            case AttrType::magnetize:
            case AttrType::selection:
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
    auto const time = Zoom::Tools::getScaledValueFromWidth(mZoomAccessor, *this, x);
    auto const markers = mAccessor.getAttr<AttrType::magnetize>() ? mAccessor.getAttr<AttrType::markers>() : std::set<double>{};
    auto const expectedTime = Zoom::Tools::getNearestValue(mZoomAccessor, time, markers);
    if(onStartPlayheadChanged != nullptr)
    {
        onStartPlayheadChanged(expectedTime);
    }
    else
    {
        mAccessor.setAttr<AttrType::startPlayhead>(expectedTime, NotificationType::synchronous);
    }
}

Transport::SelectionBar::SelectionBar(Accessor& accessor, Zoom::Accessor& zoomAcsr)
: mAccessor(accessor)
, mZoomAccessor(zoomAcsr)
, mSelectionBar(zoomAcsr)
, mPlayheadBar(accessor, zoomAcsr)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType const attribute)
    {
        switch(attribute)
        {
            case AttrType::playback:
            case AttrType::startPlayhead:
            case AttrType::runningPlayhead:
            case AttrType::stopAtLoopEnd:
            case AttrType::autoScroll:
            case AttrType::gain:
            case AttrType::looping:
            case AttrType::loopRange:
                break;
            case AttrType::selection:
            {
                auto const range = acsr.getAttr<AttrType::selection>();
                auto const anchor = std::get<1_z>(mSelectionBar.getRange());
                mSelectionBar.setRange(range, anchor, juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::magnetize:
            case AttrType::markers:
            {
                mSelectionBar.setMarkers(acsr.getAttr<AttrType::magnetize>() ? acsr.getAttr<AttrType::markers>() : std::set<double>{});
            }
            break;
        }
    };

    mSelectionBar.onRangeChanged = [this](juce::Range<double> const& range, Anchor anchor)
    {
        if(onSelectionRangeChanged != nullptr)
        {
            onSelectionRangeChanged(range);
        }
        else
        {
            mAccessor.setAttr<AttrType::selection>(range, NotificationType::synchronous);
            switch(anchor)
            {
                case Anchor::start:
                    mAccessor.setAttr<AttrType::startPlayhead>(range.getEnd(), NotificationType::synchronous);
                    break;
                case Anchor::end:
                    mAccessor.setAttr<AttrType::startPlayhead>(range.getStart(), NotificationType::synchronous);
                    break;
            }
        }
    };

    mSelectionBar.onMouseDown = [this](double value)
    {
        mAccessor.setAttr<AttrType::startPlayhead>(value, NotificationType::synchronous);
        mAccessor.setAttr<AttrType::selection>(juce::Range<double>::emptyRange(value), NotificationType::synchronous);
    };

    mSelectionBar.onMouseUp = [this](double value)
    {
        juce::ignoreUnused(value);
        auto const range = std::get<0_z>(mSelectionBar.getRange());
        mAccessor.setAttr<AttrType::startPlayhead>(range.getStart(), NotificationType::synchronous);
    };

    mSelectionBar.onDoubleClick = [this](double value)
    {
        auto const& markers = mAccessor.getAttr<AttrType::markers>();
        auto const range = Zoom::Tools::getNearestRange(mZoomAccessor, value, markers);
        mSelectionBar.setRange(range, Anchor::start, juce::NotificationType::sendNotificationSync);
        mAccessor.setAttr<AttrType::startPlayhead>(range.getStart(), NotificationType::synchronous);
    };

    addAndMakeVisible(mSelectionBar);
    addAndMakeVisible(mPlayheadBar);
    mPlayheadBar.setInterceptsMouseClicks(false, false);
    colourChanged();

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Transport::SelectionBar::~SelectionBar()
{
    mAccessor.removeListener(mListener);
}

void Transport::SelectionBar::resized()
{
    auto const bounds = getLocalBounds();
    mSelectionBar.setBounds(bounds);
    mPlayheadBar.setBounds(bounds);
}

void Transport::SelectionBar::colourChanged()
{
    mSelectionBar.setColour(Zoom::SelectionBar::ColourIds::backgroundColourId, findColour(ColourIds::backgroundColourId));
    mSelectionBar.setColour(Zoom::SelectionBar::ColourIds::thumbCoulourId, findColour(ColourIds::thumbCoulourId));
}

void Transport::SelectionBar::setDefaultMouseCursor(juce::MouseCursor const& cursor)
{
    mSelectionBar.setDefaultMouseCursor(cursor);
}

Transport::LoopBar::LoopBar(Accessor& accessor, Zoom::Accessor& zoomAcsr)
: mAccessor(accessor)
, mZoomAccessor(zoomAcsr)
, mSelectionBar(zoomAcsr)
, mPlayheadBar(accessor, zoomAcsr)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType const attribute)
    {
        switch(attribute)
        {
            case AttrType::playback:
            case AttrType::startPlayhead:
            case AttrType::runningPlayhead:
            case AttrType::stopAtLoopEnd:
            case AttrType::autoScroll:
            case AttrType::gain:
            case AttrType::selection:
                break;
            case AttrType::looping:
            {
                mSelectionBar.setState(acsr.getAttr<AttrType::looping>());
            }
            break;
            case AttrType::loopRange:
            {
                auto const range = acsr.getAttr<AttrType::loopRange>();
                auto const anchor = std::get<1_z>(mSelectionBar.getRange());
                mSelectionBar.setRange(range, anchor, juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::magnetize:
            case AttrType::markers:
            {
                mSelectionBar.setMarkers(acsr.getAttr<AttrType::magnetize>() ? acsr.getAttr<AttrType::markers>() : std::set<double>{});
            }
            break;
        }
    };

    mSelectionBar.onRangeChanged = [this](juce::Range<double> const& range, Anchor anchor)
    {
        if(onLoopRangeChanged != nullptr)
        {
            onLoopRangeChanged(range);
        }
        else
        {
            mAccessor.setAttr<AttrType::loopRange>(range, NotificationType::synchronous);
            mAccessor.setAttr<AttrType::selection>(range, NotificationType::synchronous);
            switch(anchor)
            {
                case Anchor::start:
                    mAccessor.setAttr<AttrType::startPlayhead>(range.getEnd(), NotificationType::synchronous);
                    break;
                case Anchor::end:
                    mAccessor.setAttr<AttrType::startPlayhead>(range.getStart(), NotificationType::synchronous);
                    break;
            }
        }
    };

    mSelectionBar.onMouseDown = [this](double value)
    {
        mAccessor.setAttr<AttrType::startPlayhead>(value, NotificationType::synchronous);
    };

    mSelectionBar.onMouseUp = [this](double value)
    {
        juce::ignoreUnused(value);
        auto const range = std::get<0_z>(mSelectionBar.getRange());
        mAccessor.setAttr<AttrType::startPlayhead>(range.getStart(), NotificationType::synchronous);
    };

    mSelectionBar.onDoubleClick = [this](double value)
    {
        auto const& markers = mAccessor.getAttr<AttrType::markers>();
        auto const range = Zoom::Tools::getNearestRange(mZoomAccessor, value, markers);
        mSelectionBar.setRange(range, Anchor::start, juce::NotificationType::sendNotificationSync);
        mAccessor.setAttr<AttrType::startPlayhead>(range.getStart(), NotificationType::synchronous);
    };

    addAndMakeVisible(mSelectionBar);
    addAndMakeVisible(mPlayheadBar);
    mPlayheadBar.setInterceptsMouseClicks(false, false);
    colourChanged();

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Transport::LoopBar::~LoopBar()
{
    mAccessor.removeListener(mListener);
}

void Transport::LoopBar::resized()
{
    auto const bounds = getLocalBounds();
    mSelectionBar.setBounds(bounds.reduced(0, 2));
    mPlayheadBar.setBounds(bounds);
}

void Transport::LoopBar::colourChanged()
{
    mSelectionBar.setColour(Zoom::SelectionBar::ColourIds::backgroundColourId, findColour(ColourIds::backgroundColourId));
    mSelectionBar.setColour(Zoom::SelectionBar::ColourIds::thumbCoulourId, findColour(ColourIds::thumbCoulourId));
}

MISC_FILE_END
