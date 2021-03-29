#include "AnlTrackPlot.h"
#include "AnlTrackGraphics.h"

ANALYSE_FILE_BEGIN

Track::Plot::Plot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
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
    mAccessor.getAccessor<AcsrType::valueZoom>(0).addListener(mValueZoomListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::binZoom>(0).addListener(mBinZoomListener, NotificationType::synchronous);
    mTimeZoomAccessor.addListener(mTimeZoomListener, NotificationType::synchronous);
}

Track::Plot::~Plot()
{
    mTimeZoomAccessor.removeListener(mTimeZoomListener);
    mAccessor.getAccessor<AcsrType::binZoom>(0).removeListener(mBinZoomListener);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).removeListener(mValueZoomListener);
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
    
    auto const& output = mAccessor.getAttr<AttrType::description>().output;
    auto const& timeRange = mTimeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>();
    auto const& valueRange = mAccessor.getAccessor<AcsrType::valueZoom>(0).getAttr<Zoom::AttrType::visibleRange>();
    if(output.hasFixedBinCount)
    {
        switch(output.binCount)
        {
            case 0:
            {
                g.setColour(mAccessor.getAttr<AttrType::colours>().foreground);
                paintMarkers(g, bounds.toFloat(), *resultsPtr, timeRange);
            }
                break;
            case 1:
            {
                g.setColour(mAccessor.getAttr<AttrType::colours>().foreground);
                paintSegments(g, bounds.toFloat(), *resultsPtr, timeRange, valueRange);
            }
                break;
            default:
            {
                paintGrid(g, bounds, mAccessor.getAttr<AttrType::graphics>(), mTimeZoomAccessor, mAccessor.getAccessor<AcsrType::binZoom>(0));
            }
                break;
        }
    }
    else
    {
        g.setColour(mAccessor.getAttr<AttrType::colours>().foreground);
        paintSegments(g, bounds.toFloat(), *resultsPtr, timeRange, valueRange);
    }
}

void Track::Plot::paintMarkers(juce::Graphics& g, juce::Rectangle<float> const& bounds, std::vector<Plugin::Result> const& results, juce::Range<double> const& timeRange)
{
    auto constexpr epsilonPixel = 2.0f;
    auto const clipBounds = g.getClipBounds().toFloat();
    auto const clipTimeStart = Graphics::pixelToSeconds(clipBounds.getX() - epsilonPixel, timeRange, bounds);
    auto const clipTimeEnd = Graphics::pixelToSeconds(clipBounds.getRight() + epsilonPixel, timeRange, bounds);
    auto const rtStart = Graphics::secondsToRealTime(clipTimeStart);
    auto const rtEnd = Graphics::secondsToRealTime(clipTimeEnd);
    
    // Time distance corresponding to epsilon pixels
    auto const minDiffTime = Graphics::secondsToRealTime(static_cast<double>(epsilonPixel) * timeRange.getLength() / static_cast<double>(bounds.getWidth()));
  
    // Find the first result that is inside the clip bounds
    auto it = std::find_if(results.cbegin(), results.cend(), [&](Plugin::Result const& result)
    {
        return Graphics::getEndRealTime(result) >= rtStart;
    });
    
    juce::RectangleList<float> rectangles;
    while(it != results.cend() && it->timestamp < rtEnd)
    {
        if(it->hasTimestamp)
        {
            auto const start = Graphics::realTimeToSeconds(it->timestamp);
            auto const x = Graphics::secondsToPixel(start, timeRange, bounds);
            
            // Skip any adjacent result with a distance inferior to epsilon pixels
            auto next = std::next(it);
            while(next != results.cend() && next->timestamp < rtEnd && next->hasTimestamp && next->timestamp < Graphics::getEndRealTime(*it) + minDiffTime)
            {
                it = std::exchange(next, std::next(next));
            }
            
            auto const end = Graphics::realTimeToSeconds(Graphics::getEndRealTime(*it));
            auto const w = Graphics::secondsToPixel(end, timeRange, bounds) - x;
            rectangles.add({x, clipBounds.getY(), std::max(w, 1.0f), clipBounds.getHeight()});
        }
        it = std::next(it);
    }
    
    g.fillRectList(rectangles);
}

