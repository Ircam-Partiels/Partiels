#include "AnlAnalyzerRendererFrame.h"

ANALYSE_FILE_BEGIN

Analyzer::Renderer::Frame::Frame(Accessor& accessor, Zoom::Accessor& zoomAccessor)
: mAccessor(accessor)
, mZoomAccessor(zoomAccessor)
{
    mInformation.setEditable(false);
    mInformation.setInterceptsMouseClicks(false, false);
    addChildComponent(mInformation);
    
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::key:
            case AttrType::name:
            case AttrType::feature:
            case AttrType::parameters:
            case AttrType::zoomMode:
            case AttrType::colourMap:
                break;
            case AttrType::colour:
            case AttrType::results:
            {
                repaint();
            }
                break;
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
    
    mAccessor.addReceiver(mReceiver);
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
}

Analyzer::Renderer::Frame::~Frame()
{
    for(auto& zoomAcsr : mZoomAccessors)
    {
        zoomAcsr.get().removeListener(mZoomListener);
    }
    mZoomAccessor.removeListener(mZoomListener);
    mAccessor.getAccessor<AcsrType::zoom>(0).removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
    mAccessor.removeReceiver(mReceiver);
}

void Analyzer::Renderer::Frame::resized()
{
    mInformation.setBounds(getLocalBounds().removeFromRight(200).removeFromTop(80));
}

void Analyzer::Renderer::Frame::paint(juce::Graphics& g)
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
    
    auto const realTime = Vamp::RealTime::fromSeconds(0.0);
    
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
        auto image = mAccessor.getImage();
        if(image == nullptr && image->isValid())
        {
            return;
        }
        
        auto const width = getWidth();
        auto const height = getHeight();
        
        auto const globalValueRange = mAccessor.getAccessor<AcsrType::zoom>(0).getAttr<Zoom::AttrType::globalRange>();
        auto const globalTimeRange = mZoomAccessor.getAttr<Zoom::AttrType::globalRange>();
        
        auto const valueRange = juce::Range<double>(globalValueRange.getEnd() - visibleRange.getEnd(), globalValueRange.getEnd() - visibleRange.getStart());
        
        auto const deltaX = static_cast<float>(0.0 / globalTimeRange.getLength() * static_cast<double>(image->getWidth()));
        auto const deltaY = static_cast<float>(valueRange.getStart() / globalValueRange.getLength() * static_cast<double>(image->getHeight()));
        
        auto const scaleX = static_cast<float>(static_cast<double>(width) * static_cast<double>(image->getWidth()));
        auto const scaleY = static_cast<float>(globalValueRange.getLength() / valueRange.getLength() * static_cast<double>(height) / static_cast<double>(image->getHeight()));
        
        auto const transform = juce::AffineTransform::translation(-deltaX, -deltaY).scaled(scaleX, scaleY);
        
        g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
        g.drawImageTransformed(*image.get(), transform);
    }
}

ANALYSE_FILE_END
