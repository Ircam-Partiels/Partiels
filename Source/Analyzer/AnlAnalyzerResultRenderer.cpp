#include "AnlAnalyzerResultRenderer.h"
#include "AnlAnalyzerProcessor.h"

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
    mZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
}

Analyzer::ResultRenderer::~ResultRenderer()
{
    mZoomAccessor.removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Analyzer::ResultRenderer::paint(juce::Graphics& g)
{
    auto const width = getWidth();
    auto const height = getHeight();
    auto const range = mZoomAccessor.getValue<Zoom::AttrType::visibleRange>();
    
    auto timeToPixel = [&](Vamp::RealTime const&  timestamp)
    {
        auto const time = (((timestamp.sec * 1000.0 + timestamp.msec()) / 1000.0) - range.getStart()) / range.getLength();
        return static_cast<int>(time * width);
    };
    
    auto valueToPixel = [&](float const value)
    {
        return static_cast<int>((1.0f - value / 22050.0f) * height);
    };
    
    g.setColour(juce::Colours::black);
    auto const& results = mAccessor.getValue<AttrType::results>();
    juce::Point<float> pt;
    for(auto const& result : results)
    {
        if(result.values.empty())
        {
            g.drawVerticalLine(timeToPixel(result.timestamp), 0.0f, static_cast<float>(height));
        }
        else if(result.values.size() == 1)
        {
            juce::Point<float> const npt{static_cast<float>(timeToPixel(result.timestamp)), static_cast<float>(valueToPixel(result.values[0]))};
            g.drawLine({pt, npt});
            pt = npt;
        }
        else
        {
            
        }
    }
}

ANALYSE_FILE_END
