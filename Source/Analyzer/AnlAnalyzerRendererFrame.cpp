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
        juce::ignoreUnused(acsr, type, index);
        switch(type)
        {
            case AcsrType::valueZoom:
            {
                auto& zoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(index);
                zoomAcsr.addListener(mZoomListener, NotificationType::synchronous);
                mZoomAccessors.insert(mZoomAccessors.begin() + static_cast<long>(0), zoomAcsr);
            }
                break;
            case AcsrType::binZoom:
            {
                auto& zoomAcsr = mAccessor.getAccessor<AcsrType::binZoom>(index);
                zoomAcsr.addListener(mZoomListener, NotificationType::synchronous);
                mZoomAccessors.insert(mZoomAccessors.begin() + static_cast<long>(1), zoomAcsr);
            }
                break;
        }
    };
    
    mListener.onAccessorErased = [&](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr, index);
        switch(type)
        {
            case AcsrType::valueZoom:
            {
                mZoomAccessors[0].get().removeListener(mZoomListener);
                mZoomAccessors.erase(mZoomAccessors.begin());
            }
                break;
            case AcsrType::binZoom:
            {
                mZoomAccessors[1].get().removeListener(mZoomListener);
                mZoomAccessors.erase(mZoomAccessors.begin()+1);
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
    mAccessor.removeListener(mListener);
    mAccessor.removeReceiver(mReceiver);
}

void Analyzer::Renderer::Frame::resized()
{
    mInformation.setBounds(getLocalBounds().removeFromRight(200).removeFromTop(80));
}

void Analyzer::Renderer::Frame::paint(juce::Graphics& g)
{
    
}

ANALYSE_FILE_END
