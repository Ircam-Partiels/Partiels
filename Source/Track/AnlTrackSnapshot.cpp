#include "AnlTrackSnapshot.h"
#include "AnlTrackRenderer.h"
#include "AnlTrackTools.h"
#include "AnlTrackTooltip.h"

ANALYSE_FILE_BEGIN

Track::Snapshot::Snapshot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
, mTransportAccessor(transportAccessor)
{
    setInterceptsMouseClicks(false, false);
    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::key:
            case AttrType::input:
            case AttrType::file:
            case AttrType::name:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomLink:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::zoomAcsr:
            case AttrType::focused:
            case AttrType::showInGroup:
                break;
            case AttrType::description:
            case AttrType::grid:
            case AttrType::results:
            case AttrType::edit:
            case AttrType::graphics:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::unit:
            case AttrType::channelsLayout:
            {
                repaint();
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
            case Zoom::AttrType::anchor:
                break;
            case Zoom::AttrType::visibleRange:
            {
                if(Tools::getFrameType(mAccessor) != Track::FrameType::label)
                {
                    repaint();
                }
            }
            break;
        }
    };

    mGridListener.onAttrChanged = [this](Zoom::Grid::Accessor const& acsr, Zoom::Grid::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };

    mTransportListener.onAttrChanged = [this](Transport::Accessor const& acsr, Transport::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Transport::AttrType::startPlayhead:
            {
                if(!acsr.getAttr<Transport::AttrType::playback>())
                {
                    if(Tools::getFrameType(mAccessor) != Track::FrameType::label)
                    {
                        repaint();
                    }
                }
            }
            break;
            case Transport::AttrType::runningPlayhead:
            {
                if(acsr.getAttr<Transport::AttrType::playback>())
                {
                    if(Tools::getFrameType(mAccessor) != Track::FrameType::label)
                    {
                        repaint();
                    }
                }
            }
            break;
            case Transport::AttrType::playback:
            case Transport::AttrType::looping:
            case Transport::AttrType::loopRange:
            case Transport::AttrType::stopAtLoopEnd:
            case Transport::AttrType::gain:
            case Transport::AttrType::markers:
            case Transport::AttrType::magnetize:
            case Transport::AttrType::autoScroll:
            case Transport::AttrType::selection:
                break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::valueZoom>().addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::binZoom>().addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
    mTransportAccessor.addListener(mTransportListener, NotificationType::synchronous);
}

Track::Snapshot::~Snapshot()
{
    mTransportAccessor.removeListener(mTransportListener);
    mAccessor.getAcsr<AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
    mAccessor.getAcsr<AcsrType::binZoom>().removeListener(mZoomListener);
    mAccessor.getAcsr<AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
    mAccessor.getAcsr<AcsrType::valueZoom>().removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Track::Snapshot::paint(juce::Graphics& g)
{
    auto const isPlaying = mTransportAccessor.getAttr<Transport::AttrType::playback>();
    auto const time = isPlaying ? mTransportAccessor.getAttr<Transport::AttrType::runningPlayhead>() : mTransportAccessor.getAttr<Transport::AttrType::startPlayhead>();
    paint(mAccessor, mTimeZoomAccessor, time, g, getLocalBounds(), findColour(Decorator::ColourIds::normalBorderColourId));
}

void Track::Snapshot::paintGrid(Accessor const& accessor, juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour const colour)
{
    if(colour.isTransparent() || accessor.getAttr<AttrType::grid>() == GridMode::hidden)
    {
        return;
    }

    using Justification = Zoom::Grid::Justification;
    auto const justificationHorizontal = Justification(Zoom::Grid::Justification::left);

    auto const paintChannel = [&](Zoom::Accessor const& zoomAcsr, juce::Rectangle<int> const& region)
    {
        g.setColour(colour);
        Zoom::Grid::paintVertical(g, zoomAcsr.getAcsr<Zoom::AcsrType::grid>(), zoomAcsr.getAttr<Zoom::AttrType::visibleRange>(), region, nullptr, justificationHorizontal);
    };

    switch(Tools::getFrameType(accessor))
    {
        case Track::FrameType::label:
        {
            Renderer::paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int>, size_t)
                                    {
                                    });
        }
        break;
        case Track::FrameType::value:
        {
            Renderer::paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int> region, size_t)
                                    {
                                        paintChannel(accessor.getAcsr<AcsrType::valueZoom>(), region);
                                    });
        }
        break;
        case Track::FrameType::vector:
        {
            Renderer::paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int> region, size_t)
                                    {
                                        paintChannel(accessor.getAcsr<AcsrType::binZoom>(), region);
                                    });
        }
        break;
    }
}

