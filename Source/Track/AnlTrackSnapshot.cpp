#include "AnlTrackSnapshot.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Snapshot::Snapshot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
{
    setInterceptsMouseClicks(false, false);
    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::key:
            case AttrType::name:
            case AttrType::description:
            case AttrType::state:
            case AttrType::height:
            case AttrType::propertyState:
            case AttrType::warnings:
                break;
            case AttrType::results:
            case AttrType::time:
            case AttrType::graphics:
            case AttrType::processing:
            case AttrType::colours:
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
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).addListener(mValueZoomListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::binZoom>(0).addListener(mBinZoomListener, NotificationType::synchronous);
}

Track::Snapshot::~Snapshot()
{
    mAccessor.getAccessor<AcsrType::binZoom>(0).removeListener(mBinZoomListener);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).removeListener(mValueZoomListener);
    mAccessor.removeListener(mListener);
}

void Track::Snapshot::paint(juce::Graphics& g)
{
    auto const results = mAccessor.getAttr<AttrType::results>();
    if(results == nullptr || results->empty())
    {
        return;
    }
    
    auto const bounds = getLocalBounds();
    if(bounds.isEmpty())
    {
        return;
    }
    
    auto const& output = mAccessor.getAttr<AttrType::description>().output;
    auto const time = mAccessor.getAttr<AttrType::time>();
    auto const& valueRange = mAccessor.getAccessor<AcsrType::valueZoom>(0).getAttr<Zoom::AttrType::visibleRange>();
    if(output.hasFixedBinCount)
    {
        switch(output.binCount)
        {
            case 0:
            {
                paintMarker(g, bounds.toFloat(), mAccessor.getAttr<AttrType::colours>().foreground, *results, time);
            }
                break;
            case 1:
            {
                paintSegment(g, bounds.toFloat(), mAccessor.getAttr<AttrType::colours>().foreground, *results, time, valueRange);
            }
                break;
            default:
            {
                paintGrid(g, bounds, mAccessor.getAttr<AttrType::graphics>(), time, mTimeZoomAccessor, mAccessor.getAccessor<AcsrType::binZoom>(0));
            }
                break;
        }
    }
    else
    {
        paintSegment(g, bounds.toFloat(), mAccessor.getAttr<AttrType::colours>().foreground, *results, time, valueRange);
    }
}

void Track::Snapshot::paintMarker(juce::Graphics& g, juce::Rectangle<float> const& bounds, juce::Colour const& colour, std::vector<Plugin::Result> const& results, double time)
{
    auto const rt = Tools::secondsToRealTime(time);
    auto it = std::find_if(results.cbegin(), results.cend(), [&](Plugin::Result const& result)
    {
        return result.hasTimestamp && Tools::getEndRealTime(result) >= rt;
    });
    if(it != results.cend() && Tools::getEndRealTime(*it) <= rt)
    {
        g.setColour(colour);
        g.fillRect(bounds);
    }
}

void Track::Snapshot::paintSegment(juce::Graphics& g, juce::Rectangle<float> const& bounds, juce::Colour const& colour, std::vector<Plugin::Result> const& results, double time, juce::Range<double> const& valueRange)
{
    auto const rt = Tools::secondsToRealTime(time);
    auto const second = std::find_if(results.cbegin(), results.cend(), [&](Plugin::Result const& result)
    {
        return result.hasTimestamp && Tools::getEndRealTime(result) >= rt;
    });
    if(second == results.cend())
    {
        return;
    }
    
    auto const clipBounds = g.getClipBounds().toFloat();
    auto paintLineWithShadow = [&](float const y)
    {
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        g.drawLine(clipBounds.getX(), y + 3.0f, clipBounds.getRight(), y + 3.0f,  1.0f);
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawLine(clipBounds.getX(), y + 2.0f, clipBounds.getRight(), y + 2.0f,  1.0f);
        g.setColour(juce::Colours::black.withAlpha(0.75f));
        g.drawLine(clipBounds.getX(), y + 1.0f, clipBounds.getRight(), y + 1.0f,  1.0f);
        g.setColour(colour);
        g.drawLine(clipBounds.getX(), y, clipBounds.getRight(), y ,  1.0f);
    };
    
    auto const first = second != results.cbegin() ? std::prev(second) : results.cbegin();
    if(second->timestamp <= rt && Tools::getEndRealTime(*second) >= rt)
    {
        if(!second->values.empty())
        {
            paintLineWithShadow(Tools::valueToPixel(second->values[0], valueRange, bounds));
        }
    }
    else if(first->timestamp <= rt && Tools::getEndRealTime(*first) >= rt)
    {
        if(!first->values.empty())
        {
            paintLineWithShadow(Tools::valueToPixel(first->values[0], valueRange, bounds));
        }
    }
    else if(first != second && first->hasTimestamp)
    {
        if(!first->values.empty() && !second->values.empty())
        {
            auto const start = Tools::realTimeToSeconds(Tools::getEndRealTime(*first));
            auto const end = Tools::realTimeToSeconds(second->timestamp);
            anlStrongAssert(end > start);
            if(end <= start)
            {
                return;
            }
            auto const ratio = static_cast<float>((time - start) / (end - start));
            auto const value = (1.0f - ratio) * first->values[0] + ratio * second->values[0];
            paintLineWithShadow(Tools::valueToPixel(value, valueRange, bounds));
        }
    }
}

