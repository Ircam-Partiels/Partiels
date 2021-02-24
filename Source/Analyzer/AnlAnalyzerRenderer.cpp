#include "AnlAnalyzerRenderer.h"

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
    
    auto const colourMap = accessor.getAttr<AttrType::colours>().map;
    auto valueToColour = [&](float const value)
    {
        auto const color = tinycolormap::GetColor(static_cast<double>(value) / (height * 0.25), colourMap);
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

Analyzer::Renderer::Renderer(Accessor& accessor, Type type)
: mAccessor(accessor)
, mType(type)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
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
                break;
            case AttrType::colours:
            case AttrType::results:
            {
                if(mProcess.valid())
                {
                    mProcessState = ProcessState::aborted;
                    mProcess.get();
                }
                mProcessState = ProcessState::available;
                
                auto const& results = acsr.getAttr<AttrType::results>();
                if(results.empty() || results[0].values.size() <= 1)
                {
                    mImage = {};
                    if(onUpdated != nullptr)
                    {
                        onUpdated();
                    }
                    return;
                }
                
                auto const witdh = static_cast<int>(results.size());
                auto const height = static_cast<int>(results.empty() ? 0 : results[0].values.size());
                anlWeakAssert(witdh > 0 && height > 0);
                if(witdh < 0 || height < 0)
                {
                    mImage = {};
                    if(onUpdated != nullptr)
                    {
                        onUpdated();
                    }
                    return;
                }
                
                if(mType == Type::frame)
                {
                    mImage = juce::Image(juce::Image::PixelFormat::ARGB, 1, height, false);
                    if(onUpdated != nullptr)
                    {
                        onUpdated();
                    }
                    return;
                }
                
                mProcess = std::async([this]() -> juce::Image
                {
                    juce::Thread::setCurrentThreadName("Analyzer::Renderer::Process");
                    auto expected = ProcessState::available;
                    if(!mProcessState.compare_exchange_weak(expected, ProcessState::running))
                    {
                        triggerAsyncUpdate();
                        return {};
                    }
                    
                    auto image = createImage(mAccessor, [this]()
                    {
                        return mProcessState.load() != ProcessState::aborted;
                    });
                    
                    expected = ProcessState::running;
                    if(mProcessState.compare_exchange_weak(expected, ProcessState::ended))
                    {
                        triggerAsyncUpdate();
                        return image;
                    }
                    triggerAsyncUpdate();
                    return {};
                });
            };
                break;
            case AttrType::time:
            case AttrType::warnings:
            case AttrType::processing:
                break;
        }
    };
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Analyzer::Renderer::~Renderer()
{
    mAccessor.removeListener(mListener);
    if(mProcess.valid())
    {
        mProcessState = ProcessState::aborted;
        mProcess.get();
    }
}

void Analyzer::Renderer::handleAsyncUpdate()
{
    if(mProcess.valid())
    {
        auto expected = ProcessState::ended;
        if(mProcessState.compare_exchange_weak(expected, ProcessState::available))
        {
            mImage = mProcess.get();
            if(onUpdated != nullptr)
            {
                onUpdated();
            }
        }
        else if(mProcessState == ProcessState::aborted)
        {
            mImage = mProcess.get();
            mProcessState = ProcessState::available;
            if(onUpdated != nullptr)
            {
                onUpdated();
            }
        }
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
    
    g.setColour(colours.background);
    g.fillRect(bounds);
    
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
    
    auto it = Plugin::getResultAt(results, time);
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
            auto image = mImage;
            if(!image.isValid())
            {
                return;
            }
            
            g.setColour(colours.background.withAlpha(1.0f));
            juce::Image::BitmapData const data(image, juce::Image::BitmapData::writeOnly);
            
            auto const colourMap = colours.map;
            auto valueToColour = [&](float const value)
            {
                auto const color = tinycolormap::GetColor(static_cast<double>(value) / (image.getHeight() * 0.25), colourMap);
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

            auto const transform = juce::AffineTransform::translation(0, -deltaY).scaled(width, scaleY).translated(bounds.getX(), bounds.getY());

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
    auto const& colours = mAccessor.getAttr<AttrType::colours>();
    auto const& results = mAccessor.getAttr<AttrType::results>();
    
    g.setColour(colours.background);
    g.fillRect(bounds);
    
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
        auto image = mImage;
        if(!image.isValid())
        {
            return;
        }
        g.setColour(colours.background.withAlpha(1.0f));
        auto const& binZoomAcsr = mAccessor.getAccessor<AcsrType::binZoom>(0);
        auto const binVisibleRange = binZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
        auto const binGlobalRange = binZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
        
        auto const globalTimeRange = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
        
        auto const valueRange = juce::Range<double>(binGlobalRange.getEnd() - binVisibleRange.getEnd(), binGlobalRange.getEnd() - binVisibleRange.getStart());
        
        auto const deltaX = static_cast<float>(timeRange.getStart() / globalTimeRange.getLength() * static_cast<double>(image.getWidth()));
        auto const deltaY = static_cast<float>(valueRange.getStart() / binGlobalRange.getLength() * static_cast<double>(image.getHeight()));
        
        auto const scaleX = static_cast<float>(globalTimeRange.getLength() / timeRange.getLength() * static_cast<double>(width) / static_cast<double>(image.getWidth()));
        auto const scaleY = static_cast<float>(binGlobalRange.getLength() / valueRange.getLength() * static_cast<double>(height) / static_cast<double>(image.getHeight()));
        
        auto const transform = juce::AffineTransform::translation(-deltaX, -deltaY).scaled(scaleX, scaleY).translated(bounds.getX(), bounds.getY());
        
        g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
        g.drawImageTransformed(image, transform);
    }
}

ANALYSE_FILE_END
