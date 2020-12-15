#include "AnlAnalyzerInstantRenderer.h"
#include "AnlAnalyzerProcessor.h"
#include "../Tools/AnlMisc.h"
#include "../../tinycolormap/include/tinycolormap.hpp"

ANALYSE_FILE_BEGIN

Analyzer::InstantRenderer::InstantRenderer(Accessor& accessor, Zoom::Accessor& zoomAccessor)
: mAccessor(accessor)
, mZoomAccessor(zoomAccessor)
{
    mInformation.setEditable(false);
    mInformation.setInterceptsMouseClicks(false, false);
    addChildComponent(mInformation);
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
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
                
                auto valueToColour = [&](float const value)
                {
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
    
    mListener.onAccessorInserted = [&](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::zoom:
            {
                auto& zoomAcsr = mAccessor.getAccessor<AcsrType::zoom>(index);
                zoomAcsr.addListener(mZoomListener, NotificationType::synchronous);
                mZoomAccessors.insert(mZoomAccessors.begin() + static_cast<long>(index), zoomAcsr);
            }
                break;
        }
    };
    
    mListener.onAccessorErased = [&](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::zoom:
            {
                mZoomAccessors[index].get().removeListener(mZoomListener);
                mZoomAccessors.erase(mZoomAccessors.begin() + static_cast<long>(index));
            }
                break;
        }
    };
    
    mZoomListener.onAttrChanged = [&](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
}

Analyzer::InstantRenderer::~InstantRenderer()
{
    for(auto& zoomAcsr : mZoomAccessors)
    {
        zoomAcsr.get().removeListener(mZoomListener);
    }
    mZoomAccessor.removeListener(mZoomListener);
    mAccessor.getAccessor<AcsrType::zoom>(0).removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Analyzer::InstantRenderer::resized()
{
    mInformation.setBounds(getLocalBounds().removeFromRight(200).removeFromTop(80));
}

void Analyzer::InstantRenderer::paint(juce::Graphics& g)
{
    auto& zoomAcsr = mAccessor.getAccessor<AcsrType::zoom>(0);
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
    
    if(results.cbegin()->values.size() == 1)
    {
        auto it = std::lower_bound(results.cbegin(), results.cend(), realTime, [](auto const& result, auto const& time)
        {
            return result.timestamp < time;
        });
        if(it == results.cend())
        {
            return;
        }
        
        auto const value = (it->values[0]  - visibleRange.getStart()) / visibleRange.getLength();
        auto const bounds = getLocalBounds();
        auto const position = static_cast<int>((1.0 - value) * static_cast<double>(bounds.getHeight()));
        auto const area = bounds.withTop(position).toFloat();
        
        auto const colour = mAccessor.getAttr<AttrType::colour>();
        g.setColour(colour.withAlpha(0.2f));
        g.fillRect(area);
        
        g.setColour(colour);
        g.fillRect(area.withHeight(1.0f));
    }
    else if(results.cbegin()->values.size() > 1)
    {
        auto image = mImage;
        
        auto const width = getWidth();
        auto const height = getHeight();
        
        auto const globalValueRange = mAccessor.getAccessor<AcsrType::zoom>(0).getAttr<Zoom::AttrType::globalRange>();
        auto const globalTimeRange = mZoomAccessor.getAttr<Zoom::AttrType::globalRange>();
        
        auto const valueRange = juce::Range<double>(globalValueRange.getEnd() - visibleRange.getEnd(), globalValueRange.getEnd() - visibleRange.getStart());
        
        auto const deltaX = static_cast<float>(mTime / globalTimeRange.getLength() * static_cast<double>(image.getWidth()));
        auto const deltaY = static_cast<float>(valueRange.getStart() / globalValueRange.getLength() * static_cast<double>(image.getHeight()));
        
        auto const scaleX = static_cast<float>(static_cast<double>(width) * static_cast<double>(image.getWidth()));
        auto const scaleY = static_cast<float>(globalValueRange.getLength() / valueRange.getLength() * static_cast<double>(height) / static_cast<double>(image.getHeight()));
        
        auto const transform = juce::AffineTransform::translation(-deltaX, -deltaY).scaled(scaleX, scaleY);
        
        g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
        g.drawImageTransformed(image, transform);
    }
}

void Analyzer::InstantRenderer::setTime(double time)
{
    mTime = time;
    repaint();
}

ANALYSE_FILE_END
