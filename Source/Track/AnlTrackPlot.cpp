#include "AnlTrackPlot.h"
#include "AnlTrackRenderer.h"
#include "AnlTrackTools.h"
#include "AnlTrackTooltip.h"
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
            case AttrType::zoomValueMode:
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
            case AttrType::extraThresholds:
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

Track::Plot::Overlay::Overlay(Plot& plot, juce::ApplicationCommandManager& commandManager)
: mPlot(plot)
, mAccessor(mPlot.mAccessor)
, mTimeZoomAccessor(mPlot.mTimeZoomAccessor)
, mTransportAccessor(mPlot.mTransportAccessor)
, mCommandManager(commandManager)
, mSelectionBar(mPlot.mAccessor, mTimeZoomAccessor, mTransportAccessor)
{
    setWantsKeyboardFocus(true);
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
            case AttrType::zoomValueMode:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::extraThresholds:
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
    auto const localPoint = getLocalPoint(event.eventComponent, juce::Point<int>{event.x, event.y});
    updateTooltip(localPoint);
    updateActionMode(localPoint, event.mods);
}

void Track::Plot::Overlay::mouseEnter(juce::MouseEvent const& event)
{
    mouseMove(event);
}

void Track::Plot::Overlay::mouseExit([[maybe_unused]] juce::MouseEvent const& event)
{
    Tooltip::BubbleClient::setTooltip("");
}

void Track::Plot::Overlay::mouseDown(juce::MouseEvent const& event)
{
    mouseMove(event);
    switch(mActionMode)
    {
        case ActionMode::none:
            break;
        case ActionMode::create:
        {
            auto const channel = Tools::getChannelVerticalRange(mAccessor, getLocalBounds(), event.y, false);
            if(!channel.has_value())
            {
                break;
            }
            switch(Tools::getFrameType(mAccessor))
            {
                case Track::FrameType::label:
                {
                    auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, event.x);
                    mCurrentEdition.channel = std::get<0_z>(channel.value());
                    std::vector<float> extras;
                    auto const& extraOutputs = mAccessor.getAttr<AttrType::description>().extraOutputs;
                    for(auto index = 0_z; index < extraOutputs.size(); ++index)
                    {
                        auto const extraRange = Tools::getExtraRange(mAccessor, index);
                        extras.push_back(extraRange.has_value() ? static_cast<float>(extraRange->getEnd()) : 1.0f);
                    }
                    mCurrentEdition.data = std::vector<Result::Data::Marker>{{time, 0.0, "", extras}};
                    mAccessor.setAttr<AttrType::edit>(mCurrentEdition, NotificationType::synchronous);
                }
                break;
                case Track::FrameType::value:
                {
                    auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, event.x);
                    auto const& valueZoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
                    auto const range = valueZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
                    auto const top = static_cast<float>(std::get<1_z>(*channel).getStart());
                    auto const bottom = static_cast<float>(std::get<1_z>(*channel).getEnd());
                    auto const value = Tools::pixelToValue(static_cast<float>(event.y), range, {0.0f, top, 1.0f, bottom - top});
                    mCurrentEdition.channel = std::get<0_z>(channel.value());
                    std::vector<float> extras;
                    auto const& extraOutputs = mAccessor.getAttr<AttrType::description>().extraOutputs;
                    for(auto index = 0_z; index < extraOutputs.size(); ++index)
                    {
                        auto const extraRange = Tools::getExtraRange(mAccessor, index);
                        extras.push_back(extraRange.has_value() ? static_cast<float>(extraRange->getEnd()) : 1.0f);
                    }
                    mCurrentEdition.data = std::vector<Result::Data::Point>{{time, 0.0, value, extras}};
                    mMouseDownTime = time;
                }
                break;
                case Track::FrameType::vector:
                    break;
            }
        }
        break;
        case ActionMode::move:
        {
            auto const channel = Tools::getChannelVerticalRange(mAccessor, getLocalBounds(), event.y, false);
            if(!channel.has_value())
            {
                break;
            }
            auto const& results = mAccessor.getAttr<AttrType::results>();
            auto const access = results.getReadAccess();
            if(!static_cast<bool>(access))
            {
                break;
            }
            auto const markers = results.getMarkers();
            auto const channelIndex = std::get<0_z>(channel.value());
            if(markers == nullptr || markers->size() <= channelIndex || markers->at(channelIndex).empty())
            {
                break;
            }

            auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, event.x);
            auto const epsilon = 2.0 / static_cast<double>(getWidth()) * mTimeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>().getLength();
            auto const& markerChannel = markers->at(channelIndex);
            auto const it = std::lower_bound(markerChannel.cbegin(), markerChannel.cend(), time - epsilon, Result::lower_cmp<Results::Marker>);
            if(it == markerChannel.cend() || std::get<0_z>(*it) > time + epsilon)
            {
                break;
            }
            auto const next = std::next(it);
            mCurrentEdition.channel = channelIndex;
            mCurrentEdition.data = std::vector<Result::Data::Marker>{{time, std::get<1_z>(*it), std::get<2_z>(*it), std::get<3_z>(*it)}};
            mCurrentEdition.range.setStart(std::get<0_z>(*it));
            mCurrentEdition.range.setEnd(next == markerChannel.cend() ? std::numeric_limits<double>::max() : std::get<0_z>(*next));
        }
        break;
    }
}

