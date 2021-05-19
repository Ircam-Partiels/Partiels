#include "AnlTrackSnapshot.h"
#include "../Zoom/AnlZoomTools.h"
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
            case AttrType::zoomLink:
            case AttrType::warnings:
            case AttrType::zoomAcsr:
            case AttrType::focused:
                break;
            case AttrType::results:
            case AttrType::time:
            case AttrType::graphics:
            case AttrType::processing:
            case AttrType::colours:
            {
                if(Tools::getDisplayType(mAccessor) != Tools::DisplayType::markers)
                {
                    repaint();
                }
            }
            break;
        }
    };

    mZoomListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::minimumLength:
                break;
            case Zoom::AttrType::visibleRange:
            {
                if(Tools::getDisplayType(mAccessor) != Tools::DisplayType::markers)
                {
                    repaint();
                }
            }
                break;
            case Zoom::AttrType::anchor:
                break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::valueZoom>().addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::binZoom>().addListener(mZoomListener, NotificationType::synchronous);
}

Track::Snapshot::~Snapshot()
{
    mAccessor.getAcsr<AcsrType::binZoom>().removeListener(mZoomListener);
    mAccessor.getAcsr<AcsrType::valueZoom>().removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Track::Snapshot::paint(juce::Graphics& g)
{
    switch(Tools::getDisplayType(mAccessor))
    {
        case Tools::DisplayType::markers:
            break;
        case Tools::DisplayType::segments:
        {
            paintPoints(mAccessor, g, getLocalBounds(), mTimeZoomAccessor);
        }
        break;
        case Tools::DisplayType::grid:
        {
            paintColumns(mAccessor, g, getLocalBounds(), mTimeZoomAccessor);
        }
        break;
    }
}

void Track::Snapshot::paintPoints(Accessor const& accessor, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr)
{
    if(bounds.isEmpty())
    {
        return;
    }

    auto const clipBounds = g.getClipBounds().toFloat();
    if(clipBounds.isEmpty())
    {
        return;
    }

    auto const& globalRange = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
    if(globalRange.isEmpty())
    {
        return;
    }

    auto const results = accessor.getAttr<AttrType::results>();
    auto const points = results.getPoints();
    if(points == nullptr || points->empty())
    {
        return;
    }

    auto const value = Tools::getValue(points, globalRange, accessor.getAttr<AttrType::time>());
    if(!value.has_value())
    {
        return;
    }
    auto const y = Tools::valueToPixel(*value, accessor.getAcsr<AcsrType::valueZoom>().getAttr<Zoom::AttrType::visibleRange>(), bounds.toFloat());
    g.setColour(juce::Colours::black.withAlpha(0.25f));
    g.drawLine(clipBounds.getX(), y + 3.0f, clipBounds.getRight(), y + 3.0f, 1.0f);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawLine(clipBounds.getX(), y + 2.0f, clipBounds.getRight(), y + 2.0f, 1.0f);
    g.setColour(juce::Colours::black.withAlpha(0.75f));
    g.drawLine(clipBounds.getX(), y + 1.0f, clipBounds.getRight(), y + 1.0f, 1.0f);
    g.setColour(accessor.getAttr<AttrType::colours>().foreground);
    g.drawLine(clipBounds.getX(), y, clipBounds.getRight(), y, 1.0f);
}

void Track::Snapshot::paintColumns(Accessor const& accessor, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr)
{
    if(bounds.isEmpty())
    {
        return;
    }

    auto const& globalTimeRange = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
    if(globalTimeRange.isEmpty())
    {
        return;
    }

    auto const images = accessor.getAttr<AttrType::graphics>();
    if(images.empty())
    {
        return;
    }

    auto renderImage = [&](juce::Image const& image, double t, Zoom::Accessor const& xZoomAcsr, Zoom::Accessor const& yZoomAcsr)
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
            return ZoomRange{globalRange.getEnd() - visibleRange.getEnd() + globalRange.getStart(), globalRange.getEnd() - visibleRange.getStart() + globalRange.getStart()};
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
        if(clipBounds.isEmpty())
        {
            return;
        }

        auto const xClippedRange = clipZoomRange(bounds.getHorizontalRange(), clipBounds.getHorizontalRange(), {t, t});
        auto const xRange = toImageRange(xZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), xClippedRange, image.getWidth());

        auto const yClippedRange = clipZoomRange(bounds.getVerticalRange(), clipBounds.getVerticalRange(), getZoomRange(yZoomAcsr, true));
        auto const yRange = toImageRange(yZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), yClippedRange, image.getHeight());

        drawImage({std::floor(xRange.getStart()), yRange.getStart(), 1.0f, yRange.getLength()});
    };

    renderImage(images.back(), accessor.getAttr<AttrType::time>(), timeZoomAcsr, accessor.getAcsr<AcsrType::binZoom>());
}

Track::Snapshot::Overlay::Overlay(Snapshot& snapshot)
: mSnapshot(snapshot)
, mAccessor(mSnapshot.mAccessor)
{
    addAndMakeVisible(mSnapshot);
    setInterceptsMouseClicks(true, true);

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::key:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomLink:
            case AttrType::graphics:
            case AttrType::name:
            case AttrType::description:
            case AttrType::results:
            case AttrType::zoomAcsr:
            case AttrType::focused:
                break;
            case AttrType::time:
            {
                updateTooltip(getMouseXYRelative());
            }
            break;
            case AttrType::colours:
            {
                setOpaque(acsr.getAttr<AttrType::colours>().background.isOpaque());
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
    mSnapshot.setBounds(getLocalBounds());
}

void Track::Snapshot::Overlay::paint(juce::Graphics& g)
{
    g.fillAll(mAccessor.getAttr<AttrType::colours>().background);
}

void Track::Snapshot::Overlay::mouseMove(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
}

void Track::Snapshot::Overlay::mouseEnter(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
}

void Track::Snapshot::Overlay::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    setTooltip("");
}

void Track::Snapshot::Overlay::updateTooltip(juce::Point<int> const& pt)
{
    if(Tools::getDisplayType(mAccessor) == Tools::DisplayType::markers)
    {
        setTooltip("");
        return;
    }
    if(!getLocalBounds().contains(pt))
    {
        setTooltip("");
        return;
    }
    auto const& globalRange = mSnapshot.mTimeZoomAccessor.getAttr<Zoom::AttrType::globalRange>();
    auto const time = mAccessor.getAttr<AttrType::time>();
    auto const bin = Zoom::Tools::getScaledValueFromHeight(mAccessor.getAcsr<AcsrType::binZoom>(), *this, pt.y);
    auto const tip = Tools::getResultText(mAccessor, globalRange, time, static_cast<size_t>(std::floor(bin)), 0.0);
    setTooltip(Format::secondsToString(time) + ": " + (tip.isEmpty() ? "-" : tip));
}

ANALYSE_FILE_END
