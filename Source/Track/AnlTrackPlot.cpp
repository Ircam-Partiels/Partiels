#include "AnlTrackPlot.h"
#include "AnlTrackTools.h"
#include "Result/AnlTrackResultModifier.h"

ANALYSE_FILE_BEGIN

void Track::Plot::PathArrangement::stopLine()
{
    if(!std::exchange(mShouldStartNewPath, true))
    {
        mPath.lineTo(mLastPoint);
    }
}

void Track::Plot::PathArrangement::addLine(float x1, float x2, float y)
{
    if(std::exchange(mShouldStartNewPath, false))
    {
        mPath.startNewSubPath(x1, y);
        mLastPoint = {x2, y};
    }
    else if(std::abs(y - mLastPoint.y) > std::numeric_limits<float>::epsilon())
    {
        mPath.lineTo(mLastPoint);
        mPath.lineTo(x1, y);
        mLastPoint = {x2, y};
    }
    else
    {
        mLastPoint = {x2, y};
    }
}

void Track::Plot::PathArrangement::draw(juce::Graphics& g, juce::Colour const& foreground, juce::Colour const& shadow)
{
    stopLine();
    if(mPath.isEmpty())
    {
        return;
    }

    juce::PathStrokeType const pathStrokeType(1.0f);
    pathStrokeType.createStrokedPath(mPath, mPath);

    if(!shadow.isTransparent())
    {
        g.setColour(shadow.withMultipliedAlpha(0.5f));
        g.fillPath(mPath, juce::AffineTransform::translation(0.0f, 2.0f));
        g.setColour(shadow.withMultipliedAlpha(0.75f));
        g.fillPath(mPath, juce::AffineTransform::translation(1.0f, 1.0f));
    }

    if(!foreground.isTransparent())
    {
        g.setColour(foreground);
        g.fillPath(mPath);
    }
}

Track::Plot::LabelArrangement::LabelArrangement(juce::Font const& font, juce::String const unit, int numDecimals)
: mFont(font)
, mUnit(unit)
, mNumDecimal(numDecimals)
{
}

void Track::Plot::LabelArrangement::addValue(float value, float x, float y)
{
    if(std::abs(mLastValue - value) < std::numeric_limits<float>::epsilon())
    {
        return;
    }

    if(!mHighInfo.text.isEmpty() && x > mHighInfo.x + mHighInfo.w)
    {
        if(mLowInfo.text.isEmpty() && y < mHighInfo.y)
        {
            mGlyphArrangement.addLineOfText(mFont, mHighInfo.text + mUnit, mHighInfo.x, mHighInfo.y + mLowInfo.offset);
        }
        else
        {
            mGlyphArrangement.addLineOfText(mFont, mHighInfo.text + mUnit, mHighInfo.x, mHighInfo.y + mHighInfo.offset);
        }
        mHighInfo.text.clear();
    }
    if(!mLowInfo.text.isEmpty() && x > mLowInfo.x + mLowInfo.w)
    {
        mGlyphArrangement.addLineOfText(mFont, mLowInfo.text + mUnit, mLowInfo.x, mLowInfo.y + mLowInfo.offset);
        mLowInfo.text.clear();
    }

    auto const shouldInsertHight = !mHighInfo.text.isEmpty() && y < mHighInfo.y;
    auto const shouldInsertLow = !mLowInfo.text.isEmpty() && y > mLowInfo.y;
    if(shouldInsertHight || (!shouldInsertLow && mHighInfo.text.isEmpty()))
    {
        mHighInfo.text = Format::valueToString(value, mNumDecimal);
        mHighInfo.x = x;
        mHighInfo.y = y;
        mHighInfo.w = std::ceil(static_cast<float>(mHighInfo.text.length()) * mCharWidth + mUnitWidth + 4.0f);
        mLastValue = value;
    }
    else if(shouldInsertLow || mLowInfo.text.isEmpty())
    {
        mLowInfo.text = Format::valueToString(value, mNumDecimal);
        mLowInfo.x = x;
        mLowInfo.y = y;
        mLowInfo.w = std::ceil(static_cast<float>(mLowInfo.text.length()) * mCharWidth + mUnitWidth + 4.0f);
        mLastValue = value;
    }
}

void Track::Plot::LabelArrangement::draw(juce::Graphics& g, juce::Colour const& colour)
{
    g.setColour(colour);
    mGlyphArrangement.draw(g);
}

