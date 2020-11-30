#include "AnlAnalyzerInstantRenderer.h"
#include "AnlAnalyzerProcessor.h"
#include "../Tools/AnlMisc.h"
#include "../../tinycolormap/include/tinycolormap.hpp"

ANALYSE_FILE_BEGIN

Analyzer::InstantRenderer::InstantRenderer(Accessor& accessor)
: mAccessor(accessor)
{
    mInformation.setEditable(false);
    mInformation.setInterceptsMouseClicks(false, false);
    addChildComponent(mInformation);
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        if(attribute == AttrType::colour || attribute == AttrType::results || attribute == AttrType::colourMap)
        {
            repaint();
        }
    };
    
    mZoomListener.onChanged = [&](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAccessor<AttrType::zoom>(0).addListener(mZoomListener, NotificationType::synchronous);
}

Analyzer::InstantRenderer::~InstantRenderer()
{
    mAccessor.getAccessor<AttrType::zoom>(0).removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Analyzer::InstantRenderer::resized()
{
    mInformation.setBounds(getLocalBounds().removeFromRight(200).removeFromTop(80));
}

void Analyzer::InstantRenderer::paint(juce::Graphics& g)
{
    auto& zoomAcsr = mAccessor.getAccessor<AttrType::zoom>(0);
    auto const visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    if(visibleRange.isEmpty())
    {
        return;
    }
    
    auto const& results = mAccessor.getAttr<AttrType::results>();
    if(results.empty())
    {
        return;
    }
    
    auto const realTime = Vamp::RealTime::fromSeconds(mTime);
    auto it = std::lower_bound(results.cbegin(), results.cend(), realTime, [](auto const& result, auto const& time)
    {
        return result.timestamp < time;
    });
    if(it == results.cend())
    {
        return;
    }
    
    if(it->values.size() == 1)
    {
        auto const value = it->values[0];
//        auto const prevTime = static_cast<double>(it->timestamp.sec) + static_cast<double>(it->timestamp.msec()) / 1000.0;
//        ++it;
//        auto const nextValue = it != results.cend() ? it->values[0] : prevValue;
//        auto const nextTime = it != results.cend() ? static_cast<double>(it->timestamp.sec) + static_cast<double>(it->timestamp.msec()) / 1000.0 : prevTime;
//
//
        
        auto const bounds = getLocalBounds();
        auto const position = (1.0 - (value - visibleRange.getStart()) / visibleRange.getLength()) * static_cast<double>(bounds.getHeight());
        auto const area = bounds.withTop(static_cast<int>(position)).toFloat();
        
        auto const colour = mAccessor.getAttr<AttrType::colour>();
        g.setColour(colour.withAlpha(0.2f));
        g.fillRect(area);
        
        g.setColour(colour);
        g.fillRect(area.withHeight(1.0f));
    }
    else if(it->values.size() > 1)
    {
        
    }
}

void Analyzer::InstantRenderer::setTime(double time)
{
    mTime = time;
    repaint();
}

ANALYSE_FILE_END
