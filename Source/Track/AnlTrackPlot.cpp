#include "AnlTrackPlot.h"
#include "AnlTrackTools.h"
#include "../Zoom/AnlZoomTools.h"

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
            case AttrType::propertyState:
            case AttrType::zoomLink:
            case AttrType::warnings:
            case AttrType::time:
            case AttrType::processing:
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
    if(bounds.isEmpty())
    {
        return;
    }
    
    auto const& timeRange = mTimeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>();
    auto const& valueRange = mAccessor.getAcsr<AcsrType::valueZoom>().getAttr<Zoom::AttrType::visibleRange>();
    switch(Tools::getDisplayType(mAccessor))
    {
        case Tools::DisplayType::markers:
        {
            paintMarkers(g, bounds.toFloat(), mAccessor.getAttr<AttrType::colours>().foreground, *resultsPtr, timeRange);
        }
            break;
        case Tools::DisplayType::segments:
        {
            paintSegments(g, bounds.toFloat(), mAccessor.getAttr<AttrType::colours>().foreground, *resultsPtr, timeRange, valueRange);
        }
            break;
        case Tools::DisplayType::grid:
        {
            paintGrid(g, bounds, mAccessor.getAttr<AttrType::graphics>(), mTimeZoomAccessor, mAccessor.getAcsr<AcsrType::binZoom>());
        }
            break;
    }
}

void Track::Plot::paintMarkers(juce::Graphics& g, juce::Rectangle<float> const& bounds, juce::Colour const& colour, std::vector<Plugin::Result> const& results, juce::Range<double> const& timeRange)
{
    auto constexpr epsilonPixel = 2.0f;
    auto const clipBounds = g.getClipBounds().toFloat();
    auto const clipTimeStart = Tools::pixelToSeconds(clipBounds.getX() - epsilonPixel, timeRange, bounds);
    auto const clipTimeEnd = Tools::pixelToSeconds(clipBounds.getRight() + epsilonPixel, timeRange, bounds);
    auto const rtStart = Tools::secondsToRealTime(clipTimeStart);
    auto const rtEnd = Tools::secondsToRealTime(clipTimeEnd);
    
    // Time distance corresponding to epsilon pixels
    auto const minDiffTime = Tools::secondsToRealTime(static_cast<double>(epsilonPixel) * timeRange.getLength() / static_cast<double>(bounds.getWidth()));
  
    // Find the first result that is inside the clip bounds
    auto it = std::find_if(results.cbegin(), results.cend(), [&](Plugin::Result const& result)
    {
        return Tools::getEndRealTime(result) >= rtStart;
    });
    
    juce::RectangleList<float> rectangles;
    while(it != results.cend() && it->timestamp < rtEnd)
    {
        if(it->hasTimestamp)
        {
            auto const start = Tools::realTimeToSeconds(it->timestamp);
            auto const x = Tools::secondsToPixel(start, timeRange, bounds);
            
            // Skip any adjacent result with a distance inferior to epsilon pixels
            auto next = std::next(it);
            while(next != results.cend() && next->timestamp < rtEnd && next->hasTimestamp && next->timestamp < Tools::getEndRealTime(*it) + minDiffTime)
            {
                it = std::exchange(next, std::next(next));
            }
            
            auto const end = Tools::realTimeToSeconds(Tools::getEndRealTime(*it));
            auto const w = Tools::secondsToPixel(end, timeRange, bounds) - x;
            rectangles.add({x, clipBounds.getY(), std::max(w, 1.0f), clipBounds.getHeight()});
        }
        it = std::next(it);
    }
    
    // Shadow
    {
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        rectangles.offsetAll(-2.0f, 0.0f);
        g.fillRectList(rectangles);
        rectangles.offsetAll(2.0f, 0.0f);
        
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        rectangles.offsetAll(-1.0f, 0.0f);
        g.fillRectList(rectangles);
        rectangles.offsetAll(1.0f, 0.0f);
    }
    g.setColour(colour);
    g.fillRectList(rectangles);
}

