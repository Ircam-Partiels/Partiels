#include "AnlGroupPlot.h"
#include "../Track/AnlTrackRenderer.h"
#include "../Track/AnlTrackTools.h"
#include "../Track/AnlTrackTooltip.h"

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
    juce::ignoreUnused(mTransportAccessor);

    mTrackListener.onAttrChanged = [this]([[maybe_unused]] Track::Accessor const& acsr, Track::AttrType attribute)
    {
        switch(attribute)
        {
            case Track::AttrType::identifier:
            case Track::AttrType::name:
            case Track::AttrType::file:
            case Track::AttrType::key:
            case Track::AttrType::input:
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
            case Track::AttrType::edit:
            case Track::AttrType::graphics:
            case Track::AttrType::colours:
            case Track::AttrType::font:
            case Track::AttrType::unit:
            case Track::AttrType::channelsLayout:
            case Track::AttrType::showInGroup:
            {
                repaint();
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
            }
            break;
        }
    };

    mGridListener.onAttrChanged = [this]([[maybe_unused]] Zoom::Grid::Accessor const& acsr, [[maybe_unused]] Zoom::Grid::AttrType attribute)
    {
        repaint();
    };

    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acsr, AttrType attribute)
    {
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
    auto const zoomTrack = Tools::getZoomTrackAcsr(mAccessor);
    for(auto it = layout.crbegin(); it != layout.crend(); ++it)
    {
        auto const trackAcsr = Tools::getTrackAcsr(mAccessor, *it);
        if(trackAcsr.has_value() && trackAcsr.value().get().getAttr<Track::AttrType::showInGroup>())
        {
            auto const isSelected = (!zoomTrack.has_value() && it == std::prev(layout.crend())) || std::addressof(trackAcsr.value()) == std::addressof(zoomTrack.value());
            auto const colour = isSelected ? findColour(Decorator::ColourIds::normalBorderColourId) : juce::Colours::transparentBlack;
            Track::Renderer::paint(trackAcsr.value().get(), mTimeZoomAccessor, g, bounds, colour);
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

Group::Plot::Overlay::Overlay(Plot& plot, juce::ApplicationCommandManager& commandManager)
: mPlot(plot)
, mAccessor(mPlot.mAccessor)
, mTimeZoomAccessor(mPlot.mTimeZoomAccessor)
, mTransportAccessor(mPlot.mTransportAccessor)
, mCommandManager(commandManager)
, mNavigationBar(mPlot.mAccessor, mTimeZoomAccessor, mTransportAccessor)
{
    setWantsKeyboardFocus(true);
    addAndMakeVisible(mPlot);
    addAndMakeVisible(mNavigationBar);
    mNavigationBar.addMouseListener(this, true);

    mTimeZoomListener.onAttrChanged = [this]([[maybe_unused]] Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
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
    mNavigationBar.setBounds(bounds);
}

void Group::Plot::Overlay::paint(juce::Graphics& g)
{
    g.fillAll(mAccessor.getAttr<AttrType::colour>());
}

void Group::Plot::Overlay::mouseMove(juce::MouseEvent const& event)
{
    updateTooltip(getLocalPoint(event.eventComponent, juce::Point<int>{event.x, event.y}));
}

void Group::Plot::Overlay::mouseEnter(juce::MouseEvent const& event)
{
    updateTooltip(getLocalPoint(event.eventComponent, juce::Point<int>{event.x, event.y}));
}

void Group::Plot::Overlay::mouseExit([[maybe_unused]] juce::MouseEvent const& event)
{
    setTooltip("");
}

void Group::Plot::Overlay::mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel)
{
    mScrollHelper.mouseWheelMove(wheel, [&](ScrollHelper::Orientation orientation)
                                 {
                                     mScrollOrientation = orientation;
                                     mScrollModifiers = event.mods;
                                 });
    if(mScrollModifiers.isCtrlDown())
    {
        auto const delta = mScrollOrientation == ScrollHelper::Orientation::vertical ? wheel.deltaY : wheel.deltaX;
        if(mScrollModifiers.isShiftDown())
        {
            auto const visibleRange = mAccessor.getAcsr<AcsrType::zoom>().getAttr<Zoom::AttrType::visibleRange>();
            auto const offset = static_cast<double>(delta) * visibleRange.getLength();
            mAccessor.getAcsr<AcsrType::zoom>().setAttr<Zoom::AttrType::visibleRange>(visibleRange + offset, NotificationType::synchronous);
        }
        else
        {
            auto const anchor = Zoom::Tools::getScaledValueFromHeight(mAccessor.getAcsr<AcsrType::zoom>(), *this, event.y);
            Zoom::Tools::zoomIn(mAccessor.getAcsr<AcsrType::zoom>(), -static_cast<double>(delta) / 5.0, anchor, NotificationType::synchronous);
        }
    }
    else
    {
        if((mScrollModifiers.isShiftDown() && mScrollOrientation == ScrollHelper::vertical) || mScrollOrientation == ScrollHelper::horizontal)
        {
            auto const visibleRange = mTimeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>();
            auto const delta = mScrollModifiers.isShiftDown() ? static_cast<double>(wheel.deltaY) : static_cast<double>(wheel.deltaX);
            mTimeZoomAccessor.setAttr<Zoom::AttrType::visibleRange>(visibleRange - delta * visibleRange.getLength(), NotificationType::synchronous);
        }
        else
        {
            auto const isTransportAnchor = Utils::isCommandTicked(mCommandManager, ApplicationCommandIDs::viewTimeZoomAnchorOnPlayhead);
            auto const anchor = isTransportAnchor ? mTransportAccessor.getAttr<Transport::AttrType::startPlayhead>() : Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, event.x);
            auto const amount = static_cast<double>(wheel.deltaY);
            Zoom::Tools::zoomIn(mTimeZoomAccessor, amount, anchor, NotificationType::synchronous);
        }
    }
}

void Group::Plot::Overlay::mouseMagnify(juce::MouseEvent const& event, float magnifyAmount)
{
    auto const amount = static_cast<double>(1.0f - magnifyAmount) / -5.0;
    if(event.mods.isCtrlDown())
    {
        auto const anchor = Zoom::Tools::getScaledValueFromHeight(mAccessor.getAcsr<AcsrType::zoom>(), *this, event.y);
        Zoom::Tools::zoomIn(mAccessor.getAcsr<AcsrType::zoom>(), amount, anchor, NotificationType::synchronous);
    }
    else
    {
        auto const anchor = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, event.x);
        Zoom::Tools::zoomIn(mTimeZoomAccessor, amount, anchor, NotificationType::synchronous);
    }
}

void Group::Plot::Overlay::takeSnapshot()
{
    ComponentSnapshot::takeSnapshot(mPlot, mAccessor.getAttr<AttrType::name>(), juce::Colours::transparentBlack);
}

void Group::Plot::Overlay::updateTooltip(juce::Point<int> const& pt)
{
    if(!getLocalBounds().contains(pt))
    {
        setTooltip("");
        return;
    }
    juce::StringArray lines;
    auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, pt.x);
    lines.add(juce::translate("Time: TIME").replace("TIME", Format::secondsToString(time)));
    auto const zoomTrackAcsr = Tools::getZoomTrackAcsr(mAccessor);
    if(zoomTrackAcsr.has_value())
    {
        lines.add(juce::translate("Mouse: VALUE").replace("VALUE", Track::Tools::getZoomTootip(zoomTrackAcsr->get(), *this, pt.y)));
    }
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    for(auto const& identifier : layout)
    {
        auto trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
        if(trackAcsr.has_value() && trackAcsr->get().getAttr<Track::AttrType::showInGroup>())
        {
            lines.add(trackAcsr->get().getAttr<Track::AttrType::name>() + ":");
            for(auto const& line : Track::Tools::getValueTootip(trackAcsr->get(), mTimeZoomAccessor, *this, pt.y, time))
            {
                lines.addArray("\t" + line);
            }
        }
    }
    setTooltip(lines.joinIntoString("\n"));
}

ANALYSE_FILE_END
