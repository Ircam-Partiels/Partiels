#include "AnlTrackPlot.h"
#include "../Zoom/AnlZoomTools.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Plot::Plot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor)
: mAccessor(accessor)
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
            case AttrType::name:
            case AttrType::key:
            case AttrType::description:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::warnings:
            case AttrType::time:
            case AttrType::processing:
            case AttrType::focused:
                break;
            case AttrType::results:
            case AttrType::graphics:
            case AttrType::colours:
            {
                repaint();
            }
            break;
        }
    };

    mValueZoomListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };

    mBinZoomListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };

    mTimeZoomListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::valueZoom>().addListener(mValueZoomListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::binZoom>().addListener(mBinZoomListener, NotificationType::synchronous);
    mTimeZoomAccessor.addListener(mTimeZoomListener, NotificationType::synchronous);
}

Track::Plot::~Plot()
{
    mTimeZoomAccessor.removeListener(mTimeZoomListener);
    mAccessor.getAcsr<AcsrType::binZoom>().removeListener(mBinZoomListener);
    mAccessor.getAcsr<AcsrType::valueZoom>().removeListener(mValueZoomListener);
    mAccessor.removeListener(mListener);
}

void Track::Plot::paint(juce::Graphics& g)
{
    auto const resultsPtr = mAccessor.getAttr<AttrType::results>();
    if(resultsPtr == nullptr || resultsPtr->empty())
    {
        return;
    }

    auto const bounds = getLocalBounds();
    if(bounds.isEmpty() || g.getClipBounds().isEmpty())
    {
        return;
    }

    auto const& globalRange = mTimeZoomAccessor.getAttr<Zoom::AttrType::globalRange>();
    if(globalRange.isEmpty())
    {
        return;
    }

    auto const& valueRange = mAccessor.getAcsr<AcsrType::valueZoom>().getAttr<Zoom::AttrType::visibleRange>();
    auto const& colours = mAccessor.getAttr<AttrType::colours>();
    auto const& output = mAccessor.getAttr<AttrType::description>().output;
    switch(Tools::getDisplayType(mAccessor))
    {
        case Tools::DisplayType::markers:
        {
            paintMarkers(g, bounds.toFloat(), colours, output.unit, *resultsPtr, mTimeZoomAccessor);
        }
        break;
        case Tools::DisplayType::segments:
        {
            paintSegments(g, bounds.toFloat(), colours, output.unit, *resultsPtr, mTimeZoomAccessor, valueRange);
        }
        break;
        case Tools::DisplayType::grid:
        {
            paintGrid(g, bounds, mAccessor.getAttr<AttrType::graphics>(), mTimeZoomAccessor, mAccessor.getAcsr<AcsrType::binZoom>());
        }
        break;
    }
}