void Track::Snapshot::paint(Accessor const& accessor, Zoom::Accessor const& timeZoomAcsr, double time, juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour const colour)
{
    switch(Tools::getFrameType(accessor))
    {
        case Track::FrameType::label:
        {
            paintGrid(accessor, g, bounds, colour);
            Renderer::paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int>, size_t)
                                    {
                                    });
        }
        break;
        case Track::FrameType::value:
        {
            paintGrid(accessor, g, bounds, colour);
            Renderer::paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int> region, size_t channel)
                                    {
                                        paintPoints(accessor, channel, g, region, timeZoomAcsr, time);
                                    });
        }
        break;
        case Track::FrameType::vector:
        {
            Renderer::paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int> region, size_t channel)
                                    {
                                        paintColumns(accessor, channel, g, region, timeZoomAcsr, time);
                                    });
            paintGrid(accessor, g, bounds, colour);
        }
        break;
    }
}

void Track::Snapshot::paintPoints(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr, double time)
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

    auto const& valueRange = accessor.getAcsr<AcsrType::valueZoom>().getAttr<Zoom::AttrType::visibleRange>();
    if(valueRange.isEmpty())
    {
        return;
    }

    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access) || results.isEmpty())
    {
        return;
    }
    auto const value = Results::getValue(results.getPoints(), channel, time);
    if(!value.has_value())
    {
        return;
    }
    auto const y = Tools::valueToPixel(*value, valueRange, bounds.toFloat());
    auto const shadowColour = accessor.getAttr<AttrType::colours>().shadow;
    if(!shadowColour.isTransparent())
    {
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawLine(clipBounds.getX(), y + 2.0f, clipBounds.getRight(), y + 2.0f, 1.0f);
        g.setColour(juce::Colours::black.withAlpha(0.75f));
        g.drawLine(clipBounds.getX(), y + 1.0f, clipBounds.getRight(), y + 1.0f, 1.0f);
    }
    g.setColour(accessor.getAttr<AttrType::colours>().foreground);
    g.drawLine(clipBounds.getX(), y, clipBounds.getRight(), y, 1.0f);
}

void Track::Snapshot::paintColumns(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr, double time)
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
    if(images.empty() || images.size() <= channel || images.at(channel).empty())
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

        auto const clipBounds = g.getClipBounds().constrainedWithin(bounds);
        if(clipBounds.isEmpty())
        {
            return;
        }

        auto const xClippedRange = clipZoomRange(bounds.getHorizontalRange(), clipBounds.getHorizontalRange(), {t, t});
        auto const xRange = toImageRange(xZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), xClippedRange, image.getWidth());

        auto const yClippedRange = clipZoomRange(bounds.getVerticalRange(), clipBounds.getVerticalRange(), getZoomRange(yZoomAcsr, true));
        auto const yRange = toImageRange(yZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), yClippedRange, image.getHeight());

        g.setColour(juce::Colours::black);
        g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
        Renderer::paintClippedImage(g, image, {std::floor(xRange.getStart()), yRange.getStart(), 1.0f, yRange.getLength()});
    };

    renderImage(images.at(channel).back(), time, timeZoomAcsr, accessor.getAcsr<AcsrType::binZoom>());
}

