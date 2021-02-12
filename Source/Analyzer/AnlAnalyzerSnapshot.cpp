#include "AnlAnalyzerSnapshot.h"
#include "AnlAnalyzerRenderer.h"

ANALYSE_FILE_BEGIN

Analyzer::Snapshot::Snapshot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
{
    mInformation.setEditable(false);
    mInformation.setInterceptsMouseClicks(false, false);
    addChildComponent(mInformation);
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::key:
            case AttrType::description:
            case AttrType::state:
            case AttrType::height:
                break;
            case AttrType::colours:
            {
                repaint();
            }
                break;
            case AttrType::propertyState:
                break;
            case AttrType::results:
            case AttrType::time:
            {
                repaint();
            }
                break;
                
            case AttrType::warnings:
            case AttrType::processing:
                break;
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
    
    
    mAccessor.addReceiver(mReceiver);
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::binZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
    mTimeZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
}

Analyzer::Snapshot::~Snapshot()
{
    mTimeZoomAccessor.removeListener(mZoomListener);
    mAccessor.getAccessor<AcsrType::binZoom>(0).removeListener(mZoomListener);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
    mAccessor.removeReceiver(mReceiver);
}

void Analyzer::Snapshot::resized()
{
    mInformation.setBounds(getLocalBounds().removeFromRight(200).removeFromTop(80));
}

void Analyzer::Snapshot::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
    auto const bounds = getLocalBounds().reduced(2);
    juce::Path path;
    path.addRoundedRectangle(bounds.expanded(1), 4.0f);
    g.setColour(findColour(ColourIds::borderColourId));
    g.strokePath(path, juce::PathStrokeType(1.0f));
    path.clear();
    path.addRoundedRectangle(bounds, 4.0f);
    g.reduceClipRegion(path);
    
    auto const& valueZoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
    auto const& visibleValueRange = valueZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const& colours = mAccessor.getAttr<AttrType::colours>();
    auto const& description = mAccessor.getAttr<AttrType::description>();
    auto const& results = mAccessor.getAttr<AttrType::results>();
    auto const time = mAccessor.getAttr<AttrType::time>();
    Renderer::paint(g, bounds, colours.line, description.output, results, visibleValueRange, time);
    
    if(!results.empty() && results.cbegin()->values.size() > 1)
    {
        auto image = mAccessor.getImage();
        if(image == nullptr || !image->isValid())
        {
            return;
        }
        
        auto const width = getWidth();
        auto const height = getHeight();
        
        auto const& binZoomAcsr = mAccessor.getAccessor<AcsrType::binZoom>(0);
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
