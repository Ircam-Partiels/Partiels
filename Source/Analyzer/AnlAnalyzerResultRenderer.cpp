#include "AnlAnalyzerResultRenderer.h"
#include "AnlAnalyzerProcessor.h"
#include "../../tinycolormap/include/tinycolormap.hpp"

ANALYSE_FILE_BEGIN

Analyzer::ResultRenderer::ResultRenderer(Accessor& accessor, Zoom::Accessor& zoomAccessor)
: mAccessor(accessor)
, mZoomAccessor(zoomAccessor)
{
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        if(attribute == AttrType::results)
        {
            repaint();
        }
    };
    
    mZoomListener.onChanged = [&](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        repaint();
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAccessors<AttrType::zoom>()[0].get().addListener(mZoomListener, NotificationType::synchronous);
    mZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
}

Analyzer::ResultRenderer::~ResultRenderer()
{
    mZoomAccessor.removeListener(mZoomListener);
    mAccessor.getAccessors<AttrType::zoom>()[0].get().removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Analyzer::ResultRenderer::paint(juce::Graphics& g)
{
    auto const bounds = getLocalBounds();
    auto const width = bounds.getWidth();
    auto const height = bounds.getHeight();
    
    auto const clip = g.getClipBounds();
    auto const horizontalRange = clip.getHorizontalRange().expanded(1);
    
    g.setColour(juce::Colours::black);
    auto const& results = mAccessor.getValue<AttrType::results>();
    if(results.empty() && width > 0)
    {
        return;
    }
    
    auto const timeRange = mZoomAccessor.getValue<Zoom::AttrType::visibleRange>();
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
        for(size_t i = 0; i < results.size(); i += resultIncrement)
        {
            auto const x = timeToPixel(results[i].timestamp);
            if(horizontalRange.contains(x))
            {
                g.drawVerticalLine(x, 0.0f, static_cast<float>(height));
            }
        }
    }
    else if(results.front().values.size() == 1)
    {
        auto const valueRange = mAccessor.getAccessors<AttrType::zoom>()[0].get().getValue<Zoom::AttrType::visibleRange>();
        auto valueToPixel = [&](float const value)
        {
            auto const ratio = 1.0f - (value - valueRange.getStart()) / valueRange.getLength();
            return static_cast<int>(ratio * height);
        };
        juce::Point<float> pt;
        
        for(size_t i = 0; i < results.size(); i += resultIncrement)
        {
            auto const x = timeToPixel(results[i].timestamp);
            if(horizontalRange.contains(x))
            {
                juce::Point<float> const npt{static_cast<float>(x), static_cast<float>(valueToPixel(results[i].values[0]))};
                g.drawLine({pt, npt});
                pt = npt;
            }
        }
    }
    else
    {
        
        auto const valueRange = mAccessor.getAccessors<AttrType::zoom>()[0].get().getValue<Zoom::AttrType::visibleRange>();
        
        auto valueToColour = [&](float const value)
        {
            auto const color = tinycolormap::GetColor(static_cast<double>(value) / valueRange.getEnd(), tinycolormap::ColormapType::Hot);
            return juce::Colour::fromFloatRGBA(static_cast<float>(color.r()), static_cast<float>(color.g()), static_cast<float>(color.b()), 1.0f);
        };
        
        
        auto const start = static_cast<size_t>(std::floor(valueRange.getStart()));
        auto const end = static_cast<size_t>(std::floor(valueRange.getEnd()));
        
        auto const valueRatio = valueRange.getLength() / static_cast<double>(height);
        auto const valuetIncrement = static_cast<size_t>(std::floor(std::max(valueRatio, 1.0)));
        auto const cellHeight = static_cast<int>(std::ceil(std::max(1.0 / valueRatio, 1.0)));
        auto const cellWidth = static_cast<int>(std::ceil(std::max(width / timeLength, 1.0)));
        
        for(size_t i = 0; i < results.size(); i += resultIncrement)
        {
            auto const x = timeToPixel(results[i].timestamp);
            if(horizontalRange.contains(x))
            {
                auto y = height - cellHeight;
                for(auto index = start; index < end; index += valuetIncrement)
                {
                    g.setColour(valueToColour(results[i].values[index]));
                    g.fillRect(x, y, cellWidth, cellHeight);
                    y -= cellHeight;
                }
            }
        }
        
    }
}

ANALYSE_FILE_END
