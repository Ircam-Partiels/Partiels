#include "AnlAnalyzerRenderer.h"
#include "AnlAnalyzerResults.h"

ANALYSE_FILE_BEGIN

juce::Image Analyzer::Renderer::createImage(Accessor const& accessor, std::function<bool(void)> predicate)
{
    auto const& results = accessor.getAttr<AttrType::results>();
    if(results.empty())
    {
        return {};
    }
    
    auto const width = static_cast<int>(results.size());
    auto const height = static_cast<int>(results.front().values.size());
    anlWeakAssert(width > 0 && height > 0);
    if(width <= 0 || height <= 0)
    {
        return {};
    }
    
    if(predicate == nullptr)
    {
        predicate = []()
        {
            return true;
        };
    }
    
    auto image = juce::Image(juce::Image::PixelFormat::ARGB, width, height, false);
    juce::Image::BitmapData const data(image, juce::Image::BitmapData::writeOnly);
    
    auto const& valueZoomAcsr = accessor.getAccessor<AcsrType::valueZoom>(0);
    auto const valueRange = valueZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const colourMap = accessor.getAttr<AttrType::colours>().map;
    auto valueToColour = [&](float const value)
    {
        auto const ratio = (value - valueRange.getStart()) / valueRange.getLength();
        auto const color = tinycolormap::GetColor(static_cast<double>(ratio), colourMap);
        return juce::Colour::fromFloatRGBA(static_cast<float>(color.r()), static_cast<float>(color.g()), static_cast<float>(color.b()), 1.0f);
    };
    
    for(int i = 0; i < width && predicate(); ++i)
    {
        for(int j = 0; j < height && predicate(); ++j)
        {
            auto const colour = valueToColour(results[static_cast<size_t>(i)].values[static_cast<size_t>(j)]);
            data.setPixelColour(i, height - 1 - j, colour);
        }
    }
    return predicate() ? image : juce::Image();
}

void Analyzer::Renderer::renderImage(juce::Graphics& g, juce::Rectangle<int> const& bounds, juce::Image const& image, Zoom::Accessor const& xZoomAcsr, Zoom::Accessor const& yZoomAcsr)
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
}

Analyzer::Renderer::Renderer(Accessor& accessor, Type type)
: mAccessor(accessor)
, mType(type)
{
}

Analyzer::Renderer::~Renderer()
{
    if(mProcess.valid())
    {
        mProcessState = ProcessState::aborted;
        mProcess.get();
    }
}

bool Analyzer::Renderer::isPreparing() const
{
    return mProcess.valid();
}

void Analyzer::Renderer::prepareRendering()
{
    if(mProcess.valid())
    {
        mProcessState = ProcessState::aborted;
        mProcess.get();
        cancelPendingUpdate();
    }
    mProcessState = ProcessState::available;
    
    auto const& results = mAccessor.getAttr<AttrType::results>();
    if(results.empty() || results[0].values.size() <= 1)
    {
        mImages.clear();
        if(onUpdated != nullptr)
        {
            onUpdated();
        }
        return;
    }
    
    auto const width = static_cast<int>(results.size());
    auto const height = static_cast<int>(results.empty() ? 0 : results[0].values.size());
    anlWeakAssert(width > 0 && height > 0);
    if(width < 0 || height < 0)
    {
        mImages.clear();
        if(onUpdated != nullptr)
        {
            onUpdated();
        }
        return;
    }
    
    if(mType == Type::frame)
    {
        mImages = {juce::Image(juce::Image::PixelFormat::ARGB, 1, height, false)};
        if(onUpdated != nullptr)
        {
            onUpdated();
        }
        return;
    }
    mRenderingStartTime = juce::Time::getCurrentTime();
    
    anlDebug("Analyzer", "rendering launched");
    mProcess = std::async([this]() -> juce::Image
    {
        juce::Thread::setCurrentThreadName("Analyzer::Renderer::Process");
        juce::Thread::setCurrentThreadPriority(10);
        auto expected = ProcessState::available;
        if(!mProcessState.compare_exchange_weak(expected, ProcessState::running))
        {
            triggerAsyncUpdate();
            return {};
        }
        
        if(!mAccessor.acquireResultsReadingAccess())
        {
            mProcessState = ProcessState::aborted;
            triggerAsyncUpdate();
            return {};
        }
        auto image = createImage(mAccessor, [this]()
        {
            return mAccessor.canContinueToReadResults() && mProcessState.load() != ProcessState::aborted;
        });
        mAccessor.releaseResultsReadingAccess();
        if(image.isNull())
        {
            mProcessState = ProcessState::aborted;
            triggerAsyncUpdate();
            return {};
        }
        
        expected = ProcessState::running;
        if(mProcessState.compare_exchange_weak(expected, ProcessState::ended))
        {
            triggerAsyncUpdate();
            return image;
        }
        triggerAsyncUpdate();
        return {};
    });
    
    if(onUpdated != nullptr)
    {
        onUpdated();
    }
}

