#include "AnlTrackSnapshot.h"
#include "AnlTrackResults.h"

ANALYSE_FILE_BEGIN

Track::Snapshot::Snapshot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
{
    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
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
            case AttrType::propertyState:
            case AttrType::warnings:
                break;
            case AttrType::results:
            case AttrType::graphics:
            case AttrType::processing:
            case AttrType::colours:
            case AttrType::time:
            {
                repaint();
            }
                break;
        }
    };
    
    mValueZoomListener.onAttrChanged = [=, this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };
    
    mBinZoomListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };
    
    mTimeZoomListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };
    

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).addListener(mValueZoomListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::binZoom>(0).addListener(mBinZoomListener, NotificationType::synchronous);
    mTimeZoomAccessor.addListener(mTimeZoomListener, NotificationType::synchronous);
}

Track::Snapshot::~Snapshot()
{
    mTimeZoomAccessor.removeListener(mTimeZoomListener);
    mAccessor.getAccessor<AcsrType::binZoom>(0).removeListener(mBinZoomListener);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).removeListener(mValueZoomListener);
    mAccessor.removeListener(mListener);
}

void Track::Snapshot::paint(juce::Graphics& g)
{
    auto const bounds = getLocalBounds();
    auto const& valueZoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
    auto const& visibleValueRange = valueZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const& colours = mAccessor.getAttr<AttrType::colours>();
    auto const& output = mAccessor.getAttr<AttrType::description>().output;
    auto const time = mAccessor.getAttr<AttrType::time>();
    auto resultPtr = mAccessor.getAttr<AttrType::results>();
    
    if(resultPtr == nullptr || resultPtr->empty() || bounds.isEmpty() || visibleValueRange.isEmpty())
    {
        return;
    }
    
    auto const& results = *resultPtr;
    
    enum class DisplayMode
    {
          unsupported
        , surface
        , bar
        , segment
        , matrix
    };
    
    auto const numDimensions = output.hasFixedBinCount ? output.binCount : results.front().values.size();
    auto getDisplayMode = [&]()
    {
        switch(numDimensions)
        {
            case 0:
                return output.hasDuration ? DisplayMode::surface : DisplayMode::unsupported;
            case 1:
                return output.hasDuration ? DisplayMode::bar : DisplayMode::segment;
            default:
                return DisplayMode::matrix;
        }
    };
    
    auto getScaledValue = [&](float value)
    {
        anlWeakAssert(!visibleValueRange.isEmpty());
        auto const verticalRange = bounds.getVerticalRange();
        anlWeakAssert(!verticalRange.isEmpty());
        if(visibleValueRange.isEmpty() || verticalRange.isEmpty())
        {
            return 0;
        }
        
        value = (value - static_cast<float>(visibleValueRange.getStart())) / static_cast<float>(visibleValueRange.getLength());
        value = (1.0f - value) * static_cast<float>(verticalRange.getLength()) + static_cast<float>(verticalRange.getStart());
        return static_cast<int>(std::floor(value));
    };
    
    auto it = Results::getResultAt(results, time);
    if(it == results.cend())
    {
        return;
    }
    auto const displayMode = getDisplayMode();
    switch(displayMode)
    {
        case DisplayMode::unsupported:
            break;
        case DisplayMode::surface:
        {
            g.setColour(colours.foreground);
            g.fillRect(bounds);
        }
            break;
        case DisplayMode::bar:
        {
            anlWeakAssert(!it->values.empty());
            if(it->values.empty())
            {
                return;
            }
            
            auto const area = bounds.withTop(getScaledValue(it->values[0])).toFloat();
            g.setColour(colours.foreground.withAlpha(0.2f));
            g.fillRect(area);
            
            g.setColour(colours.foreground);
            g.fillRect(area.withHeight(1.0f));
        }
            break;
        case DisplayMode::segment:
        {
            auto const realTime = Vamp::RealTime::fromSeconds(time);
            anlWeakAssert(!it->values.empty());
            if(it->values.empty())
            {
                return;
            }
            
            auto getValue = [&]()
            {
                if(it != results.cbegin())
                {
                    auto const prev = std::prev(it, 1);
                    auto const ratio = static_cast<float>((realTime - prev->timestamp) / (it->timestamp - prev->timestamp));
                    return prev->values[0] * (1.0f - ratio) + it->values[0] * ratio;
                }
                return it->values[0];
            };
            
            auto const value = getValue();
            auto const area = bounds.withTop(getScaledValue(value)).toFloat();
            
            g.setColour(colours.foreground.withAlpha(0.2f));
            g.fillRect(area);
            
            g.setColour(colours.foreground);
            g.fillRect(area.withHeight(1.0f));
        }
            break;
        case DisplayMode::matrix:
        {
            juce::Image image(juce::Image::PixelFormat::ARGB, 1, static_cast<int>(numDimensions), false);
            juce::Image::BitmapData const data(image, juce::Image::BitmapData::writeOnly);
            
            auto const colourMap = colours.map;
            auto valueToColour = [&](float const value)
            {
                auto const& valueRange = valueZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
                auto const ratio = (value - valueRange.getStart()) / valueRange.getLength();
                auto const color = tinycolormap::GetColor(static_cast<double>(ratio), colourMap);
                return juce::Colour::fromFloatRGBA(static_cast<float>(color.r()), static_cast<float>(color.g()), static_cast<float>(color.b()), 1.0f);
            };
            
            for(int j = 0; j < image.getHeight(); ++j)
            {
                auto const colour = valueToColour(it->values[static_cast<size_t>(j)]);
                data.setPixelColour(0, image.getHeight() - 1 - j, colour);
            }
            
            auto const width = static_cast<float>(bounds.getWidth());
            auto const height = bounds.getHeight();
            
            auto const& binZoomAcsr = mAccessor.getAccessor<AcsrType::binZoom>(0);
            auto const binVisibleRange = binZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            auto const binGlobalRange = binZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
            
            auto const valueRange = juce::Range<double>(binGlobalRange.getEnd() - binVisibleRange.getEnd(), binGlobalRange.getEnd() - binVisibleRange.getStart());
            
            auto const deltaY = static_cast<float>(valueRange.getStart() / binGlobalRange.getLength() * static_cast<double>(image.getHeight()));
            auto const scaleY = static_cast<float>(binGlobalRange.getLength() / valueRange.getLength() * static_cast<double>(height) / static_cast<double>(image.getHeight()));
            
            auto const transform = juce::AffineTransform::translation(0.0f, -deltaY).scaled(width, scaleY).translated(static_cast<float>(bounds.getX()), static_cast<float>(bounds.getY()));
            
            g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
            g.drawImageTransformed(image, transform);
        }
            break;
    }
}

ANALYSE_FILE_END