void Track::Plot::paintMarkers(juce::Graphics& g, juce::Rectangle<float> const& bounds, ColourSet const& colours, juce::String const& unit, std::vector<Plugin::Result> const& results, Zoom::Accessor const& timeZoomAcsr)
{
    auto constexpr epsilonPixel = 2.0f;
    auto const& globalRange = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
    auto const& timeRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const clipBounds = g.getClipBounds().toFloat();
    auto const clipTimeStart = Tools::pixelToSeconds(clipBounds.getX() - epsilonPixel, timeRange, bounds);
    auto const clipTimeEnd = Tools::pixelToSeconds(clipBounds.getRight() + epsilonPixel, timeRange, bounds);
    auto const rtEnd = Tools::secondsToRealTime(clipTimeEnd);

    // Time distance corresponding to epsilon pixels
    auto const minDiffTime = Tools::secondsToRealTime(static_cast<double>(epsilonPixel) * timeRange.getLength() / static_cast<double>(bounds.getWidth()));

    juce::RectangleList<float> rectangles;
    std::vector<std::tuple<juce::String, int, int>> labels;
    auto const showLabel = !colours.text.isTransparent();
    auto const font = g.getCurrentFont();

    auto it = Tools::getIteratorAt(results, globalRange, clipTimeStart);
    while(it != results.cend() && it->timestamp < rtEnd)
    {
        auto const start = Tools::realTimeToSeconds(it->timestamp);
        auto const x1 = Tools::secondsToPixel(start, timeRange, bounds);

        // Skip any adjacent result with a distance inferior to epsilon pixels
        auto next = std::next(it);
        while(next != results.cend() && next->timestamp < rtEnd && next->timestamp < Tools::getEndRealTime(*it) + minDiffTime)
        {
            it = std::exchange(next, std::next(next));
        }

        auto const end = Tools::realTimeToSeconds(Tools::getEndRealTime(*it));
        auto const x2 = Tools::secondsToPixel(end, timeRange, bounds);
        auto const w = Tools::secondsToPixel(end, timeRange, bounds) - x1;
        rectangles.addWithoutMerging({x1, clipBounds.getY(), std::max(w, 1.0f), clipBounds.getHeight()});

        if(showLabel && !it->label.empty() && (labels.empty() || static_cast<float>(std::get<1>(labels.back()) + std::get<2>(labels.back())) <= x2))
        {
            auto const text = juce::String(it->label) + unit;
            auto const textWidth = font.getStringWidth(text) + 2;
            auto const textX = static_cast<int>(std::round(x2)) + 2;
            labels.push_back(std::make_tuple(text, textX, textWidth));
        }
        it = std::next(it);
    }

    if(!colours.shadow.isTransparent())
    {
        rectangles.offsetAll(-2.0f, 0.0f);

        g.setColour(colours.shadow.withMultipliedAlpha(0.5f));
        g.fillRectList(rectangles);
        rectangles.offsetAll(1.0f, 0.0f);

        g.setColour(colours.shadow.withMultipliedAlpha(0.75f));
        g.fillRectList(rectangles);
        rectangles.offsetAll(1.0f, 0.0f);
    }
    g.setColour(colours.foreground);
    g.fillRectList(rectangles);

    if(showLabel)
    {
        g.setColour(colours.text);
        for(auto const& label : labels)
        {
            g.drawSingleLineText(std::get<0>(label), std::get<1>(label), 22, juce::Justification::left);
        }
    }
}

