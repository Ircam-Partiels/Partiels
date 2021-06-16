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
            case AttrType::processing:
            case AttrType::focused:
                break;
            case AttrType::results:
            case AttrType::graphics:
            case AttrType::colours:
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

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::valueZoom>().addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::binZoom>().addListener(mZoomListener, NotificationType::synchronous);
    mTimeZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
}

Track::Plot::~Plot()
{
    mTimeZoomAccessor.removeListener(mZoomListener);
    mAccessor.getAcsr<AcsrType::binZoom>().removeListener(mZoomListener);
    mAccessor.getAcsr<AcsrType::valueZoom>().removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Track::Plot::paint(juce::Graphics& g)
{
    paint(mAccessor, g, getLocalBounds(), mTimeZoomAccessor);
}

void Track::Plot::paint(Accessor const& accessor, juce::Graphics& g, juce::Rectangle<int> bounds, Zoom::Accessor const& timeZoomAcsr)
{
    switch(Tools::getDisplayType(accessor))
    {
        case Tools::DisplayType::markers:
        {
            Tools::paintChannels(accessor, g, bounds, [&](juce::Rectangle<int> region, size_t channel)
                                 {
                                     paintMarkers(accessor, channel, g, region, timeZoomAcsr);
                                 });
        }
        break;
        case Tools::DisplayType::points:
        {
            Tools::paintChannels(accessor, g, bounds, [&](juce::Rectangle<int> region, size_t channel)
                                 {
                                     paintPoints(accessor, channel, g, region, timeZoomAcsr);
                                 });
        }
        break;
        case Tools::DisplayType::columns:
        {
            Tools::paintChannels(accessor, g, bounds, [&](juce::Rectangle<int> region, size_t channel)
                                 {
                                     paintColumns(accessor, channel, g, region, timeZoomAcsr);
                                 });
        }
        break;
    }
}

void Track::Plot::paintMarkers(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr)
{
    auto const results = accessor.getAttr<AttrType::results>();
    if(results.isEmpty())
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

    auto const& globalRange = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
    auto const& timeRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    if(globalRange.isEmpty() || timeRange.isEmpty())
    {
        return;
    }

    auto const fbounds = bounds.toFloat();
    auto const& colours = accessor.getAttr<AttrType::colours>();
    auto const& unit = accessor.getAttr<AttrType::description>().output.unit;

    auto constexpr epsilonPixel = 2.0f;
    auto const clipBounds = g.getClipBounds().toFloat();
    auto const clipTimeStart = Tools::pixelToSeconds(clipBounds.getX() - epsilonPixel, timeRange, fbounds);
    auto const clipTimeEnd = Tools::pixelToSeconds(clipBounds.getRight() + epsilonPixel, timeRange, fbounds);

    // Time distance corresponding to epsilon pixels
    auto const timeEpsilon = static_cast<double>(epsilonPixel) * timeRange.getLength() / static_cast<double>(bounds.getWidth());

    juce::RectangleList<float> rectangles;
    std::vector<std::tuple<juce::String, int, int>> labels;
    auto const showLabel = !colours.text.isTransparent();
    auto const font = g.getCurrentFont();

    auto const& channelResults = markers->at(channel);
    auto it = Tools::findFirstAt(channelResults, globalRange, clipTimeStart);
    while(it != channelResults.cend() && std::get<0>(*it) < clipTimeEnd)
    {
        auto const start = std::get<0>(*it);
        auto const x1 = Tools::secondsToPixel(start, timeRange, fbounds);
        auto timeLimit = std::min(std::get<0>(*it) + std::get<1>(*it) + timeEpsilon, clipTimeEnd);
        // Skip any adjacent result with a distance inferior to epsilon pixels
        auto next = std::next(it);
        while(next != channelResults.cend() && std::get<0>(*next) < timeLimit)
        {
            it = std::exchange(next, std::next(next));
            timeLimit = std::min(std::get<0>(*it) + std::get<1>(*it) + timeEpsilon, clipTimeEnd);
        }

        auto const end = std::get<0>(*it) + std::get<1>(*it);
        auto const x2 = Tools::secondsToPixel(end, timeRange, fbounds);
        auto const w = Tools::secondsToPixel(end, timeRange, fbounds) - x1;
        rectangles.addWithoutMerging({x1, fbounds.getY(), std::max(w, 1.0f), fbounds.getHeight()});

        if(showLabel && !std::get<2>(*it).empty() && (labels.empty() || static_cast<float>(std::get<1>(labels.back()) + std::get<2>(labels.back())) <= x2))
        {
            auto const text = std::get<2>(*it) + unit;
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
            g.drawSingleLineText(std::get<0>(label), std::get<1>(label), bounds.getY() + 22, juce::Justification::left);
        }
    }
}

void Track::Plot::paintPoints(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr)
{
    auto const results = accessor.getAttr<AttrType::results>();
    if(results.isEmpty())
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

    auto const& globalRange = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
    auto const& timeRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    if(globalRange.isEmpty() || timeRange.isEmpty())
    {
        return;
    }

    auto const& valueRange = accessor.getAcsr<AcsrType::valueZoom>().getAttr<Zoom::AttrType::visibleRange>();
    anlWeakAssert(!valueRange.isEmpty());
    if(valueRange.isEmpty())
    {
        return;
    }

    auto const fbounds = bounds.toFloat();
    auto const& colours = accessor.getAttr<AttrType::colours>();
    auto const& unit = accessor.getAttr<AttrType::description>().output.unit;

    auto const font = g.getCurrentFont();
    auto const fontAscent = font.getAscent();
    auto const fontDescent = font.getDescent();

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
    auto const charWidth = font.getStringWidthFloat("0");
    auto const unitWidth = font.getStringWidthFloat(unit);
    auto const getTextValue = [numDecimals](float value)
    {
        if(numDecimals == 0)
        {
            return juce::String(static_cast<int>(value));
        }
        return juce::String(value, numDecimals).trimCharactersAtEnd("0").trimCharactersAtEnd(".");
    };

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
                g.drawSingleLineText(getTextValue(value) + unit, 4, static_cast<int>(y - fontDescent) - 2, juce::Justification::left);
            }
        }
        return;
    }

    auto it = Tools::findFirstAt(channelResults, globalRange, clipTimeStart);
    if(it != channelResults.cbegin())
    {
        it = std::prev(it);
    }

    // Time distance corresponding to epsilon pixels
    auto const timeEpsilon = static_cast<double>(epsilonPixel) * timeRange.getLength() / static_cast<double>(bounds.getWidth());

    juce::Path path;
    juce::RectangleList<int> rectangles;
    using labelInfo = std::tuple<juce::String, int, int, float>;
    labelInfo labelInfoLow;
    labelInfo labelInfoHigh;
    juce::GlyphArrangement glyphArr;
    auto const insertLabel = [&](int x, float y, float value)
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
            glyphArr.addLineOfText(font, std::get<0>(labelInfoHigh), static_cast<float>(std::get<1>(labelInfoHigh)), std::get<3>(labelInfoHigh) - fontDescent + 2.0f);
            std::get<0>(labelInfoHigh).clear();
        }
        if(std::get<0>(labelInfoHigh).isEmpty() || y < std::get<3>(labelInfoHigh))
        {
            auto const text = getTextValue(value);
            auto const textWidth = text.length() * charWidth + unitWidth + 2;
            labelInfoHigh = std::make_tuple(text + unit, x, textWidth, y);
        }

        if(canInsertLow)
        {
            if(labelInfoLow == labelInfoHigh)
            {
                std::get<0>(labelInfoHigh).clear();
            }
            glyphArr.addLineOfText(font, std::get<0>(labelInfoLow), static_cast<float>(std::get<1>(labelInfoLow)), std::get<3>(labelInfoLow) + fontAscent + 2.0f);
            std::get<0>(labelInfoLow).clear();
        }
        if(std::get<0>(labelInfoLow).isEmpty() || y > std::get<3>(labelInfoLow))
        {
            auto const text = getTextValue(value);
            auto const textWidth = text.length() * charWidth + unitWidth + 2;
            labelInfoLow = std::make_tuple(text + unit, x, textWidth, y);
        }
    };

    auto getNextItBeforeTime = [](decltype(it) _start, decltype(it) _end, double const& l)
    {
        float min = *std::get<2>(*_start);
        float max = min;
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
    auto shouldStartSubPath = true;
    auto hasExceededEnd = false;
    while(!hasExceededEnd && it != channelResults.cend())
    {
        if(!std::get<2>(*it).has_value())
        {
            shouldStartSubPath = true;
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
            if(it != next)
            {
                auto const nend = std::get<0>(*next) + std::get<1>(*next);
                auto const x2 = static_cast<int>(Tools::secondsToPixel(nend, timeRange, fbounds));
                auto const y1 = static_cast<int>(Tools::valueToPixel(min, valueRange, fbounds));
                auto const y2 = static_cast<int>(Tools::valueToPixel(max, valueRange, fbounds));
                if(!std::exchange(shouldStartSubPath, true))
                {
                    path.lineTo(x, y1);
                }

                rectangles.addWithoutMerging({static_cast<int>(x), y2, std::max(x2 - static_cast<int>(x), 1), std::max(y1 - y2, 1)});
                if(showLabel)
                {
                    insertLabel(static_cast<int>(x), y1, min);
                    insertLabel(static_cast<int>(x), y2, max);
                }

                hasExceededEnd = nend >= clipTimeEnd;
                it = next;
            }
            else
            {
                auto const y = Tools::valueToPixel(value, valueRange, fbounds);
                if(showLabel)
                {
                    insertLabel(static_cast<int>(x), y, value);
                }
                if(std::exchange(shouldStartSubPath, false))
                {
                    path.startNewSubPath(x, y);
                }
                else
                {
                    path.lineTo(x, y);
                }
                if(std::get<1>(*it) > 0.0)
                {
                    auto const x2 = Tools::secondsToPixel(end, timeRange, fbounds);
                    path.lineTo(x2, y);
                }
                it = std::next(it);
                hasExceededEnd = end >= clipTimeEnd;
            }
        }
        else
        {
            auto const value = *std::get<2>(*it);
            auto const x = Tools::secondsToPixel(std::get<0>(*it), timeRange, fbounds);
            auto const y = Tools::valueToPixel(value, valueRange, fbounds);
            if(showLabel)
            {
                insertLabel(static_cast<int>(x), y, value);
            }
            if(std::exchange(shouldStartSubPath, false))
            {
                path.startNewSubPath(x, y);
            }
            else
            {
                path.lineTo(x, y);
            }
            auto const end = std::get<0>(*it) + std::get<1>(*it);
            auto const x2 = Tools::secondsToPixel(end, timeRange, fbounds);
            path.lineTo(x2, y);
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
        glyphArr.draw(g);
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
, mTransportPlayheadBar(plot.mTransportAccessor, mPlot.mTimeZoomAccessor)
{
    addAndMakeVisible(mGrid);
    addAndMakeVisible(mPlot);
    addAndMakeVisible(mTransportPlayheadBar);
    mTransportPlayheadBar.setInterceptsMouseClicks(false, false);
    addMouseListener(&mTransportPlayheadBar, false);
    setInterceptsMouseClicks(true, true);

    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::key:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::graphics:
            case AttrType::warnings:
            case AttrType::focused:
            case AttrType::processing:
                break;
            case AttrType::description:
            case AttrType::results:
            {
                removeChildComponent(&mGrid);
                removeChildComponent(&mPlot);
                removeChildComponent(&mTransportPlayheadBar);
                switch(Tools::getDisplayType(mAccessor))
                {
                    case Tools::DisplayType::markers:
                    {
                        addAndMakeVisible(mPlot);
                    }
                    break;
                    case Tools::DisplayType::points:
                    {
                        addAndMakeVisible(mGrid);
                        addAndMakeVisible(mPlot);
                    }
                    break;
                    case Tools::DisplayType::columns:
                    {
                        addAndMakeVisible(mPlot);
                        addAndMakeVisible(mGrid);
                    }
                    break;
                }
                addAndMakeVisible(mTransportPlayheadBar);
            }
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
    mGrid.setBounds(bounds);
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

    auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, pt.x);
    auto const tip = Tools::getValueTootip(mAccessor, mTimeZoomAccessor, *this, pt.y, time);
    setTooltip(Format::secondsToString(time) + ": " + (tip.isEmpty() ? "-" : tip));
}

ANALYSE_FILE_END