void Analyzer::Renderer::handleAsyncUpdate()
{
    if(mProcess.valid())
    {
        anlWeakAssert(mProcessState != ProcessState::available);
        anlWeakAssert(mProcessState != ProcessState::running);
        
        auto expected = ProcessState::ended;
        if(mProcessState.compare_exchange_weak(expected, ProcessState::available))
        {
            auto const now = juce::Time::getCurrentTime();
            anlDebug("Analyzer", "rendering succeeded (" + (now - mRenderingStartTime).getDescription() + ")");
            auto constexpr maxSize = 4096;
            auto image = mProcess.get();
            auto const dimension = std::max(image.getWidth(), image.getHeight());
            for(int i = maxSize; i < dimension; i *= 2)
            {
                mImages.push_back(image.rescaled(std::min(i, image.getWidth()), std::min(i, image.getHeight())));
            }
            mImages.push_back(image);
        }
        expected = ProcessState::aborted;
        if(mProcessState.compare_exchange_weak(expected, ProcessState::available))
        {
            mImages.clear();
        }
    }
    
    if(onUpdated != nullptr)
    {
        onUpdated();
    }
}

void Analyzer::Renderer::paint(juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr)
{
    switch(mType)
    {
        case Type::frame:
            paintFrame(g, bounds);
            break;
        case Type::range:
            paintRange(g, bounds, timeZoomAcsr);
            break;
    }
}

