#include "AnlAnalyzerInstantRenderer.h"
#include "../Plot/AnlPlotRenderer.h"

ANALYSE_FILE_BEGIN

Analyzer::InstantRenderer::InstantRenderer(Accessor& accessor, Zoom::Accessor& timeZoomAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
{
    mInformation.setEditable(false);
    mInformation.setInterceptsMouseClicks(false, false);
    addChildComponent(mInformation);
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        if(attribute == AttrType::results || attribute == AttrType::time)
        {
            repaint();
        }
    };
    
    mReceiver.onSignal = [&](Accessor const& acsr, SignalType signal, juce::var value)
    {
        juce::ignoreUnused(acsr, value);
        switch(signal)
        {
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
    
    mPlotListener.onAttrChanged = [&](Plot::Accessor const& acsr, Plot::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        if(attribute == Plot::AttrType::colourPlain || attribute == Plot::AttrType::colourMap)
        {
            repaint();
        }
    };
    
    mAccessor.addReceiver(mReceiver);
    mAccessor.addListener(mListener, NotificationType::synchronous);
    auto& plotAcsr = mAccessor.getAccessor<AcsrType::plot>(0);
    plotAcsr.addListener(mPlotListener, NotificationType::synchronous);
    plotAcsr.getAccessor<Plot::AcsrType::valueZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
    plotAcsr.getAccessor<Plot::AcsrType::binZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
    mTimeZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
}

Analyzer::InstantRenderer::~InstantRenderer()
{
    mTimeZoomAccessor.removeListener(mZoomListener);
    auto& plotAcsr = mAccessor.getAccessor<AcsrType::plot>(0);
    plotAcsr.getAccessor<Plot::AcsrType::binZoom>(0).removeListener(mZoomListener);
    plotAcsr.getAccessor<Plot::AcsrType::valueZoom>(0).removeListener(mZoomListener);
    plotAcsr.removeListener(mPlotListener);
    mAccessor.removeListener(mListener);
    mAccessor.removeReceiver(mReceiver);
}

void Analyzer::InstantRenderer::resized()
{
    mInformation.setBounds(getLocalBounds().removeFromRight(200).removeFromTop(80));
}

void Analyzer::InstantRenderer::paint(juce::Graphics& g)
{
    auto const& plotAscr = mAccessor.getAccessor<AcsrType::plot>(0);
    auto const& valueZoomAcsr = plotAscr.getAccessor<Plot::AcsrType::valueZoom>(0);
    auto const& visibleValueRange = valueZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const& colour = plotAscr.getAttr<Plot::AttrType::colourPlain>();
    auto const& description = mAccessor.getAttr<AttrType::description>();
    auto const& results = mAccessor.getAttr<AttrType::results>();
    auto const time = mAccessor.getAttr<AttrType::time>();
    Plot::Renderer::paint(g, getLocalBounds(), colour, description.output, results, visibleValueRange, time);
    
    if(!results.empty() && results.cbegin()->values.size() > 1)
    {
        auto image = mAccessor.getImage();
        if(image == nullptr || !image->isValid())
        {
            return;
        }
        
        auto const width = getWidth();
        auto const height = getHeight();
        
        auto const& binZoomAcsr = plotAscr.getAccessor<Plot::AcsrType::binZoom>(0);
        auto const binVisibleRange = binZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
        auto const binGlobalRange = binZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
        
        auto const globalTimeRange = mTimeZoomAccessor.getAttr<Zoom::AttrType::globalRange>();
        
        auto const valueRange = juce::Range<double>(binGlobalRange.getEnd() - binVisibleRange.getEnd(), binGlobalRange.getEnd() - binVisibleRange.getStart());
        
        auto const deltaX = static_cast<float>(time / globalTimeRange.getLength() * static_cast<double>(image->getWidth()));
        auto const deltaY = static_cast<float>(valueRange.getStart() / binGlobalRange.getLength() * static_cast<double>(image->getHeight()));
        
        auto const scaleX = static_cast<float>(static_cast<double>(width) * static_cast<double>(image->getWidth()));
        auto const scaleY = static_cast<float>(binGlobalRange.getLength() / valueRange.getLength() * static_cast<double>(height) / static_cast<double>(image->getHeight()));
        
        auto const transform = juce::AffineTransform::translation(-deltaX, -deltaY).scaled(scaleX, scaleY);
        
        g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
        g.drawImageTransformed(*image.get(), transform);
    }
}

ANALYSE_FILE_END
