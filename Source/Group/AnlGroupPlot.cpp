#include "AnlGroupPlot.h"
#include "../Track/AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Group::Plot::Plot(Accessor& accessor, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr)
: mAccessor(accessor)
, mTransportAccessor(transportAcsr)
, mTimeZoomAccessor(timeZoomAcsr)
, mLayoutNotifier(accessor, [this]()
                  {
                      updateContent();
                  })
{
    juce::ignoreUnused(mAccessor);
    juce::ignoreUnused(mTransportAccessor);

    mTrackListener.onAttrChanged = [this](Track::Accessor const& acsr, Track::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Track::AttrType::identifier:
            case Track::AttrType::name:
            case Track::AttrType::file:
            case Track::AttrType::key:
            case Track::AttrType::description:
            case Track::AttrType::state:
            case Track::AttrType::height:
            case Track::AttrType::zoomLink:
            case Track::AttrType::zoomAcsr:
            case Track::AttrType::warnings:
            case Track::AttrType::processing:
            case Track::AttrType::focused:
                break;
            case Track::AttrType::grid:
            case Track::AttrType::results:
            case Track::AttrType::graphics:
            case Track::AttrType::colours:
            case Track::AttrType::channelsLayout:
            {
                repaint();
            }
            break;
        }
    };

    mZoomListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
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

    mGridListener.onAttrChanged = [this](Zoom::Grid::Accessor const& acsr, Zoom::Grid::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::height:
            case AttrType::colour:
            case AttrType::expanded:
            case AttrType::layout:
            case AttrType::tracks:
            case AttrType::focused:
                break;
            case AttrType::zoomid:
            {
                repaint();
            }
            break;
        }
    };

    setInterceptsMouseClicks(false, false);
    setCachedComponentImage(new LowResCachedComponentImage(*this));
    setSize(100, 80);
    mTimeZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::Plot::~Plot()
{
    mAccessor.removeListener(mListener);
    mTimeZoomAccessor.removeListener(mZoomListener);
    for(auto& trackAcsr : mTrackAccessors.getContents())
    {
        trackAcsr.second.get().getAcsr<Track::AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
        trackAcsr.second.get().getAcsr<Track::AcsrType::binZoom>().removeListener(mZoomListener);
        trackAcsr.second.get().getAcsr<Track::AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
        trackAcsr.second.get().getAcsr<Track::AcsrType::valueZoom>().removeListener(mZoomListener);
        trackAcsr.second.get().removeListener(mTrackListener);
    }
}

void Group::Plot::paint(juce::Graphics& g)
{
    auto const bounds = getLocalBounds();
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    auto const zoomid = mAccessor.getAttr<AttrType::zoomid>();
    auto const hasZoomTrack = Tools::hasTrackAcsr(mAccessor, zoomid);
    for(auto it = layout.crbegin(); it != layout.crend(); ++it)
    {
        auto const trackAcsr = Tools::getTrackAcsr(mAccessor, *it);
        if(trackAcsr.has_value())
        {
            auto const isSelected = (!hasZoomTrack && it == std::prev(layout.crend())) || zoomid == *it;
            auto const colour = isSelected ? findColour(Decorator::ColourIds::normalBorderColourId) : juce::Colours::transparentBlack;
            Track::Plot::paint(*trackAcsr, mTimeZoomAccessor, g, bounds, colour);
        }
    }
}

void Group::Plot::updateContent()
{
    mTrackAccessors.updateContents(
        mAccessor,
        [this](Track::Accessor& trackAccessor)
        {
            trackAccessor.addListener(mTrackListener, NotificationType::synchronous);
            trackAccessor.getAcsr<Track::AcsrType::valueZoom>().addListener(mZoomListener, NotificationType::synchronous);
            trackAccessor.getAcsr<Track::AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
            trackAccessor.getAcsr<Track::AcsrType::binZoom>().addListener(mZoomListener, NotificationType::synchronous);
            trackAccessor.getAcsr<Track::AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
            return std::ref(trackAccessor);
        },
        [this](std::reference_wrapper<Track::Accessor>& content)
        {
            content.get().getAcsr<Track::AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
            content.get().getAcsr<Track::AcsrType::binZoom>().removeListener(mZoomListener);
            content.get().getAcsr<Track::AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
            content.get().getAcsr<Track::AcsrType::valueZoom>().removeListener(mZoomListener);
            content.get().removeListener(mTrackListener);
        });
    repaint();
}

Group::Plot::Overlay::Overlay(Plot& plot)
: mPlot(plot)
, mAccessor(mPlot.mAccessor)
, mTimeZoomAccessor(mPlot.mTimeZoomAccessor)
, mSelectionBar(mPlot.mAccessor, mTimeZoomAccessor, mPlot.mTransportAccessor)
{
    addAndMakeVisible(mPlot);
    addAndMakeVisible(mSelectionBar);
    mSelectionBar.addMouseListener(this, true);
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
            case AttrType::zoomid:
                break;
            case AttrType::focused:
            {
                colourChanged();
            }
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
    mSelectionBar.setBounds(bounds);
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
    if(event.mods.isAltDown())
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
    if(event.mods.isAltDown() && !mSnapshotMode)
    {
        mSnapshotMode = true;
        showCameraCursor(true);
        mSelectionBar.setInterceptsMouseClicks(false, false);
    }
    else if(!event.mods.isAltDown() && mSnapshotMode)
    {
        mSnapshotMode = false;
        showCameraCursor(false);
        mSelectionBar.setInterceptsMouseClicks(true, true);
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