Track::Plot::Plot(Director& director, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor)
: mDirector(director)
, mTimeZoomAccessor(timeZoomAccessor)
, mTransportAccessor(transportAccessor)
{
    setInterceptsMouseClicks(false, false);
    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
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
    paint(mAccessor, mTimeZoomAccessor, g, getLocalBounds(), findColour(Decorator::ColourIds::normalBorderColourId));
}

void Track::Plot::paintGrid(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour const colour)
{
    if(colour.isTransparent() || accessor.getAttr<AttrType::grid>() == GridMode::hidden)
    {
        return;
    }
    using Justification = Zoom::Grid::Justification;
    auto const justificationHorizontal = accessor.getAttr<AttrType::grid>() == GridMode::partial ? Justification(Zoom::Grid::Justification::left | Zoom::Grid::Justification::right) : Justification(Justification::horizontallyCentred);
    auto const justificationVertical = accessor.getAttr<AttrType::grid>() == GridMode::partial ? Justification(Zoom::Grid::Justification::top | Zoom::Grid::Justification::bottom) : Justification(Justification::verticallyCentred);

    auto const getStringify = [&]() -> std::function<juce::String(double)>
    {
        switch(Tools::getFrameType(accessor))
        {
            case Track::FrameType::label:
            {
                return nullptr;
            }
            case Track::FrameType::value:
            {
                return [unit = Tools::getUnit(accessor)](double value)
                {
                    return Format::valueToString(value, 4) + unit;
                };
            }
            case Track::FrameType::vector:
            {
                return [&](double value)
                {
                    auto const& output = accessor.getAttr<AttrType::description>().output;
                    auto const binIndex = value >= 0.0 ? static_cast<size_t>(std::round(value)) : 0_z;
                    if(binIndex >= output.binNames.size() || output.binNames[binIndex].empty())
                    {
                        return juce::String(binIndex);
                    }
                    return juce::String(output.binNames[binIndex]);
                };
            }
        }
        return nullptr;
    };

    auto const stringify = getStringify();
    auto const paintChannel = [&](Zoom::Accessor const& zoomAcsr, juce::Rectangle<int> const& region)
    {
        g.setColour(colour);
        Zoom::Grid::paintVertical(g, zoomAcsr.getAcsr<Zoom::AcsrType::grid>(), zoomAcsr.getAttr<Zoom::AttrType::visibleRange>(), region, stringify, justificationHorizontal);
    };

    switch(Tools::getFrameType(accessor))
    {
        case Track::FrameType::label:
        {
            Tools::paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int>, size_t)
                                 {
                                 });
        }
        break;
        case Track::FrameType::value:
        {
            Tools::paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int> region, size_t)
                                 {
                                     paintChannel(accessor.getAcsr<AcsrType::valueZoom>(), region);
                                 });
        }
        break;
        case Track::FrameType::vector:
        {
            Tools::paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int> region, size_t)
                                 {
                                     paintChannel(accessor.getAcsr<AcsrType::binZoom>(), region);
                                 });
        }
        break;
    }

    g.setColour(colour);
    Zoom::Grid::paintHorizontal(g, timeZoomAccessor.getAcsr<Zoom::AcsrType::grid>(), timeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>(), bounds, nullptr, 70, justificationVertical);
}

void Track::Plot::paint(Accessor const& accessor, Zoom::Accessor const& timeZoomAcsr, juce::Graphics& g, juce::Rectangle<int> const& bounds, juce::Colour const colour)
{
    g.setFont(accessor.getAttr<AttrType::font>());
    switch(Tools::getFrameType(accessor))
    {
        case Track::FrameType::label:
        {
            paintGrid(accessor, timeZoomAcsr, g, bounds, colour);
            Tools::paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int> region, size_t channel)
                                 {
                                     paintMarkers(accessor, channel, g, region, timeZoomAcsr);
                                 });
        }
        break;
        case Track::FrameType::value:
        {
            paintGrid(accessor, timeZoomAcsr, g, bounds, colour);
            Tools::paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int> region, size_t channel)
                                 {
                                     paintPoints(accessor, channel, g, region, timeZoomAcsr);
                                 });
        }
        break;
        case Track::FrameType::vector:
        {
            Tools::paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int> region, size_t channel)
                                 {
                                     paintColumns(accessor, channel, g, region, timeZoomAcsr);
                                 });
            paintGrid(accessor, timeZoomAcsr, g, bounds, colour);
        }
        break;
    }
}

