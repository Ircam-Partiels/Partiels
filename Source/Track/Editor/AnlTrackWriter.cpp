#include "AnlTrackWriter.h"
#include "../AnlTrackTools.h"
#include "../Result/AnlTrackResultModifier.h"

ANALYSE_FILE_BEGIN

Track::Writer::Writer(Director& director, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor)
: mDirector(director)
, mAccessor(mDirector.getAccessor())
, mTimeZoomAccessor(timeZoomAccessor)
, mTransportAccessor(transportAccessor)
{
}

bool Track::Writer::isPerformingAction() const
{
    return isEnabled() && mActionMode != ActionMode::none;
}

void Track::Writer::mouseMove(juce::MouseEvent const& event)
{
    updateActionMode(event);
}

void Track::Writer::mouseEnter(juce::MouseEvent const& event)
{
    updateActionMode(event);
}

void Track::Writer::mouseDown(juce::MouseEvent const& event)
{
    updateActionMode(event);
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

            auto const getDefaultExtra = [this]()
            {
                std::vector<float> extras;
                auto const& extraOutputs = mAccessor.getAttr<AttrType::description>().extraOutputs;
                extras.reserve(extraOutputs.size());
                for(auto index = 0_z; index < extraOutputs.size(); ++index)
                {
                    auto const extraRange = Tools::getExtraRange(mAccessor, index);
                    extras.push_back(extraRange.has_value() ? static_cast<float>(extraRange->getEnd()) : 1.0f);
                }
                return extras;
            };

            auto const frameType = Tools::getFrameType(mAccessor);
            if(frameType.has_value())
            {
                switch(frameType.value())
                {
                    case Track::FrameType::label:
                    {
                        auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, event.x);
                        mCurrentEdition.channel = std::get<0_z>(channel.value());
                        mCurrentEdition.data = std::vector<Result::Data::Marker>{{time, 0.0, "", getDefaultExtra()}};
                        mAccessor.setAttr<AttrType::edit>(mCurrentEdition, NotificationType::synchronous);
                        break;
                    }
                    case Track::FrameType::value:
                    {
                        auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, event.x);
                        auto const& valueZoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
                        auto const range = valueZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
                        auto const top = static_cast<float>(std::get<1_z>(*channel).getStart());
                        auto const bottom = static_cast<float>(std::get<1_z>(*channel).getEnd());
                        auto value = Tools::pixelToValue(static_cast<float>(event.y), range, {0.0f, top, 1.0f, bottom - top});
                        auto const isLog = mAccessor.getAttr<AttrType::zoomLogScale>() && Tools::hasVerticalZoomInHertz(mAccessor);
                        if(isLog)
                        {
                            auto const nyquist = mAccessor.getAttr<AttrType::sampleRate>() / 2.0;
                            auto const scaleRatio = static_cast<float>(std::max(Tools::getMidiFromHertz(nyquist), 1.0) / nyquist);
                            value = Tools::getHertzFromMidi(value * scaleRatio);
                        }
                        mCurrentEdition.channel = std::get<0_z>(channel.value());
                        mCurrentEdition.data = std::vector<Result::Data::Point>{{time, 0.0, value, getDefaultExtra()}};
                        mMouseDownTime = time;
                        break;
                    }
                    case Track::FrameType::vector:
                        break;
                }
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