void Track::Plot::paintSegments(juce::Graphics& g, juce::Rectangle<float> const& bounds, ColourSet const& colours, juce::String const& unit, std::vector<Plugin::Result> const& results, Zoom::Accessor const& timeZoomAcsr, juce::Range<double> const& valueRange)
{
    anlWeakAssert(!valueRange.isEmpty());
    if(valueRange.isEmpty())
    {
        return;
    }
    if(results.empty())
    {
        return;
    }

    auto constexpr epsilonPixel = 1.0f;
    auto const& globalRange = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
    auto const& timeRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const clipTimeStart = Tools::pixelToSeconds(static_cast<float>(g.getClipBounds().getX()) - epsilonPixel, timeRange, bounds);
    auto const clipTimeEnd = Tools::pixelToSeconds(static_cast<float>(g.getClipBounds().getRight()) + epsilonPixel, timeRange, bounds);
    auto const rtEnd = Tools::secondsToRealTime(clipTimeEnd);

    auto it = Tools::getIteratorAt(results, globalRange, clipTimeStart);

    // Time distance corresponding to epsilon pixels
    auto const minDiffTime = Tools::secondsToRealTime(static_cast<double>(epsilonPixel) * timeRange.getLength() / static_cast<double>(bounds.getWidth()));
    juce::Path path;
    juce::RectangleList<int> rectangles;
    using labelInfo = std::tuple<juce::String, int, int, float>;
    labelInfo labelInfoLow;
    labelInfo labelInfoHigh;
    std::vector<labelInfo> labels;

    auto const font = g.getCurrentFont();
    auto const fontAscent = font.getAscent();
    auto const fontDescent = font.getDescent();
    auto insertLabel = [&](int x, float y, float value, std::string const& label)
    {
        auto canInsertHight = !std::get<0>(labelInfoHigh).isEmpty() && x > std::get<1>(labelInfoHigh) + std::get<2>(labelInfoHigh) + 2;
        auto canInsertLow = !std::get<0>(labelInfoLow).isEmpty() && x > std::get<1>(labelInfoLow) + std::get<2>(labelInfoLow) + 2;

        if(canInsertHight && canInsertLow && labelInfoHigh == labelInfoLow)
        {
            canInsertHight = y >= std::get<3>(labelInfoHigh);
            canInsertLow = !canInsertHight;
        }

        if(canInsertHight)
        {
            if(labelInfoHigh == labelInfoLow)
            {
                std::get<0>(labelInfoLow).clear();
            }
            std::get<3>(labelInfoHigh) -= fontDescent - 2.0f;
            labels.push_back(labelInfoHigh);
            std::get<0>(labelInfoHigh).clear();
        }
        if(std::get<0>(labelInfoHigh).isEmpty() || y < std::get<3>(labelInfoHigh))
        {
            auto const text = juce::String(value, 2) + unit + (label.empty() ? "" : (" (" + juce::String(label) + ")"));
            auto const textWidth = font.getStringWidth(text) + 2;
            labelInfoHigh = std::make_tuple(text, x, textWidth, y);
        }

        if(canInsertLow)
        {
            if(labelInfoLow == labelInfoHigh)
            {
                std::get<0>(labelInfoHigh).clear();
            }
            std::get<3>(labelInfoLow) += fontAscent + 2.0f;
            labels.push_back(labelInfoLow);
            std::get<0>(labelInfoLow).clear();
        }
        if(std::get<0>(labelInfoLow).isEmpty() || y > std::get<3>(labelInfoLow))
        {
            auto const text = juce::String(value, 2) + unit + (label.empty() ? "" : (" (" + juce::String(label) + ")"));
            auto const textWidth = font.getStringWidth(text) + 2;
            labelInfoLow = std::make_tuple(text, x, textWidth, y);
        }
    };

    auto const showLabel = !colours.text.isTransparent();
    auto shouldStartSubPath = true;
    auto hasExceededEnd = false;
    while(!hasExceededEnd && it != results.cend())
    {
        if(it->values.empty())
        {
            shouldStartSubPath = true;
            it = std::find_if(std::next(it), results.cend(), [&](Plugin::Result const& result)
                              {
                                  return !result.values.empty();
                              });
            hasExceededEnd = it == results.cend() || Tools::getEndRealTime(*it) >= rtEnd;
        }
        else if(!it->hasDuration || it->duration < minDiffTime)
        {
            auto const end = Tools::getEndRealTime(*it);
            auto const limit = end + minDiffTime;
            auto const value = it->values[0];
            auto const x = Tools::secondsToPixel(Tools::realTimeToSeconds(it->timestamp), timeRange, bounds);

            auto getNextItBeforeTime = [](decltype(it) _start, decltype(it) _end, Vamp::RealTime const& l)
            {
                float min = _start->values[0];
                float max = min;
                _start = std::next(_start);
                while(_start != _end)
                {
                    if(_start->timestamp >= l)
                    {
                        return std::make_tuple(std::prev(_start), min, max);
                    }
                    if(!_start->values.empty())
                    {
                        auto const& lvalue = _start->values.at(0);
                        min = std::min(min, lvalue);
                        max = std::max(max, lvalue);
                    }
                    ++_start;
                }
                return std::make_tuple(std::prev(_start), min, max);
            };
            auto const nextResult = getNextItBeforeTime(it, results.cend(), limit);
            auto const next = std::get<0>(nextResult);
            auto const min = std::get<1>(nextResult);
            auto const max = std::get<2>(nextResult);

            if(it != next)
            {
                auto const nend = Tools::getEndRealTime(*next);
                auto const x2 = static_cast<int>(Tools::secondsToPixel(Tools::realTimeToSeconds(nend), timeRange, bounds));
                auto const y1 = static_cast<int>(Tools::valueToPixel(min, valueRange, bounds));
                auto const y2 = static_cast<int>(Tools::valueToPixel(max, valueRange, bounds));
                if(!shouldStartSubPath)
                {
                    path.lineTo(x, y1);
                }

                rectangles.addWithoutMerging({static_cast<int>(x), y2, std::max(x2 - static_cast<int>(x), 1), std::max(y1 - y2, 1)});
                if(showLabel)
                {
                    insertLabel(static_cast<int>(x), y1, min, it->label);
                    insertLabel(static_cast<int>(x), y2, max, it->label);
                }

                shouldStartSubPath = true;
                hasExceededEnd = nend >= rtEnd;
                it = next;
            }
            else
            {
                auto const y = Tools::valueToPixel(value, valueRange, bounds);
                if(showLabel)
                {
                    insertLabel(static_cast<int>(x), y, value, it->label);
                }
                if(std::exchange(shouldStartSubPath, false))
                {
                    path.startNewSubPath(x, y);
                }
                else
                {
                    path.lineTo(x, y);
                }
                if(it->hasDuration)
                {
                    auto const x2 = Tools::secondsToPixel(Tools::realTimeToSeconds(end), timeRange, bounds);
                    path.lineTo(x2, y);
                }
                it = std::next(it);
                hasExceededEnd = end >= rtEnd;
            }
        }
        else
        {
            auto const value = it->values[0];
            auto const x = Tools::secondsToPixel(Tools::realTimeToSeconds(it->timestamp), timeRange, bounds);
            auto const y = Tools::valueToPixel(value, valueRange, bounds);
            if(showLabel)
            {
                insertLabel(static_cast<int>(x), y, value, it->label);
            }
            if(std::exchange(shouldStartSubPath, false))
            {
                path.startNewSubPath(x, y);
            }
            else
            {
                path.lineTo(x, y);
            }
            auto const end = Tools::getEndRealTime(*it);
            auto const x2 = Tools::secondsToPixel(Tools::realTimeToSeconds(end), timeRange, bounds);
            path.lineTo(x2, y);
            it = std::next(it);
            hasExceededEnd = end >= rtEnd;
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
    if(!path.isEmpty())
    {
        juce::PathStrokeType const pathStrokeType(1.0f);
        pathStrokeType.createStrokedPath(path, path);
        if(!colours.shadow.isTransparent())
        {
            g.setColour(colours.shadow.withMultipliedAlpha(0.5f));
            g.fillPath(path, juce::AffineTransform::translation(0.0f, 2.0f));
            g.setColour(colours.shadow.withMultipliedAlpha(0.75f));
            g.fillPath(path, juce::AffineTransform::translation(1.0f, 1.0f));
        }
        g.setColour(colours.foreground);
        g.fillPath(path);
    }

    if(showLabel)
    {
        g.setColour(colours.text);
        for(auto const& label : labels)
        {
            g.drawSingleLineText(std::get<0>(label), std::get<1>(label), static_cast<int>(std::round(std::get<3>(label))), juce::Justification::left);
        }
    }
}

void Track::Plot::paintGrid(juce::Graphics& g, juce::Rectangle<int> const& bounds, std::vector<juce::Image> const& images, Zoom::Accessor const& timeZoomAcsr, Zoom::Accessor const& binZoomAcsr)
{
    if(images.empty())
    {
        return;
    }

    auto renderImage = [&](juce::Image const& image, Zoom::Accessor const& xZoomAcsr, Zoom::Accessor const& yZoomAcsr)
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

        // Draws a range of an image
        auto drawImage = [&](juce::Rectangle<float> const& rectangle)
        {
            auto const graphicsBounds = g.getClipBounds().toFloat();
            auto const imageBounds = rectangle.getSmallestIntegerContainer();
            auto const clippedImage = image.getClippedImage(imageBounds);
            anlWeakAssert(clippedImage.getWidth() == imageBounds.getWidth());
            anlWeakAssert(clippedImage.getHeight() == imageBounds.getHeight());
            auto const deltaX = static_cast<float>(imageBounds.getX()) - rectangle.getX();
            auto const deltaY = static_cast<float>(imageBounds.getY()) - rectangle.getY();
            anlWeakAssert(deltaX <= 0);
            anlWeakAssert(deltaY <= 0);
            auto const scaleX = graphicsBounds.getWidth() / rectangle.getWidth();
            auto const scaleY = graphicsBounds.getHeight() / rectangle.getHeight();

            g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
            g.drawImageTransformed(clippedImage, juce::AffineTransform::translation(deltaX, deltaY).scaled(scaleX, scaleY).translated(graphicsBounds.getX(), graphicsBounds.getY()));
        };

        auto const clipBounds = g.getClipBounds().constrainedWithin(bounds);

        auto const xClippedRange = clipZoomRange(bounds.getHorizontalRange(), clipBounds.getHorizontalRange(), getZoomRange(xZoomAcsr, false));
        auto const xRange = toImageRange(xZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), xClippedRange, image.getWidth());

        auto const yClippedRange = clipZoomRange(bounds.getVerticalRange(), clipBounds.getVerticalRange(), getZoomRange(yZoomAcsr, true));
        auto const yRange = toImageRange(yZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), yClippedRange, image.getHeight());

        drawImage({xRange.getStart(), yRange.getStart(), xRange.getLength(), yRange.getLength()});
    };

    auto getZoomRatio = [](Zoom::Accessor const& acsr)
    {
        return acsr.getAttr<Zoom::AttrType::globalRange>().getLength() / acsr.getAttr<Zoom::AttrType::visibleRange>().getLength();
    };

    auto const timezoomRatio = getZoomRatio(timeZoomAcsr);
    auto const binZoomRatio = getZoomRatio(binZoomAcsr);
    auto const boundsDimension = std::max(bounds.getWidth() * timezoomRatio, bounds.getHeight() * binZoomRatio);
    for(auto const& image : images)
    {
        auto const imageDimension = std::max(image.getWidth(), image.getHeight());
        if(imageDimension >= boundsDimension)
        {
            renderImage(image, timeZoomAcsr, binZoomAcsr);
            return;
        }
    }
    renderImage(images.back(), timeZoomAcsr, binZoomAcsr);
}

