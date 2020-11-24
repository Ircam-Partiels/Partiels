#include "AnlAnalyzerResultRenderer.h"
#include "AnlAnalyzerProcessor.h"
#include "../Tools/AnlMisc.h"
#include "../../tinycolormap/include/tinycolormap.hpp"

ANALYSE_FILE_BEGIN

Analyzer::ResultRenderer::ResultRenderer(Accessor& accessor, Zoom::Accessor& zoomAccessor)
: mAccessor(accessor)
, mZoomAccessor(zoomAccessor)
{
    mInformation.setEditable(false);
    mInformation.setInterceptsMouseClicks(false, false);
    addChildComponent(mInformation);
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        if(attribute == AttrType::colour)
        {
            repaint();
        }
        else if(attribute == AttrType::results || attribute == AttrType::colourMap)
        {
            auto const& results = acsr.getAttr<AttrType::results>();
            if(results.empty())
            {
                repaint();
                return;
            }
            
            if(results.front().values.size() > 1)
            {
                auto const witdh = static_cast<int>(results.size());
                auto const height = static_cast<int>(results.front().values.size());
                mImage = juce::Image(juce::Image::PixelFormat::ARGB, witdh, height, false);
                auto image = mImage;
                juce::Image::BitmapData const data(image, juce::Image::BitmapData::writeOnly);
                
                float maxValue = 0.0;
                auto valueToColour = [&](float const value)
                {
                    maxValue = std::max(maxValue, value);
                    auto const color = tinycolormap::GetColor(static_cast<double>(value) / (height * 0.25), acsr.getAttr<AttrType::colourMap>());
                    return juce::Colour::fromFloatRGBA(static_cast<float>(color.r()), static_cast<float>(color.g()), static_cast<float>(color.b()), 1.0f);
                };
                
                for(int i = 0; i < witdh; ++i)
                {
                    for(int j = 0; j < height; ++j)
                    {
                        auto const colour = valueToColour(results[static_cast<size_t>(i)].values[static_cast<size_t>(j)]);
                        data.setPixelColour(i, height - 1 - j, colour);
                    }
                }
            }
            
            repaint();
        }
    };
    
    mZoomListener.onChanged = [&](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        repaint();
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAccessor<AttrType::zoom>(0).addListener(mZoomListener, NotificationType::synchronous);
    mZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
}

Analyzer::ResultRenderer::~ResultRenderer()
{
    mZoomAccessor.removeListener(mZoomListener);
    mAccessor.getAccessor<AttrType::zoom>(0).removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Analyzer::ResultRenderer::resized()
{
    mInformation.setBounds(getLocalBounds().removeFromRight(200).removeFromTop(80));
}

void Analyzer::ResultRenderer::paint(juce::Graphics& g)
{
    auto const bounds = getLocalBounds();
    auto const width = bounds.getWidth();
    auto const height = bounds.getHeight();
    
    auto const& results = mAccessor.getAttr<AttrType::results>();
    if(results.empty() && width > 0)
    {
        return;
    }
    
    auto const timeRange = mZoomAccessor.getAttr<Zoom::AttrType::visibleRange>();
    
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
        g.setColour(mAccessor.getAttr<AttrType::colour>());
        for(size_t i = 0; i < results.size(); i += resultIncrement)
        {
            if(realTimeRange.contains(results[i].timestamp))
            {
                auto const x = timeToPixel(results[i].timestamp);
                g.drawVerticalLine(x, 0.0f, static_cast<float>(height));
            }
            else if(results[i].timestamp >= realTimeRange.getEnd())
            {
                break;
            }
        }
    }
    else if(results.front().values.size() == 1)
    {
        g.setColour(mAccessor.getAttr<AttrType::colour>());
        auto const valueRange = mAccessor.getAccessor<AttrType::zoom>(0).getAttr<Zoom::AttrType::visibleRange>();
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
            if(isVisible)
            {
                auto const x = timeToPixel(results[i].timestamp);
                juce::Point<float> const npt{static_cast<float>(x), static_cast<float>(valueToPixel(results[i].values[0]))};
                g.drawLine({pt, npt});
                pt = npt;
            }
            else if(results[i].timestamp >= realTimeRange.getEnd())
            {
                break;
            }
        }
    }
    else
    {
        auto image = mImage;
        
        auto const globalValueRange = mAccessor.getAccessor<AttrType::zoom>(0).getAttr<Zoom::AttrType::globalRange>();
        auto const vRange = mAccessor.getAccessor<AttrType::zoom>(0).getAttr<Zoom::AttrType::visibleRange>();
        auto const globalTimeRange = mZoomAccessor.getAttr<Zoom::AttrType::globalRange>();
        
        auto const valueRange = juce::Range<double>(globalValueRange.getEnd() - vRange.getEnd(), globalValueRange.getEnd() - vRange.getStart());
        
        auto const deltaX = static_cast<float>(timeRange.getStart() / globalTimeRange.getLength() * static_cast<double>(image.getWidth()));
        auto const deltaY = static_cast<float>(valueRange.getStart() / globalValueRange.getLength() * static_cast<double>(image.getHeight()));
        
        auto const scaleX = static_cast<float>(globalTimeRange.getLength() / timeRange.getLength() * static_cast<double>(width) / static_cast<double>(image.getWidth()));
        auto const scaleY = static_cast<float>(globalValueRange.getLength() / valueRange.getLength() * static_cast<double>(height) / static_cast<double>(image.getHeight()));
        
        auto const transform = juce::AffineTransform::translation(-deltaX, -deltaY).scaled(scaleX, scaleY);
        g.drawImageTransformed(image, transform);
    }
}

void Analyzer::ResultRenderer::mouseMove(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    repaint();
    auto const& results = mAccessor.getAttr<AttrType::results>();
    if(results.empty() && getWidth() > 0)
    {
        return;
    }
    juce::String text;
    
    auto const timeRange = mZoomAccessor.getAttr<Zoom::AttrType::visibleRange>();
    auto const time = static_cast<double>(event.x) / static_cast<double>(getWidth()) * timeRange.getLength() + timeRange.getStart();
    auto const rtr = Vamp::RealTime::fromSeconds(time);
    text += Tools::secondsToString(time) + "\n";
    if(results.front().values.empty())
    {
        
    }
    else if(results.front().values.size() == 1)
    {
        for(size_t i = 0; i < results.size(); i++)
        {
            if(results[i].timestamp >= rtr)
            {
                text += juce::String(results[i].values[0]) + results[i].label;
                break;
            }
        }
    }
    else
    {
        auto const valueRange = mAccessor.getAccessor<AttrType::zoom>(0).getAttr<Zoom::AttrType::visibleRange>();
        for(size_t i = 0; i < results.size(); i++)
        {
            if(results[i].timestamp >= rtr)
            {
                auto const index = std::max(std::min((1.0 - static_cast<double>(event.y) / static_cast<double>(getHeight())), 1.0), 0.0) * valueRange.getLength() + valueRange.getStart();
                text += juce::String(static_cast<long>(index)) + "\n";
                text += juce::String(results[i].values[static_cast<size_t>(index)] / valueRange.getLength()) + results[i].label;
                break;
            }
        }
    }
    mInformation.setText(text, juce::NotificationType::dontSendNotification);
}

void Analyzer::ResultRenderer::mouseEnter(juce::MouseEvent const& event)
{
    mInformation.setVisible(true);
    mouseMove(event);
}

void Analyzer::ResultRenderer::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    mInformation.setVisible(false);
}

ANALYSE_FILE_END
