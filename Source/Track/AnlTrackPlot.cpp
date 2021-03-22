#include "AnlTrackPlot.h"
#include "AnlTrackResults.h"

ANALYSE_FILE_BEGIN

Track::Plot::Plot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
{
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
    auto const bounds = getLocalBounds();
    
    auto const& valueZoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
    auto const& visibleValueRange = valueZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const resultsPtr = mAccessor.getAttr<AttrType::results>();
    if(resultsPtr == nullptr)
    {
        return;
    }
    
    auto const& results = *resultsPtr;
    if(results.empty() || bounds.isEmpty() || visibleValueRange.isEmpty())
    {
        return;
    }
    
    auto const width = bounds.getWidth();
    auto const height = bounds.getHeight();
    
    auto const timeRange = mTimeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>();
    
    auto const realTimeRange = juce::Range<Vamp::RealTime>{Vamp::RealTime::fromSeconds(timeRange.getStart()), Vamp::RealTime::fromSeconds(timeRange.getEnd())};
    
    auto const timeLength = timeRange.getLength();
    auto timeToPixel = [&](Vamp::RealTime const&  timestamp)
    {
        auto const time = (((timestamp.sec * 1000.0 + timestamp.msec()) / 1000.0) - timeRange.getStart()) / timeLength;
        return static_cast<int>(time * width);
    };
    
    auto const resultRatio = static_cast<double>(results.size()) / static_cast<double>(width);
    auto const resultIncrement = static_cast<size_t>(std::floor(std::max(resultRatio, 1.0)));
    
    if(results.front().values.empty())
    {
        g.setColour(mAccessor.getAttr<AttrType::colours>().foreground);
        for(size_t i = 0; i < results.size(); i += resultIncrement)
        {
            if(realTimeRange.contains(results[i].timestamp))
            {
                auto const x = timeToPixel(results[i].timestamp);
                g.drawVerticalLine(x, static_cast<float>(bounds.getY()), static_cast<float>(bounds.getBottom()));
            }
            else if(results[i].timestamp >= realTimeRange.getEnd())
            {
                break;
            }
        }
    }
    else if(results.front().values.size() == 1)
    {
        auto const clip = g.getClipBounds();
        g.setColour(mAccessor.getAttr<AttrType::colours>().foreground);
        auto const valueRange = mAccessor.getAccessor<AcsrType::valueZoom>(0).getAttr<Zoom::AttrType::visibleRange>();
        auto valueToPixel = [&](float const value)
        {
            auto const ratio = 1.0f - (value - valueRange.getStart()) / valueRange.getLength();
            return static_cast<int>(ratio * height);
        };
        juce::Point<float> pt;
        
        for(size_t i = 0; i < results.size(); i += resultIncrement)
        {
            auto const next = i + resultIncrement;
            auto const isVisible = realTimeRange.contains(results[i].timestamp) || (next < results.size() && realTimeRange.contains(results[next].timestamp));
            
            auto const x = timeToPixel(results[i].timestamp);
            auto const y = valueToPixel(results[i].values[0]);
            juce::Point<float> const npt{static_cast<float>(x), static_cast<float>(y)};
            if(x >= clip.getX() && x <= clip.getRight() && isVisible && i > 0)
            {
                g.drawLine({pt, npt});
            }
            else if(results[i].timestamp >= realTimeRange.getEnd() || x > clip.getRight())
            {
                break;
            }
            pt = npt;
        }
    }
    else
    {
        auto const images = mAccessor.getAttr<AttrType::graphics>();
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
        
        auto const timezoomRatio = getZoomRatio(mTimeZoomAccessor);
        auto const binZoomRatio = getZoomRatio(mAccessor.getAccessor<AcsrType::binZoom>(0));
        auto const boundsDimension = std::max(bounds.getWidth() * timezoomRatio, bounds.getHeight() * binZoomRatio);
        for(auto const& image : images)
        {
            auto const imageDimension = std::max(image.getWidth(), image.getHeight());
            if(imageDimension >= boundsDimension)
            {
                renderImage(image, mTimeZoomAccessor, mAccessor.getAccessor<AcsrType::binZoom>(0));
                return;
            }
        }
        renderImage(images.back(), mTimeZoomAccessor, mAccessor.getAccessor<AcsrType::binZoom>(0));
    }
}

ANALYSE_FILE_END
