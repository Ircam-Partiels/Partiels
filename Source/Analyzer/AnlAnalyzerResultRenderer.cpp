#include "AnlAnalyzerResultRenderer.h"

ANALYSE_FILE_BEGIN

Analyzer::ResultRenderer::ResultRenderer(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        if(attribute == AttrType::results)
        {
            repaint();
        }
    };
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Analyzer::ResultRenderer::~ResultRenderer()
{
    mAccessor.removeListener(mListener);
}

void Analyzer::ResultRenderer::paint(juce::Graphics& g)
{
    auto const width = getWidth();
    auto const height = getHeight();
    auto timeToPixel = [&](Vamp::RealTime const&  timestamp)
    {
        auto const time = (timestamp.sec * 1000.0 + timestamp.msec()) / 60000.0;
        return static_cast<int>(time * width);
    };
    g.setColour(juce::Colours::black);
    auto const& results = mAccessor.getValue<AttrType::results>();
    for(auto const& result : results)
    {
        if(result.values.empty())
        {
            g.drawVerticalLine(timeToPixel(result.timestamp), 0.0f, static_cast<float>(height));
        }
        else if(result.values.size() == 1)
        {
            
        }
        else
        {
            
        }
    }
}

ANALYSE_FILE_END
