#include "AnlAnalyzerTimeRenderer.h"

ANALYSE_FILE_BEGIN

Analyzer::TimeRenderer::TimeRenderer(Accessor& accessor, Zoom::Accessor& zoomAccessor)
: mAccessor(accessor)
, mZoomAccessor(zoomAccessor)
{
    mInformation.setEditable(false);
    mInformation.setInterceptsMouseClicks(false, false);
    addChildComponent(mInformation);
    
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
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
                break;
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

Analyzer::TimeRenderer::~TimeRenderer()
{
    mZoomAccessor.removeListener(mZoomListener);
    mAccessor.getAccessor<AcsrType::binZoom>(0).removeListener(mZoomListener);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
    mAccessor.removeReceiver(mReceiver);
}

void Analyzer::TimeRenderer::resized()
{
    mInformation.setBounds(getLocalBounds().removeFromRight(200).removeFromTop(80));
}

void Analyzer::TimeRenderer::paint(juce::Graphics& g)
{
    auto const bounds = getLocalBounds();
    auto const width = bounds.getWidth();
    auto const height = bounds.getHeight();
    
    auto const& results = mAccessor.getAttr<AttrType::results>();
    if(results.empty() && width > 0)
    {
        return;
    }
    
    auto& zoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
    auto const globalValueRange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
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
        auto const clip = g.getClipBounds();
        g.setColour(mAccessor.getAttr<AttrType::colour>());
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
        auto image = mAccessor.getImage();
        if(image == nullptr || image->isNull())
        {
            return;
        }
        
        auto const vRange = mAccessor.getAccessor<AcsrType::binZoom>(0).getAttr<Zoom::AttrType::visibleRange>();
        auto const globalTimeRange = mZoomAccessor.getAttr<Zoom::AttrType::globalRange>();
        
        auto const valueRange = juce::Range<double>(globalValueRange.getEnd() - vRange.getEnd(), globalValueRange.getEnd() - vRange.getStart());
        
        auto const deltaX = static_cast<float>(timeRange.getStart() / globalTimeRange.getLength() * static_cast<double>(image->getWidth()));
        auto const deltaY = static_cast<float>(valueRange.getStart() / globalValueRange.getLength() * static_cast<double>(image->getHeight()));
        
        auto const scaleX = static_cast<float>(globalTimeRange.getLength() / timeRange.getLength() * static_cast<double>(width) / static_cast<double>(image->getWidth()));
        auto const scaleY = static_cast<float>(globalValueRange.getLength() / valueRange.getLength() * static_cast<double>(height) / static_cast<double>(image->getHeight()));
        
        auto const transform = juce::AffineTransform::translation(-deltaX, -deltaY).scaled(scaleX, scaleY);
        
        g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
        g.drawImageTransformed(*image.get(), transform);
    }
}

void Analyzer::TimeRenderer::mouseMove(juce::MouseEvent const& event)
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
        auto const valueRange = mAccessor.getAccessor<AcsrType::binZoom>(0).getAttr<Zoom::AttrType::visibleRange>();
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

void Analyzer::TimeRenderer::mouseEnter(juce::MouseEvent const& event)
{
    mInformation.setVisible(true);
    mouseMove(event);
}

void Analyzer::TimeRenderer::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    mInformation.setVisible(false);
}

ANALYSE_FILE_END
