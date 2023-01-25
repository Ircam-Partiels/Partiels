#include "AnlGroupSnapshot.h"
#include "../Track/AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Group::Snapshot::Snapshot(Accessor& accessor, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr)
: mAccessor(accessor)
, mTransportAccessor(transportAcsr)
, mTimeZoomAccessor(timeZoomAcsr)
, mLayoutNotifier(accessor, [this]()
                  {
                      updateContent();
                  })
{
    mTrackListener.onAttrChanged = [this](Track::Accessor const& acsr, Track::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Track::AttrType::identifier:
            case Track::AttrType::file:
            case Track::AttrType::name:
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
            case Track::AttrType::font:
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

    mTransportListener.onAttrChanged = [this](Transport::Accessor const& acsr, Transport::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Transport::AttrType::startPlayhead:
            {
                if(!acsr.getAttr<Transport::AttrType::playback>())
                {
                    repaint();
                }
            }
            break;
            case Transport::AttrType::runningPlayhead:
            {
                if(acsr.getAttr<Transport::AttrType::playback>())
                {
                    repaint();
                }
            }
            break;
            case Transport::AttrType::playback:
            case Transport::AttrType::looping:
            case Transport::AttrType::loopRange:
            case Transport::AttrType::stopAtLoopEnd:
            case Transport::AttrType::gain:
            case Transport::AttrType::markers:
            case Transport::AttrType::magnetize:
            case Transport::AttrType::autoScroll:
            case Transport::AttrType::selection:
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
    setSize(100, 80);
    mTransportAccessor.addListener(mTransportListener, NotificationType::synchronous);
    mTimeZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::Snapshot::~Snapshot()
{
    mAccessor.removeListener(mListener);
    mTimeZoomAccessor.removeListener(mZoomListener);
    mTransportAccessor.removeListener(mTransportListener);
    for(auto& trackAcsr : mTrackAccessors.getContents())
    {
        trackAcsr.second.get().getAcsr<Track::AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
        trackAcsr.second.get().getAcsr<Track::AcsrType::binZoom>().removeListener(mZoomListener);
        trackAcsr.second.get().getAcsr<Track::AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
        trackAcsr.second.get().getAcsr<Track::AcsrType::valueZoom>().removeListener(mZoomListener);
        trackAcsr.second.get().removeListener(mTrackListener);
    }
    repaint();
}

void Group::Snapshot::paint(juce::Graphics& g)
{
    auto const bounds = getLocalBounds();
    auto const isPlaying = mTransportAccessor.getAttr<Transport::AttrType::playback>();
    auto const time = isPlaying ? mTransportAccessor.getAttr<Transport::AttrType::runningPlayhead>() : mTransportAccessor.getAttr<Transport::AttrType::startPlayhead>();
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
            Track::Snapshot::paint(*trackAcsr, mTimeZoomAccessor, time, g, bounds, colour);
        }
    }
}

void Group::Snapshot::updateContent()
{
    mTrackAccessors.updateContents(
        mAccessor,
        [this](Track::Accessor& trackAccessor)
        {
            trackAccessor.addListener(mTrackListener, NotificationType::synchronous);
            trackAccessor.getAcsr<Track::AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
            trackAccessor.getAcsr<Track::AcsrType::valueZoom>().addListener(mZoomListener, NotificationType::synchronous);
            trackAccessor.getAcsr<Track::AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
            trackAccessor.getAcsr<Track::AcsrType::binZoom>().addListener(mZoomListener, NotificationType::synchronous);
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
}

Group::Snapshot::Overlay::Overlay(Snapshot& snapshot)
: mSnapshot(snapshot)
, mAccessor(mSnapshot.mAccessor)
, mTransportAccessor(mSnapshot.mTransportAccessor)
{
    addAndMakeVisible(mSnapshot);
    setInterceptsMouseClicks(true, true);

    mTransportListener.onAttrChanged = [this](Transport::Accessor const& acsr, Transport::AttrType attribute)
    {
        switch(attribute)
        {
            case Transport::AttrType::startPlayhead:
            {
                if(!acsr.getAttr<Transport::AttrType::playback>())
                {
                    updateTooltip(getMouseXYRelative());
                }
            }
            break;
            case Transport::AttrType::runningPlayhead:
            {
                if(acsr.getAttr<Transport::AttrType::playback>())
                {
                    updateTooltip(getMouseXYRelative());
                }
            }
            break;
            case Transport::AttrType::playback:
            case Transport::AttrType::looping:
            case Transport::AttrType::loopRange:
            case Transport::AttrType::stopAtLoopEnd:
            case Transport::AttrType::gain:
            case Transport::AttrType::markers:
            case Transport::AttrType::magnetize:
            case Transport::AttrType::autoScroll:
            case Transport::AttrType::selection:
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
            case AttrType::zoomid:
                break;
            case AttrType::colour:
            {
                repaint();
            }
            break;
        }
    };

    mTransportAccessor.addListener(mTransportListener, NotificationType::synchronous);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::Snapshot::Overlay::~Overlay()
{
    mAccessor.removeListener(mListener);
    mTransportAccessor.removeListener(mTransportListener);
}

void Group::Snapshot::Overlay::resized()
{
    mSnapshot.setBounds(getLocalBounds());
}

void Group::Snapshot::Overlay::paint(juce::Graphics& g)
{
    g.fillAll(mAccessor.getAttr<AttrType::colour>());
}

void Group::Snapshot::Overlay::mouseMove(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
}

void Group::Snapshot::Overlay::mouseEnter(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
}

void Group::Snapshot::Overlay::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    setTooltip("");
}

void Group::Snapshot::Overlay::updateTooltip(juce::Point<int> const& pt)
{
    if(!getLocalBounds().contains(pt))
    {
        setTooltip("");
        return;
    }
    auto const isPlaying = mTransportAccessor.getAttr<Transport::AttrType::playback>();
    auto const time = isPlaying ? mTransportAccessor.getAttr<Transport::AttrType::runningPlayhead>() : mTransportAccessor.getAttr<Transport::AttrType::startPlayhead>();
    auto tooltip = Format::secondsToString(time);
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    for(auto const& identifier : layout)
    {
        auto trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
        if(trackAcsr.has_value())
        {
            auto const name = trackAcsr->get().getAttr<Track::AttrType::name>();
            auto const tip = Track::Tools::getValueTootip(trackAcsr->get(), mSnapshot.mTimeZoomAccessor, *this, pt.y, time);
            tooltip += "\n" + name + ": " + (tip.isEmpty() ? "-" : tip);
        }
    }
    setTooltip(tooltip);
}

ANALYSE_FILE_END