Track::Snapshot::Overlay::Overlay(Snapshot& snapshot)
: mSnapshot(snapshot)
, mAccessor(mSnapshot.mAccessor)
, mTransportAccessor(mSnapshot.mTransportAccessor)
{
    addAndMakeVisible(mSnapshot);
    setInterceptsMouseClicks(true, true);

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::key:
            case AttrType::input:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomLink:
            case AttrType::graphics:
            case AttrType::zoomAcsr:
            case AttrType::focused:
            case AttrType::grid:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::results:
            case AttrType::edit:
            case AttrType::showInGroup:
                break;
            case AttrType::colours:
            case AttrType::font:
            {
                repaint();
            }
            break;
            case AttrType::channelsLayout:
            {
                updateTooltip(getMouseXYRelative());
            }
            break;
            case AttrType::file:
            case AttrType::description:
            case AttrType::unit:
            case AttrType::name:
            {
                juce::SettableTooltipClient::setTooltip(Tools::getInfoTooltip(acsr));
            }
            break;
        }
    };

    mTransportListener.onAttrChanged = [this](Transport::Accessor const& acsr, Transport::AttrType attribute)
    {
        switch(attribute)
        {
            case Transport::AttrType::startPlayhead:
            {
                if(!acsr.getAttr<Transport::AttrType::playback>())
                {
                    updateTooltip(getMouseXYRelative());
                }
            }
            break;
            case Transport::AttrType::runningPlayhead:
            {
                if(acsr.getAttr<Transport::AttrType::playback>())
                {
                    updateTooltip(getMouseXYRelative());
                }
            }
            break;
            case Transport::AttrType::playback:
            case Transport::AttrType::looping:
            case Transport::AttrType::loopRange:
            case Transport::AttrType::stopAtLoopEnd:
            case Transport::AttrType::gain:
            case Transport::AttrType::markers:
            case Transport::AttrType::magnetize:
            case Transport::AttrType::autoScroll:
            case Transport::AttrType::selection:
                break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mTransportAccessor.addListener(mTransportListener, NotificationType::synchronous);
}

Track::Snapshot::Overlay::~Overlay()
{
    mTransportAccessor.removeListener(mTransportListener);
    mAccessor.removeListener(mListener);
}

void Track::Snapshot::Overlay::resized()
{
    auto const bounds = getLocalBounds();
    mSnapshot.setBounds(bounds);
}

void Track::Snapshot::Overlay::paint(juce::Graphics& g)
{
    g.fillAll(mAccessor.getAttr<AttrType::colours>().background);
}

void Track::Snapshot::Overlay::mouseMove(juce::MouseEvent const& event)
{
    updateTooltip(getLocalPoint(event.eventComponent, juce::Point<int>{event.x, event.y}));
}

void Track::Snapshot::Overlay::mouseEnter(juce::MouseEvent const& event)
{
    updateTooltip(getLocalPoint(event.eventComponent, juce::Point<int>{event.x, event.y}));
}

void Track::Snapshot::Overlay::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    Tooltip::BubbleClient::setTooltip("");
}

void Track::Snapshot::Overlay::updateTooltip(juce::Point<int> const& pt)
{
    if(Tools::getFrameType(mAccessor) == Track::FrameType::label || !getLocalBounds().contains(pt))
    {
        Tooltip::BubbleClient::setTooltip("");
        return;
    }
    juce::StringArray lines;
    auto const isPlaying = mTransportAccessor.getAttr<Transport::AttrType::playback>();
    auto const time = isPlaying ? mTransportAccessor.getAttr<Transport::AttrType::runningPlayhead>() : mTransportAccessor.getAttr<Transport::AttrType::startPlayhead>();
    lines.add(juce::translate("Time: TIME").replace("TIME", Format::secondsToString(time)));
    lines.add(juce::translate("Mouse: VALUE").replace("VALUE", Tools::getZoomTootip(mAccessor, *this, pt.y)));
    lines.addArray(Tools::getValueTootip(mAccessor, mSnapshot.mTimeZoomAccessor, *this, pt.y, time));
    Tooltip::BubbleClient::setTooltip(lines.joinIntoString("\n"));
}

ANALYSE_FILE_END
