#include "AnlTrackPlot.h"
#include "AnlTrackRenderer.h"
#include "AnlTrackTools.h"
#include "Result/AnlTrackResultModifier.h"

ANALYSE_FILE_BEGIN

Track::Plot::Plot(Director& director, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor)
: mDirector(director)
, mTimeZoomAccessor(timeZoomAccessor)
, mTransportAccessor(transportAccessor)
{
    setInterceptsMouseClicks(false, false);
    mListener.onAttrChanged = [=, this]([[maybe_unused]] Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::file:
            case AttrType::name:
            case AttrType::key:
            case AttrType::input:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::focused:
            case AttrType::showInGroup:
                break;
            case AttrType::description:
            case AttrType::grid:
            case AttrType::results:
            case AttrType::edit:
            case AttrType::graphics:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::unit:
            case AttrType::channelsLayout:
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

    setCachedComponentImage(new LowResCachedComponentImage(*this));
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::valueZoom>().addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::binZoom>().addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
    mTimeZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
}

Track::Plot::~Plot()
{
    mTimeZoomAccessor.removeListener(mZoomListener);
    mAccessor.getAcsr<AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
    mAccessor.getAcsr<AcsrType::binZoom>().removeListener(mZoomListener);
    mAccessor.getAcsr<AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
    mAccessor.getAcsr<AcsrType::valueZoom>().removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Track::Plot::paint(juce::Graphics& g)
{
    Renderer::paint(mAccessor, mTimeZoomAccessor, g, getLocalBounds(), findColour(Decorator::ColourIds::normalBorderColourId));
}

Track::Plot::Overlay::Overlay(Plot& plot)
: mPlot(plot)
, mAccessor(mPlot.mAccessor)
, mTimeZoomAccessor(mPlot.mTimeZoomAccessor)
, mSelectionBar(mPlot.mAccessor, mTimeZoomAccessor, mPlot.mTransportAccessor)
{
    addAndMakeVisible(mPlot);
    addAndMakeVisible(mSelectionBar);
    mSelectionBar.addMouseListener(this, true);
    setInterceptsMouseClicks(true, true);

    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::key:
            case AttrType::input:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::graphics:
            case AttrType::font:
            case AttrType::warnings:
            case AttrType::grid:
            case AttrType::processing:
            case AttrType::results:
            case AttrType::edit:
            case AttrType::focused:
            case AttrType::showInGroup:
                break;
            case AttrType::colours:
            {
                repaint();
            }
            break;
            case AttrType::channelsLayout:
            {
                updateTooltip(getMouseXYRelative());
            }
            break;
            case AttrType::file:
            case AttrType::description:
            case AttrType::name:
            case AttrType::unit:
            {
                juce::SettableTooltipClient::setTooltip(Tools::getInfoTooltip(acsr));
            }
            break;
        }
    };

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

    mTimeZoomAccessor.addListener(mTimeZoomListener, NotificationType::synchronous);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::Plot::Overlay::~Overlay()
{
    mAccessor.removeListener(mListener);
    mTimeZoomAccessor.removeListener(mTimeZoomListener);
}

void Track::Plot::Overlay::resized()
{
    auto const bounds = getLocalBounds();
    mPlot.setBounds(bounds);
    mSelectionBar.setBounds(bounds);
}

void Track::Plot::Overlay::paint(juce::Graphics& g)
{
    g.fillAll(mAccessor.getAttr<AttrType::colours>().background);
}

void Track::Plot::Overlay::mouseMove(juce::MouseEvent const& event)
{
    updateTooltip(getLocalPoint(event.eventComponent, juce::Point<int>{event.x, event.y}));
    updateMode(event);
}

void Track::Plot::Overlay::mouseEnter(juce::MouseEvent const& event)
{
    updateTooltip(getLocalPoint(event.eventComponent, juce::Point<int>{event.x, event.y}));
    updateMode(event);
}

void Track::Plot::Overlay::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    Tooltip::BubbleClient::setTooltip("");
}

void Track::Plot::Overlay::mouseDown(juce::MouseEvent const& event)
{
    updateMode(event);
    if(event.mods.isCommandDown())
    {
        using ChannelData = Result::ChannelData;
        auto const getChannel = [&]() -> std::optional<std::tuple<size_t, juce::Range<int>>>
        {
            auto const verticalRanges = Tools::getChannelVerticalRanges(mAccessor, getLocalBounds());
            for(auto const& verticalRange : verticalRanges)
            {
                if(verticalRange.second.contains(event.y))
                {
                    return std::make_tuple(verticalRange.first, verticalRange.second);
                }
            }
            return {};
        };
        auto const channel = getChannel();
        if(!channel.has_value())
        {
            return;
        }

        auto const addAction = [&](ChannelData const& data)
        {
            auto& director = mPlot.mDirector;
            auto& undoManager = director.getUndoManager();
            undoManager.beginNewTransaction(juce::translate("Add Frame"));
            auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, event.x);
            undoManager.perform(std::make_unique<Result::Modifier::ActionPaste>(director.getSafeAccessorFn(), std::get<0_z>(channel.value()), data, time).release());
        };

        switch(Tools::getFrameType(mAccessor))
        {
            case Track::FrameType::label:
            {
                addAction(std::vector<Result::Data::Marker>{{Result::Data::Marker{0.0, 0.0, ""}}});
            }
            break;
            case Track::FrameType::value:
            {
                auto const& valueZoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
                auto const range = valueZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
                auto const top = static_cast<float>(std::get<1_z>(*channel).getStart());
                auto const bottom = static_cast<float>(std::get<1_z>(*channel).getEnd());
                auto const value = Tools::pixelToValue(static_cast<float>(event.y), range, {0.0f, top, 1.0f, bottom - top});
                addAction(std::vector<Result::Data::Point>{{Result::Data::Point{0.0, 0.0, value}}});
            }
            case Track::FrameType::vector:
                break;
        }
    }
    else if(event.mods.isAltDown())
    {
        takeSnapshot(mPlot, mAccessor.getAttr<AttrType::name>(), mAccessor.getAttr<AttrType::colours>().background);
    }
}

void Track::Plot::Overlay::mouseDrag(juce::MouseEvent const& event)
{
    updateMode(event);
}

void Track::Plot::Overlay::mouseUp(juce::MouseEvent const& event)
{
    updateMode(event);
}

void Track::Plot::Overlay::updateMode(juce::MouseEvent const& event)
{
    auto const isCommandOrAltDown = event.mods.isCommandDown() || event.mods.isAltDown();
    mSelectionBar.setInterceptsMouseClicks(!isCommandOrAltDown, !isCommandOrAltDown);
    if(!event.mods.isCommandDown() && event.mods.isAltDown() && !mSnapshotMode)
    {
        mSnapshotMode = true;
        showCameraCursor(true);
    }
    else if(!event.mods.isAltDown() && mSnapshotMode)
    {
        mSnapshotMode = false;
        showCameraCursor(false);
    }
}

void Track::Plot::Overlay::updateTooltip(juce::Point<int> const& pt)
{
    if(!getLocalBounds().contains(pt))
    {
        Tooltip::BubbleClient::setTooltip("");
        return;
    }

    auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, pt.x);
    auto const tip = Tools::getValueTootip(mAccessor, mTimeZoomAccessor, *this, pt.y, time, true);
    Tooltip::BubbleClient::setTooltip(Format::secondsToString(time) + ": " + (tip.isEmpty() ? "-" : tip));
}

ANALYSE_FILE_END