void Track::Snapshot::paintGrid(juce::Graphics& g, juce::Rectangle<int> const& bounds, std::vector<juce::Image> const& images, double time, Zoom::Accessor const& timeZoomAcsr, Zoom::Accessor const& binZoomAcsr)
{
    if(images.empty())
    {
        return;
    }
    
    auto renderImage = [&](juce::Image const& image, double t,  Zoom::Accessor const& xZoomAcsr, Zoom::Accessor const& yZoomAcsr)
    {
        if(!image.isValid())
        {
            return;
        }
        
        using PixelRange = juce::Range<int>;
        using ZoomRange = Zoom::Range;
        
        // Gets the visible zoom range of a zoom accessor and inverses it if necessary
        auto const getZoomRange = [](Zoom::Accessor const& zoomAcsr, bool inverted)
        {
            if(!inverted)
            {
                return zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            }
            auto const& globalRange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
            auto const& visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            return ZoomRange{globalRange.getEnd() - visibleRange.getEnd() + globalRange.getStart(), globalRange.getEnd() - visibleRange.getStart()+ globalRange.getStart()};
        };
        
        // Gets the visible zoom range equivalent to the graphics clip bounds
        auto const clipZoomRange = [](PixelRange const& global, PixelRange const& visible, ZoomRange const& zoom)
        {
            auto const ratio = zoom.getLength() / static_cast<double>(global.getLength());
            auto const x1 = static_cast<double>(visible.getStart() - global.getStart()) * ratio + zoom.getStart();
            auto const x2 = static_cast<double>(visible.getEnd() - global.getStart()) * ratio + zoom.getStart();
            return ZoomRange{x1, x2};
        };
        
        // Converts the visible zoom range to image range
        auto toImageRange = [](ZoomRange const& globalRange, ZoomRange const& visibleRange, int imageSize)
        {
            auto const globalLength = globalRange.getLength();
            anlStrongAssert(globalLength > 0.0);
            if(globalLength <= 0.0)
            {
                return juce::Range<float>();
            }
            auto const scale = static_cast<float>(imageSize);
            auto scaleValue = [&](double value)
            {
                return static_cast<float>((value - globalRange.getStart()) / globalLength * scale);
            };
            return juce::Range<float>{scaleValue(visibleRange.getStart()), scaleValue(visibleRange.getEnd())};
        };
        
        // Draws a range of an image
        auto drawImage = [&](juce::Rectangle<float> const& rectangle)
        {
            auto const graphicsBounds = g.getClipBounds().toFloat();
            auto const imageBounds = rectangle.getSmallestIntegerContainer();
            auto const clippedImage = image.getClippedImage(imageBounds);
            anlWeakAssert(clippedImage.getWidth() == imageBounds.getWidth());
            anlWeakAssert(clippedImage.getHeight() == imageBounds.getHeight());
            auto const deltaX = static_cast<float>(imageBounds.getX()) - rectangle.getX();
            auto const deltaY = static_cast<float>(imageBounds.getY()) - rectangle.getY();
            anlWeakAssert(deltaX <= 0);
            anlWeakAssert(deltaY <= 0);
            auto const scaleX = graphicsBounds.getWidth() / rectangle.getWidth();
            auto const scaleY = graphicsBounds.getHeight() / rectangle.getHeight();
            
            g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
            g.drawImageTransformed(clippedImage, juce::AffineTransform::translation(deltaX, deltaY).scaled(scaleX, scaleY).translated(graphicsBounds.getX(), graphicsBounds.getY()));
        };
        
        auto const clipBounds = g.getClipBounds().constrainedWithin(bounds);
        
        auto const xClippedRange = clipZoomRange(bounds.getHorizontalRange(), clipBounds.getHorizontalRange(), {t, t});
        auto const xRange = toImageRange(xZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), xClippedRange, image.getWidth());
        
        auto const yClippedRange = clipZoomRange(bounds.getVerticalRange(), clipBounds.getVerticalRange(), getZoomRange(yZoomAcsr, true));
        auto const yRange = toImageRange(yZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), yClippedRange, image.getHeight());
        
        drawImage({std::floor(xRange.getStart()), yRange.getStart(), 1.0f, yRange.getLength()});
    };
    
    renderImage(images.back(), time, timeZoomAcsr, binZoomAcsr);
}