Track::Plot::Overlay::Overlay(Plot& plot)
: mPlot(plot)
, mAccessor(mPlot.mAccessor)
, mTimeZoomAccessor(mPlot.mTimeZoomAccessor)
, mTransportPlayheadBar(plot.mTransportAccessor, mPlot.mTimeZoomAccessor)
{
    addAndMakeVisible(mPlot);
    addAndMakeVisible(mTransportPlayheadBar);
    mTransportPlayheadBar.setInterceptsMouseClicks(false, false);
    addMouseListener(&mTransportPlayheadBar, false);
    setInterceptsMouseClicks(true, true);

    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::key:
            case AttrType::description:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::results:
            case AttrType::graphics:
            case AttrType::warnings:
            case AttrType::time:
            case AttrType::focused:
                break;
            case AttrType::colours:
            {
                setOpaque(acsr.getAttr<AttrType::colours>().background.isOpaque());
            }
            break;
            case AttrType::processing:
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
                break;
            case Zoom::AttrType::visibleRange:
            {
                updateTooltip(getMouseXYRelative());
            }
            break;
            case Zoom::AttrType::anchor:
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
    mTransportPlayheadBar.setBounds(bounds);
}

void Track::Plot::Overlay::paint(juce::Graphics& g)
{
    g.fillAll(mAccessor.getAttr<AttrType::colours>().background);
}

void Track::Plot::Overlay::mouseMove(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
    updateMode(event);
}

void Track::Plot::Overlay::mouseEnter(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
    updateMode(event);
}

void Track::Plot::Overlay::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    setTooltip("");
}

void Track::Plot::Overlay::mouseDown(juce::MouseEvent const& event)
{
    updateMode(event);
    if(event.mods.isCtrlDown())
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

void Track::Plot::Overlay::updateTooltip(juce::Point<int> const& pt)
{
    if(!getLocalBounds().contains(pt))
    {
        setTooltip("");
        return;
    }
    auto const& globalRange = mTimeZoomAccessor.getAttr<Zoom::AttrType::globalRange>();
    auto const epsilon = 3.0 / static_cast<double>(getWidth()) * mTimeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>().getLength();
    auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, pt.x);
    auto const bin = Zoom::Tools::getScaledValueFromHeight(mAccessor.getAcsr<AcsrType::binZoom>(), *this, pt.y);
    auto const tip = Tools::getResultText(mAccessor, globalRange, time, static_cast<size_t>(std::floor(bin)), epsilon);
    setTooltip(Format::secondsToString(time) + ": " + (tip.isEmpty() ? "-" : tip));
}

ANALYSE_FILE_END