void Track::Plot::paintMarkers(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr)
{
    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access) || results.isEmpty())
    {
        return;
    }

    auto const markers = results.getMarkers();
    if(markers == nullptr || markers->size() <= channel)
    {
        return;
    }

    if(bounds.isEmpty() || g.getClipBounds().isEmpty())
    {
        return;
    }

    auto const& timeRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    if(timeRange.isEmpty())
    {
        return;
    }

    auto const fbounds = bounds.toFloat();
    auto const& colours = accessor.getAttr<AttrType::colours>();
    auto const& unit = Tools::getUnit(accessor);

    auto constexpr epsilonPixel = 1.0f;
    auto const clipBounds = g.getClipBounds().toFloat();
    auto const clipTimeStart = Tools::pixelToSeconds(clipBounds.getX() - epsilonPixel, timeRange, fbounds);
    auto const clipTimeEnd = Tools::pixelToSeconds(clipBounds.getRight() + epsilonPixel, timeRange, fbounds);

    juce::RectangleList<float> ticks;
    juce::RectangleList<float> durations;
    std::vector<std::tuple<juce::String, int, int>> labels;
    auto const showLabel = !colours.text.isTransparent();
    auto const font = g.getCurrentFont();

    auto const y = fbounds.getY();
    auto const height = fbounds.getHeight();

    auto const& channelResults = markers->at(channel);
    auto it = Results::findFirstAt(channelResults, clipTimeStart);
    auto expectedEnd = Results::findFirstAt(channelResults, clipTimeEnd);
    auto const numElements = std::distance(it, expectedEnd);
    ticks.ensureStorageAllocated(static_cast<int>(numElements) + 1);
    durations.ensureStorageAllocated(static_cast<int>(numElements) + 1);

    juce::Rectangle<float> currentTick;
    juce::Rectangle<float> currentDuration;
    while(it != channelResults.cend() && std::get<0>(*it) < clipTimeEnd)
    {
        auto const position = std::get<0>(*it);
        auto const duration = std::get<1>(*it);
        auto const x = Tools::secondsToPixel(position, timeRange, fbounds);
        auto const w = Tools::secondsToPixel(position + duration, timeRange, fbounds) - x;

        if(!currentTick.isEmpty() && currentTick.getRight() >= x)
        {
            currentTick = currentTick.getUnion({x, y, 1.0f, height});
        }
        else
        {
            if(!currentTick.isEmpty())
            {
                ticks.addWithoutMerging(currentTick);
            }
            currentTick = {x, y, 1.0f, height};
        }

        if(w >= 1.0f && !currentDuration.isEmpty() && currentDuration.getRight() >= x)
        {
            currentDuration = currentDuration.getUnion({x, y, w, height});
        }
        else
        {
            if(!currentDuration.isEmpty())
            {
                durations.addWithoutMerging(currentDuration);
            }
            if(w >= 1.0f)
            {
                currentDuration = {x, y, w, height};
            }
            else
            {
                currentDuration = {};
            }
        }

        if(showLabel && !std::get<2>(*it).empty())
        {
            auto const previousLabelLimit = labels.empty() ? x : static_cast<float>(std::get<1>(labels.back()) + std::get<2>(labels.back()));
            if(previousLabelLimit <= x)
            {
                auto const text = juce::String(std::get<2>(*it)) + unit;
                auto const textWidth = font.getStringWidth(text) + 2;
                auto const textX = static_cast<int>(std::round(x)) + 2;
                labels.push_back(std::make_tuple(text, textX, textWidth));
            }
        }

        it = std::next(it);
    }

    if(!currentTick.isEmpty())
    {
        ticks.addWithoutMerging(currentTick);
    }
    if(!currentDuration.isEmpty())
    {
        durations.addWithoutMerging(currentDuration);
    }

    g.setColour(colours.foreground.withAlpha(0.4f));
    g.fillRectList(durations);

    if(!colours.shadow.isTransparent())
    {
        ticks.offsetAll(-2.0f, 0.0f);

        g.setColour(colours.shadow.withMultipliedAlpha(0.5f));
        g.fillRectList(ticks);
        ticks.offsetAll(1.0f, 0.0f);

        g.setColour(colours.shadow.withMultipliedAlpha(0.75f));
        g.fillRectList(ticks);
        ticks.offsetAll(1.0f, 0.0f);
    }
    g.setColour(colours.foreground);
    g.fillRectList(ticks);

    if(showLabel)
    {
        g.setColour(colours.text);
        for(auto const& label : labels)
        {
            g.drawSingleLineText(std::get<0>(label), std::get<1>(label), bounds.getY() + 22, juce::Justification::left);
        }
    }
}