void Track::Plot::paintSegments(juce::Graphics& g, juce::Rectangle<float> const& bounds, std::vector<Plugin::Result> const& results, juce::Range<double> const& timeRange, juce::Range<double> const& valueRange)
{
    auto constexpr epsilonPixel = 2.0f;
    auto const clipBounds = g.getClipBounds().toFloat();
    auto const clipTimeStart = Graphics::pixelToSeconds(clipBounds.getX() - epsilonPixel, timeRange, bounds);
    auto const clipTimeEnd = Graphics::pixelToSeconds(clipBounds.getRight() + epsilonPixel, timeRange, bounds);
    auto const rtStart = Graphics::secondsToRealTime(clipTimeStart);
    auto const rtEnd = Graphics::secondsToRealTime(clipTimeEnd);
    
    // Time distance corresponding to epsilon pixels
    auto const minDiffTime = Graphics::secondsToRealTime(static_cast<double>(epsilonPixel) * timeRange.getLength() / static_cast<double>(bounds.getWidth()));
    
    // Find the first result that is inside the clip bounds
    auto first = std::find_if(results.cbegin(), results.cend(), [&](Plugin::Result const& result)
    {
        return Graphics::getEndRealTime(result) >= rtStart;
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
        return Graphics::getEndRealTime(result) >= rtEnd;
    });
    if(last != results.cend())
    {
        last = std::next(last);
    }
    
    anlWeakAssert(first != last);
    
    juce::Path path;
    path.preallocateSpace(6 * static_cast<int>(std::distance(first, last)));
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
            auto getRangeElements = [&](decltype(first) input) -> std::tuple<decltype(first), juce::Range<float>>
            {
                juce::Range<float> minmax(input->values[0], input->values[0]);
                // Skip any adjacent result with a distance inferior to epsilon pixels
                auto const limit = Graphics::getEndRealTime(*input) + minDiffTime;
                while(input != last)
                {
                    if(!input->values.empty())
                    {
                        minmax = minmax.getUnionWith(input->values[0]);
                    }
                    auto next = std::next(input);
                    if(!next->hasTimestamp || Graphics::getEndRealTime(*next) > limit)
                    {
                        break;
                    }
                    input = next;
                }
                return {input, minmax};
            };
            
            auto const elements = getRangeElements(first);
            auto const& next = std::get<0>(elements);
            auto const& values = std::get<1>(elements);
            
            auto const start = Graphics::realTimeToSeconds(first->timestamp);
            auto const x = Graphics::secondsToPixel(start, timeRange, bounds);
            auto const y = Graphics::valueToPixel(values.getStart(), valueRange, bounds);
            
            if(std::exchange(shouldStartSubPath, false))
            {
                path.startNewSubPath(x, y);
            }
            else
            {
                path.lineTo(x, y);
            }
            
            if(next == first && first->hasDuration)
            {
                auto const end = Graphics::realTimeToSeconds(Graphics::getEndRealTime(*first));
                auto const x2 = Graphics::secondsToPixel(end, timeRange, bounds);
                path.lineTo(x2, y);
            }
            else if(next != first)
            {
                auto const end = Graphics::realTimeToSeconds(Graphics::getEndRealTime(*next));
                auto const x2 = Graphics::secondsToPixel(end, timeRange, bounds);
                auto const y2 = Graphics::valueToPixel(values.getEnd(), valueRange, bounds);
                path.lineTo(x2, y2);
            }
            first = (next != first) ? next : std::next(first);
        }
    }
    
    g.strokePath(path, juce::PathStrokeType(1));
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

ANALYSE_FILE_END