void Track::Plot::Overlay::mouseDrag(juce::MouseEvent const& event)
{
    switch(mActionMode)
    {
        case ActionMode::none:
            break;
        case ActionMode::create:
        {
            switch(Tools::getFrameType(mAccessor))
            {
                case Track::FrameType::label:
                {
                    auto* markersData = std::get_if<std::vector<Result::Data::Marker>>(&mCurrentEdition.data);
                    MiscWeakAssert(markersData != nullptr && !markersData->empty());
                    if(markersData == nullptr || markersData->empty())
                    {
                        return;
                    }
                    std::get<0_z>(markersData->front()) = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, event.x);
                    mAccessor.setAttr<AttrType::edit>(mCurrentEdition, NotificationType::synchronous);
                }
                break;
                case Track::FrameType::value:
                {
                    auto* pointsData = std::get_if<std::vector<Result::Data::Point>>(&mCurrentEdition.data);
                    MiscWeakAssert(pointsData != nullptr && !pointsData->empty());
                    if(pointsData == nullptr || pointsData->empty())
                    {
                        return;
                    }
                    auto const channels = Tools::getChannelVerticalRanges(mAccessor, getLocalBounds());
                    if(channels.empty() || channels.count(mCurrentEdition.channel) == 0_z)
                    {
                        return;
                    }
                    auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, event.x);
                    auto const& valueZoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
                    auto const range = valueZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
                    auto const top = static_cast<float>(channels.at(mCurrentEdition.channel).getStart());
                    auto const bottom = static_cast<float>(channels.at(mCurrentEdition.channel).getEnd());
                    auto const value = Tools::pixelToValue(static_cast<float>(event.y), range, {0.0f, top, 1.0f, bottom - top});
                    auto const epsilon = 2.0 / static_cast<double>(getWidth()) * mTimeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>().getLength();

                    // remove points before time is necessary
                    auto const minTime = std::min(mMouseDownTime, time);
                    auto const startIt = std::lower_bound(pointsData->begin(), pointsData->end(), minTime, Result::lower_cmp<Result::Data::Point>);
                    if(startIt != pointsData->begin())
                    {
                        pointsData->erase(pointsData->begin(), std::prev(startIt));
                    }
                    MiscWeakAssert(!pointsData->empty());

                    // remove points after time is necessary
                    auto const maxTime = std::max(mMouseDownTime, time);
                    auto const endIt = std::upper_bound(pointsData->begin(), pointsData->end(), maxTime, Result::upper_cmp<Result::Data::Point>);
                    if(endIt != pointsData->end())
                    {
                        pointsData->erase(endIt, pointsData->end());
                    }
                    MiscWeakAssert(!pointsData->empty());

                    if(time > std::get<0_z>(pointsData->back()) + epsilon)
                    {
                        pointsData->push_back({time, 0.0, value, std::vector<float>{}});
                    }
                    else if(std::abs(time - std::get<0_z>(pointsData->back())) < epsilon)
                    {
                        std::get<2_z>(pointsData->back()) = value;
                    }
                    else if(time < std::get<0_z>(pointsData->front()) - epsilon)
                    {
                        pointsData->insert(pointsData->begin(), {time, 0.0, value, std::vector<float>{}});
                    }
                    else if(std::abs(time - std::get<0_z>(pointsData->front())) < epsilon)
                    {
                        std::get<2_z>(pointsData->front()) = value;
                    }
                    mAccessor.setAttr<AttrType::edit>(mCurrentEdition, NotificationType::synchronous);
                }
                break;
                case Track::FrameType::vector:
                    break;
            }
        }
        break;
        case ActionMode::move:
        {
            switch(Tools::getFrameType(mAccessor))
            {
                case Track::FrameType::label:
                {
                    if(Result::Modifier::isEmpty(mCurrentEdition.data))
                    {
                        return;
                    }
                    auto* markersData = std::get_if<std::vector<Result::Data::Marker>>(&mCurrentEdition.data);
                    MiscWeakAssert(markersData != nullptr);
                    if(markersData == nullptr && !markersData->empty())
                    {
                        return;
                    }
                    auto const epsilon = 2.0 / static_cast<double>(getWidth()) * mTimeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>().getLength();
                    mMouseWasDragged = mMouseWasDragged || static_cast<double>(std::abs(event.getDistanceFromDragStartX())) > epsilon;
                    if(mMouseWasDragged)
                    {
                        std::get<0_z>(markersData->front()) = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, event.x);
                        mAccessor.setAttr<AttrType::edit>(mCurrentEdition, NotificationType::synchronous);
                    }
                }
                break;
                case Track::FrameType::value:
                case Track::FrameType::vector:
                    break;
            }
        }
        break;
    }
}

