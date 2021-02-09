#include "AnlPlotRenderer.h"

ANALYSE_FILE_BEGIN

void Plot::Renderer::paint(juce::Graphics& g, juce::Rectangle<int> const& bounds, juce::Colour const& colour, Plugin::Output const& output, std::vector<Plugin::Result> const& results, Zoom::Range const& valueRange, double time)
{
    if(results.empty() || bounds.isEmpty() || valueRange.isEmpty())
    {
        return;
    }
    
    enum class DisplayMode
    {
          unsupported
        , surface
        , bar
        , segment
        , matrix
    };
    
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
        anlWeakAssert(!valueRange.isEmpty());
        auto const verticalRange = bounds.getVerticalRange();
        anlWeakAssert(!verticalRange.isEmpty());
        if(valueRange.isEmpty() || verticalRange.isEmpty())
        {
            return 0;
        }
          
        value = (value - static_cast<float>(valueRange.getStart())) / static_cast<float>(valueRange.getLength());
        value = (1.0f - value) * static_cast<float>(verticalRange.getLength()) + static_cast<float>(verticalRange.getStart());
        return static_cast<int>(std::floor(value));
    };
    
    auto const displayMode = getDisplayMode();
    switch(displayMode)
    {
        case DisplayMode::unsupported:
            break;
        case DisplayMode::surface:
        {
            auto const realTime = Vamp::RealTime::fromSeconds(time);
            auto it = std::find_if(results.cbegin(), results.cend(), [&](Plugin::Result const& result)
            {
                anlWeakAssert(result.hasTimestamp);
                return realTime >= result.timestamp && realTime < result.timestamp + result.duration;
            });
            
            if(it != results.cend())
            {
                g.fillAll(colour);
            }
        }
            break;
        case DisplayMode::bar:
        {
            auto const realTime = Vamp::RealTime::fromSeconds(time);
            auto it = std::find_if(results.cbegin(), results.cend(), [&](Plugin::Result const& result)
            {
                anlWeakAssert(result.hasTimestamp);
                return realTime >= result.timestamp && realTime < result.timestamp + result.duration;
            });
            
            if(it != results.cend())
            {
                anlWeakAssert(!it->values.empty());
                if(it->values.empty())
                {
                    return;
                }
                    
                auto const value = it->values[0];
                
                auto const position = getScaledValue(value);
                auto const area = bounds.withTop(position).toFloat();
                
                g.setColour(colour.withAlpha(0.2f));
                g.fillRect(area);
                
                g.setColour(colour);
                g.fillRect(area.withHeight(1.0f));
            }
        }
            break;
        case DisplayMode::segment:
        {
            auto const realTime = Vamp::RealTime::fromSeconds(time);
            auto it = std::lower_bound(results.cbegin(), results.cend(), realTime, [](auto const& result, auto const& t)
            {
                return result.timestamp < t;
            });
            if(it != results.cend())
            {
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
                auto const position = getScaledValue(value);
                auto const area = bounds.withTop(position).toFloat();
                
                g.setColour(colour.withAlpha(0.2f));
                g.fillRect(area);
                
                g.setColour(colour);
                g.fillRect(area.withHeight(1.0f));
            }
        }
            break;
        case DisplayMode::matrix:
        {
            
        }
            break;
    }
}

juce::Image Plot::Renderer::createImage(std::vector<Plugin::Result> const& results, ColourMap const& colourMap, std::function<bool(void)> predicate)
{
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
    
    auto image = juce::Image(juce::Image::PixelFormat::ARGB, width, height, false);
    juce::Image::BitmapData const data(image, juce::Image::BitmapData::writeOnly);
    
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

ANALYSE_FILE_END