void Track::Plot::paintSegments(juce::Graphics& g, juce::Rectangle<float> const& bounds, juce::Colour const& colour, std::vector<Plugin::Result> const& results, juce::Range<double> const& timeRange, juce::Range<double> const& valueRange)
{
    auto constexpr epsilonPixel = 2.0f;
    auto const clipBounds = g.getClipBounds().toFloat();
    auto const clipTimeStart = Tools::pixelToSeconds(clipBounds.getX() - epsilonPixel, timeRange, bounds);
    auto const clipTimeEnd = Tools::pixelToSeconds(clipBounds.getRight() + epsilonPixel, timeRange, bounds);
    auto const rtStart = Tools::secondsToRealTime(clipTimeStart);
    auto const rtEnd = Tools::secondsToRealTime(clipTimeEnd);
    
    // Find the first result that is inside the clip bounds
    auto first = std::find_if(results.cbegin(), results.cend(), [&](Plugin::Result const& result)
    {
        return result.hasTimestamp && Tools::getEndRealTime(result) >= rtStart;
    });
    if(first == results.cend())
    {
        first = std::prev(results.cend());
    }
    else if(first->timestamp > rtStart && first != results.cbegin())
    {
        first = std::prev(first);
    }
    
    // Find the last result that is inside the clip bounds
    auto last = std::find_if(first, results.cend(), [&](Plugin::Result const& result)
    {
        return Tools::getEndRealTime(result) >= rtEnd;
    });
    if(last != results.cend())
    {
        last = std::next(last);
    }
    auto const distance = static_cast<int>(std::distance(first, last));
    
    
    // Time distance corresponding to epsilon pixels
    auto const avoidedTime = Tools::secondsToRealTime(static_cast<double>(epsilonPixel) * timeRange.getLength() / static_cast<double>(bounds.getWidth()) * static_cast<double>(distance) / 1024.0);
    
    anlWeakAssert(first != last);
    
    juce::Path path;
    path.preallocateSpace(3 * static_cast<int>(std::distance(first, last)));
    bool shouldStartSubPath = true;
    while(first != last)
    {
        if(!first->hasTimestamp)
        {
            first = std::next(first);
        }
        else if(first->values.empty())
        {
            shouldStartSubPath = true;
            first = std::next(first);
        }
        else
        {
            auto const startTime = Tools::realTimeToSeconds(first->timestamp);
            auto const x = Tools::secondsToPixel(startTime, timeRange, bounds);
            auto const y = Tools::valueToPixel(first->values[0], valueRange, bounds);
            
            if(std::exchange(shouldStartSubPath, false))
            {
                path.startNewSubPath(x, y);
            }
            else
            {
                path.lineTo(x, y);
            }
            
            if(first->hasDuration && first->duration > avoidedTime)
            {
                auto const end = Tools::realTimeToSeconds(Tools::getEndRealTime(*first));
                auto const x2 = Tools::secondsToPixel(end, timeRange, bounds);
                path.lineTo(x2, y);
                first = std::next(first);
            }
            else
            {
                auto const endTime = first->timestamp + avoidedTime;
                first = std::find_if(std::next(first), last, [&](Plugin::Result const& result)
                {
                    return result.hasTimestamp && Tools::getEndRealTime(result) > endTime;
                });
            }
        }
    }
    
    juce::PathStrokeType pathStrokeType(1.0f);
    pathStrokeType.createStrokedPath(path, path);
    // Shadow
    {
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillPath(path, juce::AffineTransform::translation(0.0f, 2.0f));
        g.setColour(juce::Colours::black.withAlpha(0.75f));
        g.fillPath(path, juce::AffineTransform::translation(1.0f, 1.0f));
    }
    g.setColour(colour);
    g.fillPath(path);
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
            return ZoomRange{globalRange.getEnd() - visibleRange.getEnd() + globalRange.getStart(), globalRange.getEnd() - visibleRange.getStart()+ globalRange.getStart()};
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
, mTransportPlayheadContainer(plot.mTransportAccessor, mPlot.mTimeZoomAccessor)
{
    addAndMakeVisible(mPlot);
    addAndMakeVisible(mTransportPlayheadContainer);
    mTransportPlayheadContainer.setInterceptsMouseClicks(false, false);
    addMouseListener(&mTransportPlayheadContainer, false);
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
            case AttrType::propertyState:
            case AttrType::zoomLink:
            case AttrType::results:
            case AttrType::graphics:
            case AttrType::warnings:
            case AttrType::time:
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
    mTransportPlayheadContainer.setBounds(bounds);
}

void Track::Plot::Overlay::paint(juce::Graphics& g)
{
    g.fillAll(mAccessor.getAttr<AttrType::colours>().background);
}

void Track::Plot::Overlay::mouseMove(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
}

void Track::Plot::Overlay::mouseEnter(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
}

void Track::Plot::Overlay::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    setTooltip("");
}

void Track::Plot::Overlay::updateTooltip(juce::Point<int> const& pt)
{
    if(!getLocalBounds().contains(pt))
    {
        setTooltip("");
        return;
    }
    auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, pt.x);
    auto const bin = Zoom::Tools::getScaledValueFromHeight(mAccessor.getAcsr<AcsrType::binZoom>(), *this, pt.y);
    auto const tip = Tools::getResultText(mAccessor, time, static_cast<size_t>(std::floor(bin)));
    setTooltip(Format::secondsToString(time) + ": " + (tip.isEmpty() ? "-" : tip));
}

ANALYSE_FILE_END