void Track::Plot::Overlay::mouseUp(juce::MouseEvent const& event)
{
    switch(mActionMode)
    {
        case ActionMode::none:
            break;
        case ActionMode::create:
        {
            switch(Tools::getFrameType(mAccessor))
            {
                case Track::FrameType::label:
                {
                    mouseDrag(event);
                    auto const time = Result::Modifier::getTimeRange(mCurrentEdition.data).getStart();
                    mPlot.mDirector.getUndoManager().beginNewTransaction(juce::translate("Add Marker"));
                    mPlot.mDirector.getUndoManager().perform(std::make_unique<Result::Modifier::ActionPaste>(mPlot.mDirector.getSafeAccessorFn(), mCurrentEdition.channel, mCurrentEdition.data, time).release());
                }
                break;
                case Track::FrameType::value:
                {
                    mouseDrag(event);
                    auto const time = Result::Modifier::getTimeRange(mCurrentEdition.data).getStart();
                    mPlot.mDirector.getUndoManager().beginNewTransaction(juce::translate("Add Point"));
                    mPlot.mDirector.getUndoManager().perform(std::make_unique<Result::Modifier::ActionPaste>(mPlot.mDirector.getSafeAccessorFn(), mCurrentEdition.channel, mCurrentEdition.data, time).release());
                }
                break;
                case Track::FrameType::vector:
                    break;
            }
        }
        break;
        case ActionMode::move:
        {
            switch(Tools::getFrameType(mAccessor))
            {
                case Track::FrameType::label:
                {
                    mouseDrag(event);
                    if(Result::Modifier::isEmpty(mCurrentEdition.data))
                    {
                        break;
                    }
                    auto const epsilon = 2.0 / static_cast<double>(getWidth()) * mTimeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>().getLength();
                    if(mMouseWasDragged || static_cast<double>(std::abs(event.getDistanceFromDragStartX())) > epsilon)
                    {
                        auto const time = Result::Modifier::getTimeRange(mCurrentEdition.data).getStart();
                        auto const& edition = mAccessor.getAttr<AttrType::edit>();
                        mPlot.mDirector.getUndoManager().beginNewTransaction(juce::translate("Move Marker"));
                        mPlot.mDirector.getUndoManager().perform(std::make_unique<Result::Modifier::ActionErase>(mPlot.mDirector.getSafeAccessorFn(), edition.channel, edition.range).release());
                        mPlot.mDirector.getUndoManager().perform(std::make_unique<Result::Modifier::ActionPaste>(mPlot.mDirector.getSafeAccessorFn(), edition.channel, edition.data, time).release());
                    }
                }
                break;
                case Track::FrameType::value:
                case Track::FrameType::vector:
                    break;
            }
        }
        break;
    }
    mouseMove(event);
    mMouseWasDragged = false;
    mCurrentEdition = {};
    mAccessor.setAttr<AttrType::edit>(Edition{}, NotificationType::synchronous);
}