void Track::Plot::paintPoints(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr)
{
    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access) || results.isEmpty())
    {
        return;
    }

    auto const points = results.getPoints();
    if(points == nullptr || points->size() <= channel)
    {
        return;
    }

    if(bounds.isEmpty() || g.getClipBounds().isEmpty())
    {
        return;
    }

    auto const& timeRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    if(timeRange.isEmpty())
    {
        return;
    }

    auto const& valueRange = accessor.getAcsr<AcsrType::valueZoom>().getAttr<Zoom::AttrType::visibleRange>();
    if(valueRange.isEmpty())
    {
        return;
    }

    auto const fbounds = bounds.toFloat();
    auto const& colours = accessor.getAttr<AttrType::colours>();
    auto const& unit = Tools::getUnit(accessor);

    auto const font = g.getCurrentFont();

    auto constexpr epsilonPixel = 1.0f;
    auto const clipBounds = g.getClipBounds();
    auto const clipTimeStart = Tools::pixelToSeconds(static_cast<float>(clipBounds.getX()) - epsilonPixel, timeRange, fbounds);
    auto const clipTimeEnd = Tools::pixelToSeconds(static_cast<float>(clipBounds.getRight()) + epsilonPixel, timeRange, fbounds);

    auto getNumDecimals = [&]()
    {
        auto const rangeLength = valueRange.getLength();
        int numDecimals = 0;
        while(numDecimals < 4 && std::pow(10.0, static_cast<double>(numDecimals)) / rangeLength < 1.0)
        {
            ++numDecimals;
        }
        return 4 - numDecimals;
    };
    auto const numDecimals = getNumDecimals();
    LabelArrangement labelArr(font, unit, getNumDecimals());

    auto const& channelResults = points->at(channel);
    if(channelResults.size() == 1_z)
    {
        if(std::get<2>(channelResults[0]).has_value())
        {
            auto const value = *std::get<2>(channelResults[0]);
            auto const y = Tools::valueToPixel(value, valueRange, fbounds);

            if(!colours.shadow.isTransparent())
            {
                g.setColour(colours.shadow.withMultipliedAlpha(0.5f));
                g.drawLine(fbounds.getX(), y + 2.f, fbounds.getRight(), y + 2.f);
                g.setColour(colours.shadow.withMultipliedAlpha(0.75f));
                g.drawLine(fbounds.getX(), y + 1.f, fbounds.getRight(), y + 1.f);
            }
            g.setColour(colours.foreground);
            g.drawLine(fbounds.getX(), y, fbounds.getRight(), y);

            if(!colours.text.isTransparent())
            {
                g.setColour(colours.text);
                g.drawSingleLineText(Format::valueToString(value, numDecimals) + unit, 4, static_cast<int>(y - font.getDescent()) - 2, juce::Justification::left);
            }
        }
        return;
    }

    auto it = Results::findFirstAt(channelResults, clipTimeStart);
    if(it != channelResults.cbegin())
    {
        it = std::prev(it);
    }

    // Time distance corresponding to epsilon pixels
    auto const timeEpsilon = static_cast<double>(epsilonPixel) * timeRange.getLength() / static_cast<double>(bounds.getWidth());

    juce::RectangleList<int> rectangles;
    PathArrangement pathArr;
    auto getNextItBeforeTime = [](decltype(it) _start, decltype(it) _end, double const& l)
    {
        auto min = *std::get<2>(*_start);
        auto max = min;
        _start = std::next(_start);
        while(_start != _end && std::get<0>(*_start) < l)
        {
            if(std::get<2>(*_start).has_value())
            {
                auto const& lvalue = *std::get<2>(*_start);
                min = std::min(min, lvalue);
                max = std::max(max, lvalue);
            }
            _start = std::next(_start);
        }
        return std::make_tuple(std::prev(_start), min, max);
    };

    auto const showLabel = !colours.text.isTransparent();
    auto hasExceededEnd = false;
    while(!hasExceededEnd && it != channelResults.cend())
    {
        if(!std::get<2>(*it).has_value())
        {
            pathArr.stopLine();
            it = std::find_if(std::next(it), channelResults.cend(), [&](auto const& result)
                              {
                                  return std::get<2>(result).has_value();
                              });
            hasExceededEnd = it == channelResults.cend() || std::get<0>(*it) >= clipTimeEnd;
        }
        else if(std::get<1>(*it) < timeEpsilon)
        {
            auto const end = std::get<0>(*it) + std::get<1>(*it);
            auto const limit = end + timeEpsilon;
            auto const value = *std::get<2>(*it);
            auto const x = Tools::secondsToPixel(std::get<0>(*it), timeRange, fbounds);

            auto const nextResult = getNextItBeforeTime(it, channelResults.cend(), limit);
            auto const next = std::get<0>(nextResult);
            auto const min = std::get<1>(nextResult);
            auto const max = std::get<2>(nextResult);
            auto const nend = std::get<0>(*next) + std::get<1>(*next);
            auto const x2 = Tools::secondsToPixel(nend, timeRange, fbounds);

            if(it != next && (max - min) > std::numeric_limits<float>::epsilon())
            {
                auto const y1 = Tools::valueToPixel(min, valueRange, fbounds);
                auto const y2 = Tools::valueToPixel(max, valueRange, fbounds);
                pathArr.addLine(x, x, y1);
                pathArr.stopLine();

                rectangles.addWithoutMerging({static_cast<int>(x), static_cast<int>(y2), std::max(static_cast<int>(x2) - static_cast<int>(x), 1), std::max(static_cast<int>(y1 - y2), 1)});
                if(showLabel)
                {
                    labelArr.addValue(min, x, y1);
                    labelArr.addValue(max, x, y2);
                }

                hasExceededEnd = nend >= clipTimeEnd;
                it = next;
            }
            else
            {
                auto const y = Tools::valueToPixel(value, valueRange, fbounds);
                if(showLabel)
                {
                    labelArr.addValue(value, x, y);
                }
                pathArr.addLine(x, x2, y);
                it = std::next(it);
                hasExceededEnd = end >= clipTimeEnd;
            }
        }
        else
        {
            auto const value = *std::get<2>(*it);
            auto const x = Tools::secondsToPixel(std::get<0>(*it), timeRange, fbounds);
            auto const y = Tools::valueToPixel(value, valueRange, fbounds);
            auto const end = std::get<0>(*it) + std::get<1>(*it);
            auto const x2 = Tools::secondsToPixel(end, timeRange, fbounds);
            if(showLabel)
            {
                labelArr.addValue(value, x, y);
            }
            pathArr.addLine(x, x2, y);

            it = std::next(it);
            hasExceededEnd = end >= clipTimeEnd;
        }
    }

    if(!rectangles.isEmpty())
    {
        if(!colours.shadow.isTransparent())
        {
            rectangles.offsetAll(0.0f, 2.0f);

            g.setColour(colours.shadow.withMultipliedAlpha(0.5f));
            g.fillRectList(rectangles);
            rectangles.offsetAll(0.0f, -1.0f);

            g.setColour(colours.shadow.withMultipliedAlpha(0.75f));
            g.fillRectList(rectangles);
            rectangles.offsetAll(0.0f, -1.0f);
        }
        g.setColour(colours.foreground);
        g.fillRectList(rectangles);
    }

    pathArr.draw(g, colours.foreground, colours.shadow);
    if(showLabel)
    {
        labelArr.draw(g, colours.text);
    }
}

