#include "AnlAnalyzerInstantRenderer.h"
#include "../Plot/AnlPlotRenderer.h"

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
        if(attribute == AttrType::colour || attribute == AttrType::results || attribute == AttrType::colourMap)
        {
            repaint();
        }
    };
    
    mReceiver.onSignal = [&](Accessor const& acsr, SignalType signal, juce::var value)
    {
        juce::ignoreUnused(acsr, value);
        switch(signal)
        {
            case SignalType::analyse:
            case SignalType::time:
            case SignalType::image:
            {
                repaint();
            }
                break;
        }
    };
    
    mZoomListener.onAttrChanged = [&](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };
    
    mAccessor.addReceiver(mReceiver);
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::binZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
    mZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
}

Analyzer::InstantRenderer::~InstantRenderer()
{
    mZoomAccessor.removeListener(mZoomListener);
    mAccessor.getAccessor<AcsrType::binZoom>(0).removeListener(mZoomListener);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
    mAccessor.removeReceiver(mReceiver);
}

void Analyzer::InstantRenderer::resized()
{
    mInformation.setBounds(getLocalBounds().removeFromRight(200).removeFromTop(80));
}

void Analyzer::InstantRenderer::paint(juce::Graphics& g)
{
    auto& zoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
    auto const& visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const& colour = mAccessor.getAttr<AttrType::colour>();
    auto const& description = mAccessor.getAttr<AttrType::description>();
    auto const& results = mAccessor.getAttr<AttrType::results>();
    auto const time = mAccessor.getTime();
    Plot::Renderer::paint(g, getLocalBounds(), colour, description.output, results, visibleRange, time);
    
    if(!results.empty() && results.cbegin()->values.size() > 1)
    {
        auto image = mAccessor.getImage();
        if(image == nullptr || !image->isValid())
        {
            return;
        }
        
        auto const width = getWidth();
        auto const height = getHeight();
        
        auto const globalValueRange = mAccessor.getAccessor<AcsrType::binZoom>(0).getAttr<Zoom::AttrType::globalRange>();
        auto const globalTimeRange = mZoomAccessor.getAttr<Zoom::AttrType::globalRange>();
        
        auto const valueRange = juce::Range<double>(globalValueRange.getEnd() - visibleRange.getEnd(), globalValueRange.getEnd() - visibleRange.getStart());
        
        auto const deltaX = static_cast<float>(time / globalTimeRange.getLength() * static_cast<double>(image->getWidth()));
        auto const deltaY = static_cast<float>(valueRange.getStart() / globalValueRange.getLength() * static_cast<double>(image->getHeight()));
        
        auto const scaleX = static_cast<float>(static_cast<double>(width) * static_cast<double>(image->getWidth()));
        auto const scaleY = static_cast<float>(globalValueRange.getLength() / valueRange.getLength() * static_cast<double>(height) / static_cast<double>(image->getHeight()));
        
        auto const transform = juce::AffineTransform::translation(-deltaX, -deltaY).scaled(scaleX, scaleY);
        
        g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
        g.drawImageTransformed(*image.get(), transform);
    }
}

ANALYSE_FILE_END