void Track::Plot::Overlay::mouseDoubleClick(juce::MouseEvent const& event)
{
    if(event.mods.isAltDown() || event.mods.isCommandDown())
    {
        return;
    }
    switch(Tools::getFrameType(mAccessor))
    {
        case FrameType::label:
        {
            auto const channel = Tools::getChannelVerticalRange(mAccessor, getLocalBounds(), event.y, false);
            if(!channel.has_value())
            {
                return;
            }
            auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, event.x);
            mTransportAccessor.setAttr<Transport::AttrType::startPlayhead>(time, NotificationType::synchronous);
            auto const epsilon = 2.0 / static_cast<double>(getWidth()) * mTimeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>().getLength();
            if(!Result::Modifier::matchFrame(mAccessor, std::get<0>(channel.value()), time, epsilon))
            {
                return;
            }

            auto var = std::make_unique<juce::DynamicObject>();
            if(var != nullptr)
            {
                var->setProperty("x", event.getScreenX());
                var->setProperty("y", event.getScreenY() - 40);
                mAccessor.sendSignal(SignalType::showTable, var.release(), NotificationType::synchronous);
            }
        }
        break;
        case FrameType::value:
            // Could be implemented for points
        case FrameType::vector:
            break;
    }
}

void Track::Plot::Overlay::mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel)
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
            switch(Tools::getFrameType(mAccessor))
            {
                case Track::FrameType::label:
                    break;
                case Track::FrameType::value:
                {
                    auto const visibleRange = mAccessor.getAcsr<AcsrType::valueZoom>().getAttr<Zoom::AttrType::visibleRange>();
                    auto const offset = static_cast<double>(delta) * visibleRange.getLength();
                    mAccessor.getAcsr<AcsrType::valueZoom>().setAttr<Zoom::AttrType::visibleRange>(visibleRange + offset, NotificationType::synchronous);
                }
                break;
                case Track::FrameType::vector:
                {
                    auto const visibleRange = mAccessor.getAcsr<AcsrType::binZoom>().getAttr<Zoom::AttrType::visibleRange>();
                    auto const offset = static_cast<double>(delta) * visibleRange.getLength();
                    mAccessor.getAcsr<AcsrType::binZoom>().setAttr<Zoom::AttrType::visibleRange>(visibleRange + offset, NotificationType::synchronous);
                }
                break;
            }
        }
        else
        {
            switch(Tools::getFrameType(mAccessor))
            {
                case Track::FrameType::label:
                    break;
                case Track::FrameType::value:
                {
                    auto const anchor = Zoom::Tools::getScaledValueFromWidth(mAccessor.getAcsr<AcsrType::valueZoom>(), *this, event.y);
                    Zoom::Tools::zoomIn(mAccessor.getAcsr<AcsrType::valueZoom>(), static_cast<double>(delta), anchor, NotificationType::synchronous);
                }
                break;
                case Track::FrameType::vector:
                {
                    auto const anchor = Zoom::Tools::getScaledValueFromWidth(mAccessor.getAcsr<AcsrType::binZoom>(), *this, event.y);
                    Zoom::Tools::zoomIn(mAccessor.getAcsr<AcsrType::binZoom>(), static_cast<double>(delta), anchor, NotificationType::synchronous);
                }
                break;
            }
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

void Track::Plot::Overlay::mouseMagnify(juce::MouseEvent const& event, float magnifyAmount)
{
    auto const amount = static_cast<double>(1.0f - magnifyAmount) / -5.0;
    if(event.mods.isCtrlDown())
    {
        switch(Tools::getFrameType(mAccessor))
        {
            case Track::FrameType::label:
                break;
            case Track::FrameType::value:
            {
                auto const anchor = Zoom::Tools::getScaledValueFromWidth(mAccessor.getAcsr<AcsrType::valueZoom>(), *this, event.y);
                Zoom::Tools::zoomIn(mAccessor.getAcsr<AcsrType::valueZoom>(), amount, anchor, NotificationType::synchronous);
            }
            break;
            case Track::FrameType::vector:
            {
                auto const anchor = Zoom::Tools::getScaledValueFromWidth(mAccessor.getAcsr<AcsrType::binZoom>(), *this, event.y);
                Zoom::Tools::zoomIn(mAccessor.getAcsr<AcsrType::binZoom>(), amount, anchor, NotificationType::synchronous);
            }
            break;
        }
    }
    else
    {
        auto const anchor = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, event.x);
        Zoom::Tools::zoomIn(mTimeZoomAccessor, amount, anchor, NotificationType::synchronous);
    }
}