Track::Snapshot::Overlay::Overlay(Snapshot& snapshot)
: mSnapshot(snapshot)
, mAccessor(mSnapshot.mAccessor)
{
    addAndMakeVisible(mSnapshot);
    mTooltip.setEditable(false);
    mTooltip.setJustificationType(juce::Justification::topLeft);
    mTooltip.setInterceptsMouseClicks(false, false);
    mTooltip.setComponentEffect(&mDropShadowEffect);
    addChildComponent(mTooltip);
    setInterceptsMouseClicks(true, true);

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::key:
            case AttrType::state:
            case AttrType::height:
            case AttrType::propertyState:
            case AttrType::graphics:
            case AttrType::name:
            case AttrType::description:
            case AttrType::results:
                break;
            case AttrType::time:
            {
                for(auto const& mouseSource : juce::Desktop::getInstance().getMouseSources())
                {
                    if(mouseSource.getComponentUnderMouse() == this && (mouseSource.isDragging() || !mouseSource.isTouch()))
                    {
                        mouseSource.triggerFakeMove();
                    }
                }
            }
                break;
            case AttrType::colours:
            {
                auto const colours = acsr.getAttr<AttrType::colours>();
                mTooltip.setColour(juce::Label::ColourIds::textColourId, colours.foreground);
                setOpaque(colours.background.isOpaque());
            }
                break;
            case AttrType::warnings:
            case AttrType::processing:
                break;
        }
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::Snapshot::Overlay::~Overlay()
{
    mAccessor.removeListener(mListener);
}

void Track::Snapshot::Overlay::resized()
{
    auto const bounds = getLocalBounds();
    mSnapshot.setBounds(bounds);
    mTooltip.setBounds(0, 0, getWidth(), 40);
}

void Track::Snapshot::Overlay::paint(juce::Graphics& g)
{
    auto const& colours = mAccessor.getAttr<AttrType::colours>();
    g.setColour(colours.background);
    g.fillRect(getLocalBounds());
}

void Track::Snapshot::Overlay::mouseMove(juce::MouseEvent const& event)
{
    auto const name = mAccessor.getAttr<AttrType::name>();
    auto getTooltip = [&]() -> juce::String
    {
        auto const results = mAccessor.getAttr<AttrType::results>();
        auto const& output = mAccessor.getAttr<AttrType::description>().output;
        if(results == nullptr || results->empty())
        {
            return "";
        }
        auto const time = mAccessor.getAttr<AttrType::time>();
        if(output.hasFixedBinCount)
        {
            switch(output.binCount)
            {
                case 0:
                {
                    return Tools::getMarkerText(*results, output, time);
                }
                    break;
                case 1:
                {
                    return Tools::getSegmentText(*results, output, time);
                }
                    break;
                default:
                {
                    auto const& binVisibleRange = mAccessor.getAccessor<AcsrType::binZoom>(0).getAttr<Zoom::AttrType::visibleRange>();
                    auto const y = static_cast<float>(getHeight() - 1 - event.y) / static_cast<float>(getHeight());
                    auto const bin = static_cast<size_t>(std::floor(y * binVisibleRange.getLength() + binVisibleRange.getStart()));
                    auto const binName = "bin" + juce::String(bin) + (bin < output.binNames.size() && !output.binNames[bin].empty() ? ("(" + output.binNames[bin] + ")") : "");
                    return binName + " • " + Tools::getGridText(*results, output, time, bin);
                }
                    break;
            }
        }
        return Tools::getSegmentText(*results, output, time);
    };
    
    auto const tip = getTooltip();
    setTooltip(name + ": " + tip);
    mTooltip.setText(tip, juce::NotificationType::dontSendNotification);
}

void Track::Snapshot::Overlay::mouseEnter(juce::MouseEvent const& event)
{
    mTooltip.setVisible(true);
    mouseMove(event);
}

void Track::Snapshot::Overlay::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    mTooltip.setVisible(false);
}

ANALYSE_FILE_END
