#include "AnlGroupSlice.h"
#include "../Track/AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Group::Slice::WindowContainer::WindowContainer(Slice& slice)
: FloatingWindowContainer(juce::translate("Instant Results"), slice)
, mSlice(slice)
{
    mFloatingWindow.setResizable(true, false);
    mBoundsConstrainer.setMinimumWidth(160);
    mBoundsConstrainer.setMinimumHeight(100);
    mFloatingWindow.setConstrainer(&mBoundsConstrainer);

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::name:
            {
                mFloatingWindow.setName(juce::translate("ANLNAME INSTANT RESULTS").replace("ANLNAME", acsr.getAttr<AttrType::name>().toUpperCase()));
            }
            break;
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::colour:
            case AttrType::expanded:
            case AttrType::layout:
            case AttrType::tracks:
            case AttrType::focused:
            case AttrType::referenceid:
                break;
        }
    };
    mSlice.mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::Slice::WindowContainer::~WindowContainer()
{
    mSlice.mAccessor.removeListener(mListener);
}

Group::Slice::Plot::Plot(Accessor& accessor, Transport::Accessor& transportAccessor)
: mAccessor(accessor)
, mTransportAccessor(transportAccessor)
, mLayoutNotifier(accessor, [this]()
                  {
                      updateContent();
                  },
                  {Track::AttrType::identifier, Track::AttrType::showInGroup})
{
    mTrackListener.onAttrChanged = [this]([[maybe_unused]] Track::Accessor const& acsr, Track::AttrType attribute)
    {
        switch(attribute)
        {
            case Track::AttrType::identifier:
            case Track::AttrType::file:
            case Track::AttrType::fileDescription:
            case Track::AttrType::voiceLink:
            case Track::AttrType::voiceCounts:
            case Track::AttrType::key:
            case Track::AttrType::inputs:
            case Track::AttrType::state:
            case Track::AttrType::height:
            case Track::AttrType::zoomValueMode:
            case Track::AttrType::zoomLink:
            case Track::AttrType::zoomAcsr:
            case Track::AttrType::useInputResultsExtraThresholds:
            case Track::AttrType::extraThresholds:
            case Track::AttrType::warnings:
            case Track::AttrType::processing:
            case Track::AttrType::focused:
            case Track::AttrType::oscIdentifier:
            case Track::AttrType::sendViaOsc:
            case Track::AttrType::graphics:
                break;
            case Track::AttrType::name:
            case Track::AttrType::description:
            case Track::AttrType::grid:
            case Track::AttrType::results:
            case Track::AttrType::edit:
            case Track::AttrType::graphicsSettings:
            case Track::AttrType::channelsLayout:
            case Track::AttrType::showInGroup:
            case Track::AttrType::zoomLogScale:
            case Track::AttrType::sampleRate:
            {
                repaint();
                if(isShowing())
                {
                    updateTooltip(getMouseXYRelative());
                }
            }
            break;
        }
    };

    mZoomListener.onAttrChanged = [this]([[maybe_unused]] Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::minimumLength:
            case Zoom::AttrType::anchor:
                break;
            case Zoom::AttrType::visibleRange:
            {
                repaint();
                if(isShowing())
                {
                    updateTooltip(getMouseXYRelative());
                }
            }
            break;
        }
    };

    mGridListener.onAttrChanged = [this]([[maybe_unused]] Zoom::Grid::Accessor const& acsr, [[maybe_unused]] Zoom::Grid::AttrType attribute)
    {
        repaint();
    };

    mTransportListener.onAttrChanged = [this](Transport::Accessor const& acsr, Transport::AttrType attribute)
    {
        switch(attribute)
        {
            case Transport::AttrType::startPlayhead:
            case Transport::AttrType::runningPlayhead:
            {
                auto const isPlaying = acsr.getAttr<Transport::AttrType::playback>();
                if(isPlaying == (attribute == Transport::AttrType::runningPlayhead))
                {
                    repaint();
                    if(isShowing())
                    {
                        updateTooltip(getMouseXYRelative());
                    }
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

    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acsr, AttrType attribute)
    {
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
            case AttrType::referenceid:
            {
                repaint();
            }
            break;
        }
    };

    mTransportAccessor.addListener(mTransportListener, NotificationType::synchronous);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::Slice::Plot::~Plot()
{
    mAccessor.removeListener(mListener);
    mTransportAccessor.removeListener(mTransportListener);
    for(auto& trackAcsr : mTrackAccessors.getContents())
    {
        trackAcsr.second.get().getAcsr<Track::AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
        trackAcsr.second.get().getAcsr<Track::AcsrType::binZoom>().removeListener(mZoomListener);
        trackAcsr.second.get().getAcsr<Track::AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
        trackAcsr.second.get().getAcsr<Track::AcsrType::valueZoom>().removeListener(mZoomListener);
        trackAcsr.second.get().removeListener(mTrackListener);
    }
}

void Group::Slice::Plot::paint(juce::Graphics& g)
{
    g.fillAll(mAccessor.getAttr<AttrType::colour>());
    auto const bounds = getLocalBounds();
    auto const time = Track::Slice::getTime(mTransportAccessor);
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    auto const horizontalTrackAcsr = Tools::getVerticalZoomTrackAcsr(mAccessor);
    auto const verticalTrackAcsr = Slice::getVectorTrackAcsr(mAccessor);
    auto const gridColour = findColour(Decorator::ColourIds::normalBorderColourId);
    auto const isSame = [](std::optional<std::reference_wrapper<Track::Accessor>> const& lhs, Track::Accessor const& rhs)
    {
        return lhs.has_value() && std::addressof(lhs.value().get()) == std::addressof(rhs);
    };
    for(auto it = layout.crbegin(); it != layout.crend(); ++it)
    {
        auto const trackAcsr = Tools::getTrackAcsr(mAccessor, *it);
        if(trackAcsr.has_value() && trackAcsr.value().get().getAttr<Track::AttrType::showInGroup>())
        {
            auto const& track = trackAcsr.value().get();
            auto const horizontalGridColour = isSame(horizontalTrackAcsr, track) ? gridColour : juce::Colours::transparentBlack;
            auto const verticalGridColour = isSame(verticalTrackAcsr, track) ? gridColour : juce::Colours::transparentBlack;
            Track::Slice::paint(track, time, g, bounds, horizontalGridColour, verticalGridColour);
        }
    }
}

void Group::Slice::Plot::mouseMove(juce::MouseEvent const& event)
{
    updateTooltip(getLocalPoint(event.eventComponent, juce::Point<int>{event.x, event.y}));
}

void Group::Slice::Plot::mouseEnter(juce::MouseEvent const& event)
{
    updateTooltip(getLocalPoint(event.eventComponent, juce::Point<int>{event.x, event.y}));
}

void Group::Slice::Plot::mouseExit([[maybe_unused]] juce::MouseEvent const& event)
{
    setTooltip("");
}

Group::Slice::Slice(Accessor& accessor, Transport::Accessor& transportAccessor)
: mAccessor(accessor)
, mPlot(accessor, transportAccessor)
, mLayoutNotifier(accessor, [this]()
                  {
                      updateFrame();
                  },
                  {Track::AttrType::identifier, Track::AttrType::description, Track::AttrType::results, Track::AttrType::showInGroup})
{
    addAndMakeVisible(mFrame);
    setSize(560, 220);
}

Group::Slice::~Slice()
{
    mFrame.setTracks(nullptr, nullptr);
}

void Group::Slice::resized()
{
    mFrame.setBounds(getLocalBounds());
}

void Group::Slice::updateFrame()
{
    // The horizontal axis uses the same track as the vertical ruler of the group in the main interface
    auto const horizontalTrackAcsr = Tools::getVerticalZoomTrackAcsr(mAccessor);
    auto const verticalTrackAcsr = getVectorTrackAcsr(mAccessor);
    mFrame.setTracks(horizontalTrackAcsr.has_value() ? std::addressof(horizontalTrackAcsr.value().get()) : nullptr, verticalTrackAcsr.has_value() ? std::addressof(verticalTrackAcsr.value().get()) : nullptr);
}

std::optional<std::reference_wrapper<Track::Accessor>> Group::Slice::getVectorTrackAcsr(Accessor const& accessor)
{
    for(auto const& identifier : accessor.getAttr<AttrType::layout>())
    {
        auto const trackAcsr = Tools::getTrackAcsr(accessor, identifier);
        if(trackAcsr.has_value() && trackAcsr.value().get().getAttr<Track::AttrType::showInGroup>() && Track::Tools::getFrameType(trackAcsr.value().get()) == Track::FrameType::vector)
        {
            return trackAcsr;
        }
    }
    return {};
}

bool Group::Slice::isSupported(Accessor const& accessor)
{
    auto const& layout = accessor.getAttr<AttrType::layout>();
    return std::any_of(layout.cbegin(), layout.cend(), [&](juce::String const& identifier)
                       {
                           auto const trackAcsr = Tools::getTrackAcsr(accessor, identifier);
                           return trackAcsr.has_value() && trackAcsr.value().get().getAttr<Track::AttrType::showInGroup>() && Track::Slice::isSupported(trackAcsr.value().get());
                       });
}

void Group::Slice::Plot::updateContent()
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
    repaint();
}

void Group::Slice::Plot::updateTooltip(juce::Point<int> const& pt)
{
    if(!getLocalBounds().contains(pt))
    {
        setTooltip("");
        return;
    }
    auto const time = Track::Slice::getTime(mTransportAccessor);
    juce::StringArray lines;
    lines.add(juce::translate("Time: TIMESTR").replace("TIMESTR", Format::secondsToString(time)));
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    for(auto const& identifier : layout)
    {
        auto const trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
        if(trackAcsr.has_value() && trackAcsr.value().get().getAttr<Track::AttrType::showInGroup>() && Track::Slice::isSupported(trackAcsr.value().get()))
        {
            lines.add(trackAcsr.value().get().getAttr<Track::AttrType::name>() + ":");
            for(auto const& line : Track::Slice::getTooltip(trackAcsr.value().get(), getLocalBounds(), pt, time))
            {
                lines.add("\t" + line);
            }
        }
    }
    setTooltip(lines.joinIntoString("\n"));
}

ANALYSE_FILE_END