void Analyzer::Renderer::paintFrame(juce::Graphics& g, juce::Rectangle<int> const& bounds)
{
    auto const& valueZoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
    auto const& visibleValueRange = valueZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const& colours = mAccessor.getAttr<AttrType::colours>();
    auto const& output = mAccessor.getAttr<AttrType::description>().output;
    auto const& results = mAccessor.getAttr<AttrType::results>();
    auto const time = mAccessor.getAttr<AttrType::time>();
    
    if(results.empty() || bounds.isEmpty() || visibleValueRange.isEmpty())
    {
        return;
    }
    
    auto getDisplayMode = [&]()
    {
        auto const numDimensions = output.hasFixedBinCount ? output.binCount : results.front().values.size();
        switch(numDimensions)
        {
            case 0:
                return output.hasDuration ? DisplayMode::surface : DisplayMode::unsupported;
            case 1:
                return output.hasDuration ? DisplayMode::bar : DisplayMode::segment;
            default:
                return DisplayMode::matrix;
        }
    };
    
    auto getScaledValue = [&](float value)
    {
        anlWeakAssert(!visibleValueRange.isEmpty());
        auto const verticalRange = bounds.getVerticalRange();
        anlWeakAssert(!verticalRange.isEmpty());
        if(visibleValueRange.isEmpty() || verticalRange.isEmpty())
        {
            return 0;
        }
        
        value = (value - static_cast<float>(visibleValueRange.getStart())) / static_cast<float>(visibleValueRange.getLength());
        value = (1.0f - value) * static_cast<float>(verticalRange.getLength()) + static_cast<float>(verticalRange.getStart());
        return static_cast<int>(std::floor(value));
    };
    
    auto it = Results::getResultAt(results, time);
    if(it == results.cend())
    {
        return;
    }
    auto const displayMode = getDisplayMode();
    switch(displayMode)
    {
        case DisplayMode::unsupported:
            break;
        case DisplayMode::surface:
        {
            g.setColour(colours.foreground);
            g.fillRect(bounds);
        }
            break;
        case DisplayMode::bar:
        {
            anlWeakAssert(!it->values.empty());
            if(it->values.empty())
            {
                return;
            }
            
            auto const area = bounds.withTop(getScaledValue(it->values[0])).toFloat();
            g.setColour(colours.foreground.withAlpha(0.2f));
            g.fillRect(area);
            
            g.setColour(colours.foreground);
            g.fillRect(area.withHeight(1.0f));
        }
            break;
        case DisplayMode::segment:
        {
            auto const realTime = Vamp::RealTime::fromSeconds(time);
            anlWeakAssert(!it->values.empty());
            if(it->values.empty())
            {
                return;
            }
            
            auto getValue = [&]()
            {
                if(it != results.cbegin())
                {
                    auto const prev = std::prev(it, 1);
                    auto const ratio = static_cast<float>((realTime - prev->timestamp) / (it->timestamp - prev->timestamp));
                    return prev->values[0] * (1.0f - ratio) + it->values[0] * ratio;
                }
                return it->values[0];
            };
            
            auto const value = getValue();
            auto const area = bounds.withTop(getScaledValue(value)).toFloat();
            
            g.setColour(colours.foreground.withAlpha(0.2f));
            g.fillRect(area);
            
            g.setColour(colours.foreground);
            g.fillRect(area.withHeight(1.0f));
        }
            break;
        case DisplayMode::matrix:
        {
            auto image = mImages.empty() ? juce::Image() : mImages.front();
            if(!image.isValid())
            {
                return;
            }
            
            juce::Image::BitmapData const data(image, juce::Image::BitmapData::writeOnly);
            
            auto const colourMap = colours.map;
            auto valueToColour = [&](float const value)
            {
                auto const& valueRange = valueZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
                auto const ratio = (value - valueRange.getStart()) / valueRange.getLength();
                auto const color = tinycolormap::GetColor(static_cast<double>(ratio), colourMap);
                return juce::Colour::fromFloatRGBA(static_cast<float>(color.r()), static_cast<float>(color.g()), static_cast<float>(color.b()), 1.0f);
            };
            
            for(int j = 0; j < image.getHeight(); ++j)
            {
                auto const colour = valueToColour(it->values[static_cast<size_t>(j)]);
                data.setPixelColour(0, image.getHeight() - 1 - j, colour);
            }

            auto const width = static_cast<float>(bounds.getWidth());
            auto const height = bounds.getHeight();

            auto const& binZoomAcsr = mAccessor.getAccessor<AcsrType::binZoom>(0);
            auto const binVisibleRange = binZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            auto const binGlobalRange = binZoomAcsr.getAttr<Zoom::AttrType::globalRange>();

            auto const valueRange = juce::Range<double>(binGlobalRange.getEnd() - binVisibleRange.getEnd(), binGlobalRange.getEnd() - binVisibleRange.getStart());

            auto const deltaY = static_cast<float>(valueRange.getStart() / binGlobalRange.getLength() * static_cast<double>(image.getHeight()));
            auto const scaleY = static_cast<float>(binGlobalRange.getLength() / valueRange.getLength() * static_cast<double>(height) / static_cast<double>(image.getHeight()));

            auto const transform = juce::AffineTransform::translation(0.0f, -deltaY).scaled(width, scaleY).translated(static_cast<float>(bounds.getX()), static_cast<float>(bounds.getY()));

            g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
            g.drawImageTransformed(image, transform);
        }
            break;
    }
}

void Analyzer::Renderer::paintRange(juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr)
{
    auto const& valueZoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
    auto const& visibleValueRange = valueZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const& results = mAccessor.getAttr<AttrType::results>();
    
    if(results.empty() || bounds.isEmpty() || visibleValueRange.isEmpty())
    {
        return;
    }
    
    auto const width = bounds.getWidth();
    auto const height = bounds.getHeight();
    
    auto const timeRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    
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
        if(mImages.empty())
        {
            return;
        }
        
        auto getZoomRatio = [](Zoom::Accessor const& acsr)
        {
            return acsr.getAttr<Zoom::AttrType::globalRange>().getLength() / acsr.getAttr<Zoom::AttrType::visibleRange>().getLength();
        };
        
        auto const timezoomRatio = getZoomRatio(timeZoomAcsr);
        auto const binZoomRatio = getZoomRatio(mAccessor.getAccessor<AcsrType::binZoom>(0));
        auto const boundsDimension = std::max(bounds.getWidth() * timezoomRatio, bounds.getHeight() * binZoomRatio);
        for(auto const& image : mImages)
        {
            auto const imageDimension = std::max(image.getWidth(), image.getHeight());
            if(imageDimension >= boundsDimension)
            {
                renderImage(g, bounds, image, timeZoomAcsr, mAccessor.getAccessor<AcsrType::binZoom>(0));
                return;
            }
        }
        renderImage(g, bounds, mImages.back(), timeZoomAcsr, mAccessor.getAccessor<AcsrType::binZoom>(0));
    }
}

ANALYSE_FILE_END
