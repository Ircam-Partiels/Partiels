#include "AnlGroupPlot.h"
#include "../Track/AnlTrackTools.h"
#include "../Zoom/AnlZoomTools.h"

ANALYSE_FILE_BEGIN

Group::Plot::Plot(Accessor& accessor, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr)
: mAccessor(accessor)
, mTransportAccessor(transportAcsr)
, mTimeZoomAccessor(timeZoomAcsr)
{
    juce::ignoreUnused(mAccessor);
    juce::ignoreUnused(mTransportAccessor);
    juce::ignoreUnused(mTimeZoomAccessor);
    setInterceptsMouseClicks(false, false);
    setSize(100, 80);

    mListener.onAttrChanged = [this](class Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::height:
            case AttrType::colour:
            case AttrType::expanded:
            case AttrType::focused:
                break;
            case AttrType::layout:
            case AttrType::tracks:
            {
                removeAllChildren();
                mTrackPlots.updateContents(
                    mAccessor,
                    [this](Track::Accessor& trackAccessor)
                    {
                        return std::make_unique<Track::Plot>(trackAccessor, mTimeZoomAccessor, mTransportAccessor);
                    },
                    nullptr);

                auto const& layout = mAccessor.getAttr<AttrType::layout>();
                auto const& group = mTrackPlots.getContents();
                for(auto const& identifier : layout)
                {
                    auto it = group.find(identifier);
                    if(it != group.cend() && it->second != nullptr)
                    {
                        addAndMakeVisible(it->second.get(), 0);
                    }
                }
                resized();
            }
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::Plot::~Plot()
{
    mAccessor.removeListener(mListener);
}

void Group::Plot::resized()
{
    auto const bounds = getLocalBounds();
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    auto const& contents = mTrackPlots.getContents();
    for(auto const& identifier : layout)
    {
        auto it = contents.find(identifier);
        if(it != contents.cend() && it->second != nullptr)
        {
            it->second->setBounds(bounds);
        }
    }
}

Group::Plot::Overlay::Overlay(Plot& plot)
: mPlot(plot)
, mAccessor(mPlot.mAccessor)
, mTimeZoomAccessor(mPlot.mTimeZoomAccessor)
, mTransportPlayheadBar(mPlot.mTransportAccessor, mTimeZoomAccessor)
{
    addAndMakeVisible(mPlot);
    addAndMakeVisible(mTransportPlayheadBar);
    mTransportPlayheadBar.setInterceptsMouseClicks(false, false);
    addMouseListener(&mTransportPlayheadBar, false);
    setInterceptsMouseClicks(true, true);

    mTimeZoomListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
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
                updateTooltip(getMouseXYRelative());
            }
            break;
        }
    };

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::height:
            case AttrType::expanded:
            case AttrType::layout:
            case AttrType::tracks:
            case AttrType::focused:
                break;
            case AttrType::colour:
            {
                repaint();
            }
            break;
        }
    };

    mTimeZoomAccessor.addListener(mTimeZoomListener, NotificationType::synchronous);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::Plot::Overlay::~Overlay()
{
    mAccessor.removeListener(mListener);
    mTimeZoomAccessor.removeListener(mTimeZoomListener);
}

void Group::Plot::Overlay::resized()
{
    auto const bounds = getLocalBounds();
    mPlot.setBounds(bounds);
    mTransportPlayheadBar.setBounds(bounds);
}

void Group::Plot::Overlay::paint(juce::Graphics& g)
{
    g.fillAll(mAccessor.getAttr<AttrType::colour>());
}

void Group::Plot::Overlay::mouseMove(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
    updateMode(event);
}

void Group::Plot::Overlay::mouseEnter(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
    updateMode(event);
}

void Group::Plot::Overlay::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    setTooltip("");
}

void Group::Plot::Overlay::mouseDown(juce::MouseEvent const& event)
{
    updateMode(event);
    if(event.mods.isCtrlDown())
    {
        takeSnapshot(mPlot, mAccessor.getAttr<AttrType::name>(), juce::Colours::transparentBlack);
    }
}

void Group::Plot::Overlay::mouseDrag(juce::MouseEvent const& event)
{
    updateMode(event);
}

void Group::Plot::Overlay::mouseUp(juce::MouseEvent const& event)
{
    updateMode(event);
}

void Group::Plot::Overlay::updateMode(juce::MouseEvent const& event)
{
    if(event.mods.isCtrlDown() && !mSnapshotMode)
    {
        mSnapshotMode = true;
        showCameraCursor(true);
        removeMouseListener(&mTransportPlayheadBar);
    }
    else if(!event.mods.isCtrlDown() && mSnapshotMode)
    {
        mSnapshotMode = false;
        showCameraCursor(false);
        addMouseListener(&mTransportPlayheadBar, false);
    }
}

void Group::Plot::Overlay::updateTooltip(juce::Point<int> const& pt)
{
    if(!getLocalBounds().contains(pt))
    {
        setTooltip("");
        return;
    }
    auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, pt.x);
    auto tooltip = Format::secondsToString(time);
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    for(auto const& identifier : layout)
    {
        auto trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
        if(trackAcsr.has_value())
        {
            auto const name = trackAcsr->get().getAttr<Track::AttrType::name>();
            auto const tip = Track::Tools::getValueTootip(trackAcsr->get(), mTimeZoomAccessor, *this, pt.y, time);
            tooltip += "\n" + name + ": " + (tip.isEmpty() ? "-" : tip);
        }
    }
    setTooltip(tooltip);
}

ANALYSE_FILE_END