void Track::Plot::Overlay::modifierKeysChanged(juce::ModifierKeys const& modifiers)
{
    updateActionMode(getMouseXYRelative(), modifiers);
}

void Track::Plot::Overlay::takeSnapshot()
{
    ComponentSnapshot::takeSnapshot(mPlot, mAccessor.getAttr<AttrType::name>(), mAccessor.getAttr<AttrType::colours>().background);
}

void Track::Plot::Overlay::updateActionMode(juce::Point<int> const& point, juce::ModifierKeys const& modifiers)
{
    auto const getCurrentAction = [&]()
    {
        if(modifiers.isCommandDown())
        {
            return ActionMode::create;
        }
        auto const channel = Tools::getChannelVerticalRange(mAccessor, getLocalBounds(), point.y, false);
        if(!channel.has_value())
        {
            return ActionMode::none;
        }

        switch(Tools::getFrameType(mAccessor))
        {
            case FrameType::label:
            {
                auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, point.x);
                auto const epsilon = 2.0 / static_cast<double>(getWidth()) * mTimeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>().getLength();
                if(Result::Modifier::matchFrame(mAccessor, std::get<0>(channel.value()), time, epsilon))
                {
                    return ActionMode::move;
                }
            }
            break;
            case FrameType::value:
                // Could be implemented for points
            case FrameType::vector:
                break;
        }
        return ActionMode::none;
    };

    auto const currentAction = getCurrentAction();
    if(std::exchange(mActionMode, currentAction) != currentAction)
    {
        switch(currentAction)
        {
            case ActionMode::none:
                setMouseCursor(juce::MouseCursor::StandardCursorType::NormalCursor);
                mSelectionBar.setInterceptsMouseClicks(true, true);
                break;
            case ActionMode::create:
                setMouseCursor(juce::MouseCursor::StandardCursorType::CrosshairCursor);
                mSelectionBar.setInterceptsMouseClicks(false, false);
                break;
            case ActionMode::move:
                setMouseCursor(juce::MouseCursor::StandardCursorType::LeftRightResizeCursor);
                mSelectionBar.setInterceptsMouseClicks(false, false);
                break;
        }
    }
}

void Track::Plot::Overlay::updateTooltip(juce::Point<int> const& pt)
{
    if(!getLocalBounds().contains(pt))
    {
        Tooltip::BubbleClient::setTooltip("");
        return;
    }
    juce::StringArray lines;
    auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, pt.x);
    lines.add(juce::translate("Time: TIME").replace("TIME", Format::secondsToString(time)));
    lines.add(juce::translate("Mouse: VALUE").replace("VALUE", Tools::getZoomTootip(mAccessor, *this, pt.y)));
    lines.addArray(Tools::getValueTootip(mAccessor, mTimeZoomAccessor, *this, pt.y, time));
    Tooltip::BubbleClient::setTooltip(lines.joinIntoString("\n"));
}

ANALYSE_FILE_END