void Track::Plot::paintColumns(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr)
{
    auto const images = accessor.getAttr<AttrType::graphics>();
    if(images.empty() || images.size() <= channel || images.at(channel).empty())
    {
        return;
    }

    if(bounds.isEmpty() || g.getClipBounds().isEmpty())
    {
        return;
    }

    auto const& globalTimeRange = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
    auto const& visibleTimeRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    if(globalTimeRange.isEmpty() || visibleTimeRange.isEmpty())
    {
        return;
    }

    auto const renderImage = [&](juce::Image const& image, Zoom::Accessor const& xZoomAcsr, Zoom::Accessor const& yZoomAcsr)
    {
        if(!image.isValid())
        {
            return;
        }

        using PixelRange = juce::Range<int>;
        using ZoomRange = Zoom::Range;

        // Gets the visible zoom range of a zoom accessor and inverses it if necessary
        auto const getZoomRange = [](Zoom::Accessor const& zoomAcsr, bool inverted)
        {
            if(!inverted)
            {
                return zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            }
            auto const& globalRange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
            auto const& visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            return ZoomRange{globalRange.getEnd() - visibleRange.getEnd() + globalRange.getStart(), globalRange.getEnd() - visibleRange.getStart() + globalRange.getStart()};
        };

        // Gets the visible zoom range equivalent to the graphics clip bounds
        auto const clipZoomRange = [](PixelRange const& global, PixelRange const& visible, ZoomRange const& zoom)
        {
            auto const ratio = zoom.getLength() / static_cast<double>(global.getLength());
            auto const x1 = static_cast<double>(visible.getStart() - global.getStart()) * ratio + zoom.getStart();
            auto const x2 = static_cast<double>(visible.getEnd() - global.getStart()) * ratio + zoom.getStart();
            return ZoomRange{x1, x2};
        };

        // Converts the visible zoom range to image range
        auto toImageRange = [](ZoomRange const& globalRange, ZoomRange const& visibleRange, int imageSize)
        {
            auto const globalLength = globalRange.getLength();
            anlStrongAssert(globalLength > 0.0);
            if(globalLength <= 0.0)
            {
                return juce::Range<float>();
            }
            auto const scale = static_cast<float>(imageSize);
            auto scaleValue = [&](double value)
            {
                return static_cast<float>((value - globalRange.getStart()) / globalLength * scale);
            };
            return juce::Range<float>{scaleValue(visibleRange.getStart()), scaleValue(visibleRange.getEnd())};
        };

        auto const clipBounds = g.getClipBounds().constrainedWithin(bounds);

        auto const xClippedRange = clipZoomRange(bounds.getHorizontalRange(), clipBounds.getHorizontalRange(), getZoomRange(xZoomAcsr, false));
        auto const xRange = toImageRange(xZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), xClippedRange, image.getWidth());

        auto const yClippedRange = clipZoomRange(bounds.getVerticalRange(), clipBounds.getVerticalRange(), getZoomRange(yZoomAcsr, true));
        auto const yRange = toImageRange(yZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), yClippedRange, image.getHeight());

        g.setColour(juce::Colours::black);
        g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
        Tools::paintClippedImage(g, image, {xRange.getStart(), yRange.getStart(), xRange.getLength(), yRange.getLength()});
    };

    auto const getZoomRatio = [](Zoom::Accessor const& acsr)
    {
        return acsr.getAttr<Zoom::AttrType::globalRange>().getLength() / acsr.getAttr<Zoom::AttrType::visibleRange>().getLength();
    };

    auto const& binZoomAcsr = accessor.getAcsr<AcsrType::binZoom>();

    auto const timeZoomRatio = getZoomRatio(timeZoomAcsr);
    auto const binZoomRatio = getZoomRatio(binZoomAcsr);
    auto const boundsDimension = std::max(bounds.getWidth() * timeZoomRatio, bounds.getHeight() * binZoomRatio);
    for(auto const& image : images.at(channel))
    {
        auto const imageDimension = std::max(image.getWidth(), image.getHeight());
        if(imageDimension >= boundsDimension)
        {
            renderImage(image, timeZoomAcsr, binZoomAcsr);
            return;
        }
    }
    renderImage(images.at(channel).back(), timeZoomAcsr, binZoomAcsr);
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
        using namespace Result;
        using CopiedData = Modifier::CopiedData;

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

        auto const addAction = [&](CopiedData const& data)
        {
            auto& director = mPlot.mDirector;
            auto& undoManager = director.getUndoManager();
            undoManager.beginNewTransaction(juce::translate("Add Frame"));
            auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, event.x);
            undoManager.perform(std::make_unique<Modifier::ActionPaste>(director.getSafeAccessorFn(), std::get<0_z>(*channel), juce::Range<double>{}, data, time).release());
        };

        switch(Tools::getFrameType(mAccessor))
        {
            case Track::FrameType::label:
            {
                addAction(std::map<size_t, Data::Marker>{std::make_pair(0_z, Data::Marker{0.0, 0.0, ""})});
            }
            break;
            case Track::FrameType::value:
            {
                auto const& valueZoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
                auto const range = valueZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
                auto const top = static_cast<float>(std::get<1_z>(*channel).getStart());
                auto const bottom = static_cast<float>(std::get<1_z>(*channel).getEnd());
                auto const value = Tools::pixelToValue(static_cast<float>(event.y), range, {0.0f, top, 1.0f, bottom - top});
                addAction(std::map<size_t, Data::Point>{std::make_pair(0_z, Data::Point{0.0, 0.0, value})});
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