void Track::Writer::mouseDrag(juce::MouseEvent const& event)
{
    switch(mActionMode)
    {
        case ActionMode::none:
            break;
        case ActionMode::create:
        {
            auto const frameType = Tools::getFrameType(mAccessor);
            if(frameType.has_value())
            {
                switch(frameType.value())
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
                        auto value = Tools::pixelToValue(static_cast<float>(event.y), range, {0.0f, top, 1.0f, bottom - top});
                        auto const isLog = mAccessor.getAttr<AttrType::zoomLogScale>() && Tools::hasVerticalZoomInHertz(mAccessor);
                        if(isLog)
                        {
                            auto const nyquist = mAccessor.getAttr<AttrType::sampleRate>() / 2.0;
                            auto const scaleRatio = static_cast<float>(std::max(Tools::getMidiFromHertz(nyquist), 1.0) / nyquist);
                            value = Tools::getHertzFromMidi(value * scaleRatio);
                        }

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
        }
        break;
        case ActionMode::move:
        {
            auto const frameType = Tools::getFrameType(mAccessor);
            if(frameType.has_value())
            {
                switch(frameType.value())
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
        }
        break;
    }
}

void Track::Writer::mouseUp(juce::MouseEvent const& event)
{
    auto& undoManager = mDirector.getUndoManager();
    switch(mActionMode)
    {
        case ActionMode::none:
            break;
        case ActionMode::create:
        {
            auto const frameType = Tools::getFrameType(mAccessor);
            if(frameType.has_value())
            {
                switch(frameType.value())
                {
                    case Track::FrameType::label:
                    {
                        mouseDrag(event);
                        auto const time = Result::Modifier::getTimeRange(mCurrentEdition.data).getStart();
                        undoManager.beginNewTransaction(juce::translate("Add Marker"));
                        undoManager.perform(std::make_unique<Result::Modifier::ActionPaste>(mDirector.getSafeAccessorFn(), mCurrentEdition.channel, mCurrentEdition.data, time).release());
                    }
                    break;
                    case Track::FrameType::value:
                    {
                        mouseDrag(event);
                        auto const time = Result::Modifier::getTimeRange(mCurrentEdition.data).getStart();
                        undoManager.beginNewTransaction(juce::translate("Add Point"));
                        undoManager.perform(std::make_unique<Result::Modifier::ActionPaste>(mDirector.getSafeAccessorFn(), mCurrentEdition.channel, mCurrentEdition.data, time).release());
                    }
                    break;
                    case Track::FrameType::vector:
                        break;
                }
            }
        }
        break;
        case ActionMode::move:
        {
            auto const frameType = Tools::getFrameType(mAccessor);
            if(frameType.has_value())
            {
                switch(frameType.value())
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
                            undoManager.beginNewTransaction(juce::translate("Move Marker"));
                            undoManager.perform(std::make_unique<Result::Modifier::ActionErase>(mDirector.getSafeAccessorFn(), edition.channel, edition.range).release());
                            undoManager.perform(std::make_unique<Result::Modifier::ActionPaste>(mDirector.getSafeAccessorFn(), edition.channel, edition.data, time).release());
                        }
                    }
                    break;
                    case Track::FrameType::value:
                    case Track::FrameType::vector:
                        break;
                }
            }
        }
        break;
    }
    updateActionMode(event);
    mMouseWasDragged = false;
    mCurrentEdition = {};
    mAccessor.setAttr<AttrType::edit>(Edition{}, NotificationType::synchronous);
}

void Track::Writer::mouseDoubleClick(juce::MouseEvent const& event)
{
    if(event.mods.isAltDown() || event.mods.isCommandDown())
    {
        return;
    }
    auto const frameType = Tools::getFrameType(mAccessor);
    if(frameType.has_value())
    {
        switch(frameType.value())
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
}

void Track::Writer::updateActionMode(juce::MouseEvent const& event)
{
    auto const point = event.getPosition();
    auto const& modifiers = event.mods;
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

        auto const frameType = Tools::getFrameType(mAccessor);
        if(frameType.has_value())
        {
            switch(frameType.value())
            {
                case FrameType::label:
                {
                    auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, point.x);
                    auto const epsilon = 2.0 / static_cast<double>(getWidth()) * mTimeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>().getLength();
                    if(Result::Modifier::matchFrame(mAccessor, std::get<0>(channel.value()), time, epsilon))
                    {
                        return ActionMode::move;
                    }
                    break;
                }
                case FrameType::value: // Could be implemented for points
                case FrameType::vector:
                    break;
            }
        }
        return ActionMode::none;
    };

    auto const currentAction = getCurrentAction();
    if(std::exchange(mActionMode, currentAction) != currentAction)
    {
        switch(currentAction)
        {
            case ActionMode::none:
                setMouseCursor(juce::MouseCursor::StandardCursorType::IBeamCursor);
                break;
            case ActionMode::create:
                setMouseCursor(juce::MouseCursor::StandardCursorType::CrosshairCursor);
                break;
            case ActionMode::move:
                setMouseCursor(juce::MouseCursor::StandardCursorType::LeftRightResizeCursor);
                break;
        }
        if(onModeUpdated != nullptr)
        {
            onModeUpdated();
        }
    }
}

ANALYSE_FILE_END
