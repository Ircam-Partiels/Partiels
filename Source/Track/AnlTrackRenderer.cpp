#include "AnlTrackRenderer.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    namespace Renderer
    {
        class PathArrangement
        {
        public:
            inline bool isLineStarted() const
            {
                return !mShouldStartNewPath;
            }

            inline void stopLine()
            {
                if(!std::exchange(mShouldStartNewPath, true))
                {
                    mPath.lineTo(mLastPoint);
                }
            }

            void addLine(float const x1, float const x2, float const y)
            {
                if(std::exchange(mShouldStartNewPath, false))
                {
                    mPath.startNewSubPath(x1, y);
                    mLastPoint = {x2, y};
                }
                else if(std::abs(y - mLastPoint.y) > std::numeric_limits<float>::epsilon())
                {
                    mPath.lineTo(mLastPoint);
                    mPath.lineTo(x1, y);
                    mLastPoint = {x2, y};
                }
                else
                {
                    mLastPoint = {x2, y};
                }
            }

            void draw(juce::Graphics& g, juce::Colour const& foreground, juce::Colour const& shadow, float lineWidth)
            {
                stopLine();
                if(mPath.isEmpty())
                {
                    return;
                }

                juce::PathStrokeType const pathStrokeType(lineWidth);
                pathStrokeType.createStrokedPath(mPath, mPath);
                if(!shadow.isTransparent())
                {
                    for(auto i = 0; i < 4; ++i)
                    {
                        g.setColour(shadow.withMultipliedAlpha(static_cast<float>(i + 1) / 4.0f));
                        g.fillPath(mPath, juce::AffineTransform::translation(0.0f, 2.0f - static_cast<float>(i) / 2.0f));
                    }
                }

                if(!foreground.isTransparent())
                {
                    g.setColour(foreground);
                    g.fillPath(mPath);
                }
            }

        private:
            juce::Path mPath;
            juce::Point<float> mLastPoint;
            bool mShouldStartNewPath{true};
        };

        class LabelArrangement
        {
        public:
            LabelArrangement(juce::Font const& font, juce::String const unit, int numDecimals)
            : mFont(font)
            , mUnit(unit)
            , mNumDecimal(numDecimals)
            {
            }

            ~LabelArrangement() = default;

            void addValue(float const value, float const x, float const y)
            {
                if(std::abs(mLastValue - value) < std::numeric_limits<float>::epsilon())
                {
                    return;
                }

                if(!mHighInfo.text.isEmpty() && x > mHighInfo.x + mHighInfo.w)
                {
                    if(mLowInfo.text.isEmpty() && y < mHighInfo.y)
                    {
                        mGlyphArrangement.addLineOfText(mFont, mHighInfo.text + mUnit, mHighInfo.x, mHighInfo.y + mLowInfo.offset);
                    }
                    else
                    {
                        mGlyphArrangement.addLineOfText(mFont, mHighInfo.text + mUnit, mHighInfo.x, mHighInfo.y + mHighInfo.offset);
                    }
                    mHighInfo.text.clear();
                }
                if(!mLowInfo.text.isEmpty() && x > mLowInfo.x + mLowInfo.w)
                {
                    mGlyphArrangement.addLineOfText(mFont, mLowInfo.text + mUnit, mLowInfo.x, mLowInfo.y + mLowInfo.offset);
                    mLowInfo.text.clear();
                }

                auto const shouldInsertHight = !mHighInfo.text.isEmpty() && y < mHighInfo.y;
                auto const shouldInsertLow = !mLowInfo.text.isEmpty() && y > mLowInfo.y;
                if(shouldInsertHight || (!shouldInsertLow && mHighInfo.text.isEmpty()))
                {
                    mHighInfo.text = Format::valueToString(value, mNumDecimal);
                    mHighInfo.x = x;
                    mHighInfo.y = y;
                    mHighInfo.w = std::ceil(static_cast<float>(mHighInfo.text.length()) * mCharWidth + mUnitWidth + 4.0f);
                    mLastValue = value;
                }
                else if(shouldInsertLow || mLowInfo.text.isEmpty())
                {
                    mLowInfo.text = Format::valueToString(value, mNumDecimal);
                    mLowInfo.x = x;
                    mLowInfo.y = y;
                    mLowInfo.w = std::ceil(static_cast<float>(mLowInfo.text.length()) * mCharWidth + mUnitWidth + 4.0f);
                    mLastValue = value;
                }
            }
            void draw(juce::Graphics& g, juce::Colour const& colour)
            {
                g.setColour(colour);
                mGlyphArrangement.draw(g);
            }

        private:
            auto static constexpr invalid_value = std::numeric_limits<float>::max();
            auto static constexpr epsilon = std::numeric_limits<float>::epsilon();

            struct LabelInfo
            {
                LabelInfo(float o)
                : offset(o)
                {
                }

                float const offset;
                juce::String text;
                float x;
                float y;
                float w;

                inline bool operator==(LabelInfo const& rhs) const
                {
                    return text == rhs.text && std::abs(x - rhs.x) < epsilon && std::abs(y - rhs.y) < epsilon;
                }
            };

            juce::Font const mFont;
            juce::String const mUnit;
            int const mNumDecimal;
            float const mCharWidth{juce::GlyphArrangement::getStringWidth(mFont, "0")};
            float const mUnitWidth{juce::GlyphArrangement::getStringWidth(mFont, mUnit)};
            LabelInfo mLowInfo{2.0f + mFont.getAscent()};
            LabelInfo mHighInfo{2.0f - mFont.getDescent()};
            float mLastValue = invalid_value;
            juce::GlyphArrangement mGlyphArrangement;
        };

        static constexpr int outsideGridHeight = 16;
        static constexpr int outsideGridWidth = 72;

        juce::String getStringForTime(Zoom::Accessor const& timeZoomAcsr, double time);
        std::function<juce::String(double)> getGridStringify(Accessor const& accessor);
        void paintInternalGrid(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, juce::Graphics& g, juce::Rectangle<int> bounds, std::vector<bool> const& channels, juce::Colour const colour, bool showGridLabels);
        void paintExternalGrid(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, juce::Graphics& g, juce::Rectangle<int> bounds, std::vector<bool> const& channels, juce::Colour const colour, Zoom::Grid::Justification outsideGridjustification);

        void paintMarkers(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
        void paintMarkers(juce::Graphics& g, juce::Rectangle<int> const& bounds, std::vector<Result::Data::Marker> const& results, std::vector<std::optional<float>> const& thresholds, juce::Range<double> const& timeRange, juce::Range<double> const& ignoredTimeRange, ColourSet const& colours, LabelLayout const& labelLayout, float lineWidth, juce::String const& unit);
        void paintPoints(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
        void paintPoints(juce::Graphics& g, juce::Rectangle<int> const& bounds, std::vector<Result::Data::Point> const& results, std::vector<std::optional<float>> const& thresholds, std::vector<Result::Data::Point> const& extra, juce::Range<double> const& timeRange, juce::Range<double> const& valueRange, std::function<float(float)> scaleValue, ColourSet const& colours, float lineWidth, juce::String const& unit);
        void paintColumns(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
    } // namespace Renderer
} // namespace Track

void Track::Renderer::paintChannels(juce::Graphics& g, juce::Rectangle<int> const& bounds, std::vector<bool> const& channels, juce::Colour const& separatorColour, std::function<void(juce::Rectangle<int>, size_t channel)> fn)
{
    auto const verticalRanges = Tools::getChannelVerticalRanges(bounds, channels);
    for(auto const& verticalRange : verticalRanges)
    {
        juce::Graphics::ScopedSaveState sss(g);
        auto const top = verticalRange.second.getStart();
        auto const bottom = verticalRange.second.getEnd();
        auto const region = bounds.withTop(top).withBottom(bottom);
        if(bottom < bounds.getHeight())
        {
            g.setColour(separatorColour);
            g.fillRect(bounds.withTop(bottom).withBottom(bottom + 1));
        }
        g.reduceClipRegion(region);
        if(fn != nullptr)
        {
            fn(region, verticalRange.first);
        }
    }
}

void Track::Renderer::paintClippedImage(juce::Graphics& g, juce::Image const& image, juce::Rectangle<float> const& bounds)
{
    auto const graphicsBounds = g.getClipBounds().toFloat();
    auto const deltaX = -bounds.getX();
    auto const deltaY = -bounds.getY();
    auto const scaleX = graphicsBounds.getWidth() / bounds.getWidth();
    auto const scaleY = graphicsBounds.getHeight() / bounds.getHeight();

    g.drawImageTransformed(image, juce::AffineTransform::translation(deltaX, deltaY).scaled(scaleX, scaleY).translated(graphicsBounds.getX(), graphicsBounds.getY()));
}

juce::String Track::Renderer::getStringForTime(Zoom::Accessor const& timeZoomAcsr, double time)
{
    auto const endTime = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>().getEnd();
    auto const hours = endTime > 3600.0 ? ":" : "";
    auto const minutes = endTime > 60.0 ? ":" : "";
    auto const seconds = endTime > 1.0 ? (endTime > 60.0 ? ":" : "s") : "";
    auto const milliseconds = endTime > 0.001 ? (endTime > 60.0 ? "" : "ms") : "";
    return Format::secondsToString(time, {hours, minutes, seconds, milliseconds});
}

std::function<juce::String(double)> Track::Renderer::getGridStringify(Accessor const& accessor)
{
    auto const frameType = Tools::getFrameType(accessor);
    if(!frameType.has_value())
    {
        return nullptr;
    }
    switch(frameType.value())
    {
        case Track::FrameType::label:
        {
            return nullptr;
        }
        case Track::FrameType::value:
        {
            auto const unit = Tools::getUnit(accessor);
            auto const isLog = accessor.getAttr<AttrType::zoomLogScale>() && Tools::hasVerticalZoomInHertz(accessor);
            if(isLog)
            {
                auto const nyquist = accessor.getAttr<AttrType::sampleRate>() / 2.0;
                auto const scaleRatio = static_cast<float>(std::max(Tools::getMidiFromHertz(nyquist), 1.0) / nyquist);
                return [scaleRatio, unit](double value)
                {
                    return Format::valueToString(std::round(Tools::getHertzFromMidi(value * scaleRatio)), 0) + unit;
                };
            }
            else
            {
                return [unit](double value)
                {
                    return Format::valueToString(value, 4) + unit;
                };
            }
        }
        case Track::FrameType::vector:
        {
            auto const isLog = accessor.getAttr<AttrType::zoomLogScale>() && Tools::hasVerticalZoomInHertz(accessor);
            if(isLog)
            {
                auto const numBins = accessor.getAcsr<AcsrType::binZoom>().getAttr<Zoom::AttrType::globalRange>().getEnd();
                auto const nyquist = accessor.getAttr<AttrType::sampleRate>() / 2.0;
                auto const midiMax = std::max(Tools::getMidiFromHertz(nyquist), 1.0);
                return [&, numBins, nyquist, midiMax](double value)
                {
                    auto const startMidi = Tools::getHertzFromMidi(value / numBins * midiMax);
                    auto const index = static_cast<size_t>(std::floor(startMidi / nyquist * numBins));
                    auto const name = Tools::getBinName(accessor, index);
                    return juce::String(index) + (name.isNotEmpty() ? juce::String(" - ") + name : "");
                };
            }
            else
            {
                return [&](double value)
                {
                    auto const index = static_cast<size_t>(std::floor(value));
                    auto const name = Tools::getBinName(accessor, index);
                    return juce::String(index) + (name.isNotEmpty() ? juce::String(" - ") + name : "");
                };
            }
        }
    }
    return nullptr;
}

void Track::Renderer::paintInternalGrid(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, juce::Graphics& g, juce::Rectangle<int> bounds, std::vector<bool> const& channels, juce::Colour const colour, bool showGridLabels)
{
    if(colour.isTransparent() || accessor.getAttr<AttrType::grid>() == GridMode::hidden)
    {
        return;
    }
    using Justification = Zoom::Grid::Justification;
    auto const justificationHorizontal = accessor.getAttr<AttrType::grid>() == GridMode::partial ? Justification(Justification::left | Justification::right) : Justification(Justification::horizontal);
    auto const justificationVertical = accessor.getAttr<AttrType::grid>() == GridMode::partial ? Justification(Justification::top | Justification::bottom) : Justification(Justification::vertical);

    auto const frameType = Tools::getFrameType(accessor);
    if(frameType.has_value())
    {
        auto const stringify = showGridLabels ? getGridStringify(accessor) : nullptr;
        auto const paintChannel = [&](Zoom::Accessor const& zoomAcsr, juce::Rectangle<int> const& region)
        {
            g.setColour(colour);
            Zoom::Grid::paintVertical(g, zoomAcsr.getAcsr<Zoom::AcsrType::grid>(), zoomAcsr.getAttr<Zoom::AttrType::visibleRange>(), region, stringify, justificationHorizontal);
        };

        switch(frameType.value())
        {
            case Track::FrameType::label:
            {
                paintChannels(g, bounds, channels, colour, [&](juce::Rectangle<int>, size_t)
                              {
                              });
            }
            break;
            case Track::FrameType::value:
            {
                paintChannels(g, bounds, channels, colour, [&](juce::Rectangle<int> region, size_t)
                              {
                                  paintChannel(accessor.getAcsr<AcsrType::valueZoom>(), region);
                              });
            }
            break;
            case Track::FrameType::vector:
            {
                paintChannels(g, bounds, channels, colour, [&](juce::Rectangle<int> region, size_t)
                              {
                                  paintChannel(accessor.getAcsr<AcsrType::binZoom>(), region);
                              });
            }
            break;
        }
    }
    g.setColour(colour);
    Zoom::Grid::paintHorizontal(g, timeZoomAccessor.getAcsr<Zoom::AcsrType::grid>(), timeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>(), bounds, nullptr, outsideGridWidth, justificationVertical);
}

void Track::Renderer::paintExternalGrid(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, juce::Graphics& g, juce::Rectangle<int> bounds, std::vector<bool> const& channels, juce::Colour const colour, Zoom::Grid::Justification outsideGridjustification)
{
    if(colour.isTransparent())
    {
        return;
    }
    g.setColour(colour);
    using Justification = Zoom::Grid::Justification;
    auto const frameType = Tools::getFrameType(accessor);
    if(frameType.has_value() && (outsideGridjustification.testFlags(Justification::left) || outsideGridjustification.testFlags(Justification::right)))
    {
        auto const stringify = getGridStringify(accessor);
        auto const paintChannel = [&](Zoom::Accessor const& zoomAcsr, juce::Rectangle<int> region)
        {
            if(outsideGridjustification.testFlags(Justification::left))
            {
                Zoom::Grid::paintVertical(g, zoomAcsr.getAcsr<Zoom::AcsrType::grid>(), zoomAcsr.getAttr<Zoom::AttrType::visibleRange>(), region.removeFromLeft(outsideGridWidth), stringify, Justification::right);
            }
            if(outsideGridjustification.testFlags(Justification::right))
            {
                Zoom::Grid::paintVertical(g, zoomAcsr.getAcsr<Zoom::AcsrType::grid>(), zoomAcsr.getAttr<Zoom::AttrType::visibleRange>(), region.removeFromRight(outsideGridWidth), stringify, Justification::left);
            }
        };
        auto const verticalBounds = bounds.withTrimmedTop(outsideGridjustification.testFlags(Justification::top) ? outsideGridHeight : 0).withTrimmedBottom(outsideGridjustification.testFlags(Justification::bottom) ? outsideGridHeight : 0);
        switch(frameType.value())
        {
            case Track::FrameType::label:
            {
                paintChannels(g, verticalBounds, channels, colour, [&](juce::Rectangle<int>, size_t)
                              {
                              });
            }
            break;
            case Track::FrameType::value:
            {
                paintChannels(g, verticalBounds, channels, colour, [&](juce::Rectangle<int> region, size_t)
                              {
                                  paintChannel(accessor.getAcsr<AcsrType::valueZoom>(), region);
                              });
            }
            break;
            case Track::FrameType::vector:
            {
                paintChannels(g, verticalBounds, channels, colour, [&](juce::Rectangle<int> region, size_t)
                              {
                                  paintChannel(accessor.getAcsr<AcsrType::binZoom>(), region);
                              });
            }
            break;
        }
    }

    auto horizontalBounds = bounds.withTrimmedLeft(outsideGridjustification.testFlags(Justification::left) ? outsideGridWidth : 0).withTrimmedRight(outsideGridjustification.testFlags(Justification::right) ? outsideGridWidth : 0);
    if(outsideGridjustification.testFlags(Justification::top))
    {
        Zoom::Grid::paintHorizontal(g, timeZoomAccessor.getAcsr<Zoom::AcsrType::grid>(), timeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>(), horizontalBounds.removeFromTop(outsideGridHeight).reduced(0, 1), [&](double time)
                                    {
                                        return getStringForTime(timeZoomAccessor, time);
                                    },
                                    outsideGridWidth, Justification::bottom);
    }
    if(outsideGridjustification.testFlags(Justification::bottom))
    {
        Zoom::Grid::paintHorizontal(g, timeZoomAccessor.getAcsr<Zoom::AcsrType::grid>(), timeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>(), horizontalBounds.removeFromBottom(outsideGridHeight).reduced(0, 1), [&](double time)
                                    {
                                        return getStringForTime(timeZoomAccessor, time);
                                    },
                                    outsideGridWidth, Justification::top);
    }
}

juce::BorderSize<int> Track::Renderer::getOutsideGridBorder(Zoom::Grid::Justification outsideGridjustification)
{
    using Justification = Zoom::Grid::Justification;
    return {
        outsideGridjustification.testFlags(Justification::top) ? outsideGridHeight : 0,
        outsideGridjustification.testFlags(Justification::left) ? outsideGridWidth : 0,
        outsideGridjustification.testFlags(Justification::bottom) ? outsideGridHeight : 0,
        outsideGridjustification.testFlags(Justification::right) ? outsideGridWidth : 0,
    };
}

void Track::Renderer::paint(Accessor const& accessor, Zoom::Accessor const& timeZoomAcsr, juce::Graphics& g, juce::Rectangle<int> const& bounds, std::vector<bool> const& channels, juce::Colour const colour, Zoom::Grid::Justification outsideGridjustification)
{
    using Justification = Zoom::Grid::Justification;
    auto const outsideGridBorder = getOutsideGridBorder(outsideGridjustification);
    auto const internalBounds = outsideGridBorder.subtractedFrom(bounds);
    auto const frameType = Tools::getFrameType(accessor);
    if(!frameType.has_value())
    {
        return;
    }

    if(!internalBounds.isEmpty())
    {
        auto const showGridLabels = outsideGridjustification == Justification::none;
        juce::Graphics::ScopedSaveState sss(g);
        g.reduceClipRegion(internalBounds);
        switch(frameType.value())
        {
            case Track::FrameType::label:
            {
                paintInternalGrid(accessor, timeZoomAcsr, g, internalBounds, channels, colour, showGridLabels);
                paintChannels(g, internalBounds, channels, colour, [&](juce::Rectangle<int> region, size_t channel)
                              {
                                  g.setFont(accessor.getAttr<AttrType::graphicsSettings>().font);
                                  paintMarkers(accessor, channel, g, region, timeZoomAcsr);
                              });
            }
            break;
            case Track::FrameType::value:
            {
                paintInternalGrid(accessor, timeZoomAcsr, g, internalBounds, channels, colour, showGridLabels);
                paintChannels(g, internalBounds, channels, colour, [&](juce::Rectangle<int> region, size_t channel)
                              {
                                  g.setFont(accessor.getAttr<AttrType::graphicsSettings>().font);
                                  paintPoints(accessor, channel, g, region, timeZoomAcsr);
                              });
            }
            break;
            case Track::FrameType::vector:
            {
                paintChannels(g, internalBounds, channels, colour, [&](juce::Rectangle<int> region, size_t channel)
                              {
                                  paintColumns(accessor, channel, g, region, timeZoomAcsr);
                              });
                paintInternalGrid(accessor, timeZoomAcsr, g, internalBounds, channels, colour, showGridLabels);
            }
            break;
        }
    }

    if(!outsideGridBorder.isEmpty())
    {
        g.setColour(colour);
        g.drawRect(internalBounds.expanded(1), 1);
        paintExternalGrid(accessor, timeZoomAcsr, g, bounds, channels, colour, outsideGridjustification);
    }
}

void Track::Renderer::paintMarkers(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr)
{
    auto const& timeRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const& colours = accessor.getAttr<AttrType::graphicsSettings>().colours;
    auto const& unit = Tools::getUnit(accessor);
    auto const& labelLayout = accessor.getAttr<AttrType::graphicsSettings>().labelLayout;

    auto const& thesholds = accessor.getAttr<AttrType::extraThresholds>();
    auto const& edition = accessor.getAttr<AttrType::edit>();
    auto const lineWidth = accessor.getAttr<AttrType::graphicsSettings>().lineWidth;
    if(edition.channel == channel)
    {
        auto* data = std::get_if<std::vector<Result::Data::Marker>>(&edition.data);
        if(data != nullptr && !data->empty())
        {
            paintMarkers(g, bounds, *data, thesholds, timeRange, {}, colours, labelLayout, lineWidth, unit);
        }
    }

    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return;
    }
    auto const markers = results.getMarkers();
    if(markers != nullptr && markers->size() > channel)
    {
        paintMarkers(g, bounds, markers->at(channel), thesholds, timeRange, edition.range, colours, labelLayout, lineWidth, unit);
    }
}

void Track::Renderer::paintMarkers(juce::Graphics& g, juce::Rectangle<int> const& bounds, std::vector<Result::Data::Marker> const& results, std::vector<std::optional<float>> const& thresholds, juce::Range<double> const& timeRange, juce::Range<double> const& ignoredTimeRange, ColourSet const& colours, LabelLayout const& labelLayout, float lineWidth, juce::String const& unit)
{
    auto const clipBounds = g.getClipBounds().toFloat();
    if(bounds.isEmpty() || clipBounds.isEmpty() || results.empty() || timeRange.isEmpty())
    {
        return;
    }

    auto const fbounds = bounds.toFloat();
    auto constexpr epsilonPixel = 1.0f;

    auto const clipTimeStart = Tools::pixelToSeconds(clipBounds.getX() - epsilonPixel, timeRange, fbounds);
    auto const clipTimeEnd = Tools::pixelToSeconds(clipBounds.getRight() + epsilonPixel, timeRange, fbounds);

    juce::RectangleList<float> ticks;
    juce::RectangleList<float> durations;
    std::vector<std::tuple<juce::String, int, int>> labels;
    auto const showLabel = !colours.text.isTransparent();
    auto const showDuration = !colours.duration.isTransparent();
    auto const font = g.getCurrentFont();
    auto const minTextWidth = juce::GlyphArrangement::getStringWidthInt(font, "...") + 2;

    auto const y = fbounds.getY();
    auto const height = fbounds.getHeight();

    auto it = std::prev(std::lower_bound(std::next(results.cbegin()), results.cend(), clipTimeStart, Result::lower_cmp<Results::Marker>));
    auto expectedEnd = std::upper_bound(it, results.cend(), clipTimeEnd, Result::upper_cmp<Results::Marker>);
    auto const numElements = std::distance(it, expectedEnd);
    ticks.ensureStorageAllocated(static_cast<int>(numElements) + 1);
    durations.ensureStorageAllocated(static_cast<int>(numElements) + 1);

    juce::Rectangle<float> currentTick;
    juce::Rectangle<float> currentDuration;
    while(it != results.cend() && std::get<0>(*it) < clipTimeEnd)
    {
        auto const position = std::get<0>(*it);
        auto const process = !ignoredTimeRange.contains(position) && Result::passThresholds(*it, thresholds);
        if(process)
        {
            auto const duration = std::get<1>(*it);
            auto const x = Tools::secondsToPixel(position, timeRange, fbounds);
            auto const w = Tools::secondsToPixel(position + duration, timeRange, fbounds) - x;

            if(!currentTick.isEmpty() && currentTick.getRight() >= x)
            {
                currentTick = currentTick.getUnion({x, y, lineWidth, height});
            }
            else
            {
                if(!currentTick.isEmpty())
                {
                    ticks.addWithoutMerging(currentTick);
                }
                currentTick = {x, y, lineWidth, height};
            }

            if(showDuration)
            {
                if(w >= 1.0f && !currentDuration.isEmpty() && currentDuration.getRight() >= x)
                {
                    currentDuration = currentDuration.getUnion({x, y, w + lineWidth, height});
                }
                else
                {
                    if(!currentDuration.isEmpty())
                    {
                        durations.addWithoutMerging(currentDuration);
                    }
                    if(w >= 1.0f)
                    {
                        currentDuration = {x, y, w + lineWidth, height};
                    }
                    else
                    {
                        currentDuration = {};
                    }
                }
            }

            if(showLabel && !std::get<2>(*it).empty())
            {
                auto const text = juce::String(std::get<2>(*it)) + unit;
                auto const textX = static_cast<int>(std::round(x)) + 2;
                auto const textWidth = juce::GlyphArrangement::getStringWidthInt(font, text);
                if(labels.empty())
                {
                    labels.push_back(std::make_tuple(text, textX, textWidth));
                }
                else
                {
                    auto const previousTextX = std::get<1>(labels.back());
                    auto const previousTextWidth = std::get<2>(labels.back());
                    if((previousTextX + std::min(previousTextWidth, minTextWidth)) < textX)
                    {
                        auto const maxPreviousTextWidth = textX - previousTextX - 2;
                        std::get<2>(labels.back()) = std::min(previousTextWidth, maxPreviousTextWidth);
                        labels.push_back(std::make_tuple(text, textX, textWidth));
                    }
                    else
                    {
                        // Marker is too close so we remove the previous one
                        labels.back() = std::make_tuple(text, textX, textWidth);
                    }
                }
            }
        }

        it = std::next(it);
    }

    if(!currentTick.isEmpty())
    {
        ticks.addWithoutMerging(currentTick);
    }
    if(!currentDuration.isEmpty())
    {
        durations.addWithoutMerging(currentDuration);
    }

    if(showDuration)
    {
        g.setColour(colours.duration);
        g.fillRectList(durations);
    }

    if(!colours.shadow.isTransparent())
    {
        ticks.offsetAll(-2.0f, 0.0f);

        g.setColour(colours.shadow.withMultipliedAlpha(0.5f));
        g.fillRectList(ticks);
        ticks.offsetAll(1.0f, 0.0f);

        g.setColour(colours.shadow.withMultipliedAlpha(0.75f));
        g.fillRectList(ticks);
        ticks.offsetAll(1.0f, 0.0f);
    }
    g.setColour(colours.foreground);
    g.fillRectList(ticks);

    if(showLabel)
    {
        auto const fontHeight = static_cast<int>(std::ceil(font.getHeight()));
        auto const getPositionY = [&]()
        {
            switch(labelLayout.justification)
            {
                case LabelLayout::Justification::top:
                    return bounds.getY() + static_cast<int>(std::ceil(labelLayout.position));
                case LabelLayout::Justification::centred:
                    return bounds.getCentreY() + static_cast<int>(std::ceil(labelLayout.position - font.getAscent()));
                case LabelLayout::Justification::bottom:
                    return bounds.getBottom() - static_cast<int>(std::ceil(font.getHeight() + labelLayout.position));
            }
            return bounds.getY() + static_cast<int>(std::ceil(labelLayout.position));
        };
        auto const position = getPositionY();
        g.setColour(colours.text);
        for(auto const& label : labels)
        {
            g.drawText(std::get<0>(label), std::get<1>(label), position, std::get<2>(label), fontHeight, juce::Justification::left, true);
        }
    }
}

void Track::Renderer::paintPoints(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr)
{
    auto const& timeRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const& valueZoomAcsr = accessor.getAcsr<AcsrType::valueZoom>();
    auto const& valueRange = valueZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const& colours = accessor.getAttr<AttrType::graphicsSettings>().colours;
    auto const& unit = Tools::getUnit(accessor);
    auto const lineWidth = accessor.getAttr<AttrType::graphicsSettings>().lineWidth;
    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return;
    }

    auto const isLog = accessor.getAttr<AttrType::zoomLogScale>() && Tools::hasVerticalZoomInHertz(accessor);
    auto const nyquist = accessor.getAttr<AttrType::sampleRate>() / 2.0;
    auto const scaleRatio = static_cast<float>(nyquist / std::max(Tools::getMidiFromHertz(nyquist), 1.0));
    auto const scaleFn = isLog ? std::function<float(float)>([=](float v)
                                                             {
                                                                 return Tools::getMidiFromHertz(v) * scaleRatio;
                                                             })
                               : std::function<float(float)>([](float v)
                                                             {
                                                                 return v;
                                                             });
    auto const& thesholds = accessor.getAttr<AttrType::extraThresholds>();
    auto const markers = results.getPoints();
    if(markers != nullptr && markers->size() > channel)
    {
        auto const& edition = accessor.getAttr<AttrType::edit>();
        if(edition.channel == channel)
        {
            auto* data = std::get_if<std::vector<Result::Data::Point>>(&edition.data);
            if(data != nullptr && !data->empty())
            {
                paintPoints(g, bounds, markers->at(channel), thesholds, *data, timeRange, valueRange, scaleFn, colours, lineWidth, unit);
                return;
            }
        }
        paintPoints(g, bounds, markers->at(channel), thesholds, {}, timeRange, valueRange, scaleFn, colours, lineWidth, unit);
    }
}

void Track::Renderer::paintPoints(juce::Graphics& g, juce::Rectangle<int> const& bounds, std::vector<Result::Data::Point> const& results, std::vector<std::optional<float>> const& thresholds, std::vector<Result::Data::Point> const& extra, juce::Range<double> const& timeRange, juce::Range<double> const& valueRange, std::function<float(float)> scaleValue, ColourSet const& colours, float lineWidth, juce::String const& unit)
{
    auto const clipBounds = g.getClipBounds().toFloat();
    if(bounds.isEmpty() || clipBounds.isEmpty() || results.empty() || timeRange.isEmpty() || valueRange.isEmpty())
    {
        return;
    }

    auto const fbounds = bounds.toFloat();
    auto const font = g.getCurrentFont();

    auto const epsilonPixel = 1.0f / juce::Desktop::getInstance().getGlobalScaleFactor();
    auto const clipTimeStart = Tools::pixelToSeconds(static_cast<float>(clipBounds.getX()) - epsilonPixel, timeRange, fbounds);
    auto const clipTimeEnd = Tools::pixelToSeconds(static_cast<float>(clipBounds.getRight()) + epsilonPixel, timeRange, fbounds);

    auto const itShowsValues = [&](auto const& it)
    {
        return std::get<2_z>(*it).has_value() && Result::passThresholds(*it, thresholds);
    };

    auto const rangeLength = valueRange.getLength();
    auto const getNumDecimals = [&]()
    {
        int numDecimals = 0;
        while(numDecimals < 4 && std::pow(10.0, static_cast<double>(numDecimals)) / rangeLength < 1.0)
        {
            ++numDecimals;
        }
        return 4 - numDecimals;
    };
    auto const numDecimals = getNumDecimals();

    if(results.size() == 1_z && itShowsValues(results.cbegin()))
    {
        auto const value = std::get<2_z>(results.at(0)).value();
        auto const y = Tools::valueToPixel(scaleValue(value), valueRange, fbounds);
        auto const fullLine = std::get<0_z>(results.at(0)) + std::get<1_z>(results.at(0)) < std::numeric_limits<double>::epsilon() * 2.0;
        auto const x1 = fullLine ? fbounds.getX() : Tools::secondsToPixel(std::get<0_z>(results.at(0)), timeRange, fbounds);
        auto const x2 = fullLine ? fbounds.getRight() : Tools::secondsToPixel(std::get<0_z>(results.at(0)) + std::get<1_z>(results.at(0)), timeRange, fbounds);
        if(!colours.shadow.isTransparent())
        {
            g.setColour(colours.shadow.withMultipliedAlpha(0.5f));
            g.drawLine(x1, y + 2.f, x2, y + 2.f, lineWidth);
            g.setColour(colours.shadow.withMultipliedAlpha(0.75f));
            g.drawLine(x1, y + 1.f, x2, y + 1.f, lineWidth);
        }
        g.setColour(colours.foreground);
        g.drawLine(x1, y, x2, y, lineWidth);

        if(!colours.text.isTransparent())
        {
            g.setColour(colours.text);
            g.drawSingleLineText(Format::valueToString(value, numDecimals) + unit, static_cast<int>(x1) + 4, static_cast<int>(y - font.getDescent()) - 2, juce::Justification::left);
        }
        return;
    }

    struct it_wrap
    {
        using const_iterator = std::vector<Result::Data::Point>::const_iterator;
        using const_reference = std::vector<Result::Data::Point>::const_reference;
        const_iterator it;
        bool extra{false};

        it_wrap() noexcept
        : it()
        , extra(false)
        {
        }

        it_wrap(const_iterator i, bool e) noexcept
        : it(i)
        , extra(e)
        {
        }

        bool operator==(const_iterator other) const { return it == other; }
        bool operator!=(const_iterator other) const { return !(*this == other); }
        bool operator==(it_wrap other) const { return it == other.it; }
        bool operator!=(it_wrap other) const { return !(*this == other); }
        const_reference operator*() const { return *it; }
    };

    auto const hasExtra = !extra.empty();
    auto const startExtraTime = hasExtra ? std::get<0_z>(extra.front()) : -1.0;
    auto const endExtraTime = hasExtra ? std::get<0_z>(extra.back()) : -1.0;

    it_wrap it = {std::prev(std::lower_bound(std::next(results.cbegin()), results.cend(), clipTimeStart, Result::lower_cmp<Results::Point>)), false};
    if(hasExtra && std::get<0_z>(*it) > std::get<0_z>(extra.front()))
    {
        it = {extra.cbegin(), true};
    }
    auto const sourceOutIt = std::lower_bound(results.cbegin(), results.cend(), startExtraTime, Result::lower_cmp<Results::Point>);
    auto const sourceInIt = std::upper_bound(sourceOutIt, results.cend(), endExtraTime, Result::upper_cmp<Results::Point>);

    // Zoom distances corresponding to epsilon pixels
    auto const timeEpsilon = static_cast<double>(epsilonPixel) * timeRange.getLength() / static_cast<double>(bounds.getWidth());
    auto const valueEpsilon = static_cast<double>(epsilonPixel) * valueRange.getLength() / static_cast<double>(bounds.getHeight());

    auto const getNextIt = [&](it_wrap const iterator) -> it_wrap
    {
        auto const next = std::next(iterator.it);
        if(!hasExtra)
        {
            return {next, false};
        }
        MiscWeakAssert(iterator != extra.cend());
        if(!iterator.extra)
        {
            if(next == sourceOutIt)
            {
                return {extra.cbegin(), true};
            }
            MiscWeakAssert(next != extra.cend());
            return {next, false};
        }
        if(next == extra.cend())
        {
            return {sourceInIt, false};
        }
        MiscWeakAssert(next != extra.cend());
        return {next, true};
    };

    auto const getEndTime = [&](it_wrap const iterator)
    {
        auto const start = std::get<0_z>(*iterator.it);
        auto const end = start + std::get<1_z>(*iterator.it);
        if(!hasExtra)
        {
            return end;
        }
        if(start < startExtraTime)
        {
            return std::min(end, startExtraTime);
        }
        if(start < endExtraTime)
        {
            return std::min(end, endExtraTime);
        }
        return end;
    };

    auto const getNextItBeforeTime = [&](it_wrap const start)
    {
        auto const limit = getEndTime(start) + timeEpsilon;
        auto min = std::get<2_z>(*start).value();
        auto max = min;
        auto previous = start;
        auto iterator = getNextIt(previous);
        MiscWeakAssert(!hasExtra || iterator != extra.cend());
        while(iterator != results.cend() && std::get<0_z>(*iterator) < limit && itShowsValues(iterator))
        {
            auto const& lvalue = std::get<2_z>(*iterator).value();
            min = std::min(min, lvalue);
            max = std::max(max, lvalue);
            previous = iterator;
            iterator = getNextIt(iterator);
        }
        MiscWeakAssert(!hasExtra || iterator != extra.cend());
        MiscWeakAssert(!hasExtra || previous != extra.cend());
        MiscWeakAssert(!hasExtra || previous != results.cend());
        return std::make_tuple(previous, min, max);
    };

    juce::RectangleList<float> rectangles;
    PathArrangement pathArr;
    LabelArrangement labelArr(font, unit, numDecimals);
    auto const showLabel = !colours.text.isTransparent();
    auto const rectExtent = std::max((lineWidth - 1) / 4.0f, 0.0f);
    auto hasExceededEnd = false;
    while(!hasExceededEnd && it != results.cend())
    {
        MiscWeakAssert(!hasExtra || it != extra.cend());
        if(!itShowsValues(it))
        {
            pathArr.stopLine();
            it = getNextIt(it);
            while(it != results.cend() && !hasExceededEnd && !itShowsValues(it))
            {
                hasExceededEnd = std::get<0_z>(*it) >= clipTimeEnd;
                it = getNextIt(it);
            }
        }
        else if(std::get<1_z>(*it) < timeEpsilon)
        {
            auto const nextResult = getNextItBeforeTime(it);
            auto const start = std::get<0_z>(*it);
            auto const next = std::get<0_z>(nextResult);
            auto const end = getEndTime(next);

            auto const min = std::get<1_z>(nextResult);
            auto const max = std::get<2_z>(nextResult);

            auto const x = Tools::secondsToPixel(start, timeRange, fbounds);
            auto const x2 = Tools::secondsToPixel(end, timeRange, fbounds);

            if(it != next && (max - min) >= valueEpsilon)
            {
                auto const y1 = Tools::valueToPixel(scaleValue(max), valueRange, fbounds);
                auto const y2 = Tools::valueToPixel(scaleValue(min), valueRange, fbounds);
                if(pathArr.isLineStarted())
                {
                    pathArr.addLine(x, x, y1);
                }
                pathArr.stopLine();

                rectangles.addWithoutMerging(juce::Rectangle<float>(x, y1, x2 - x, y2 - y1).expanded(rectExtent));
                if(showLabel)
                {
                    labelArr.addValue(min, x, y1);
                    labelArr.addValue(max, x, y2);
                }

                it = next;
                hasExceededEnd = end >= clipTimeEnd;
            }
            else
            {
                auto const value = std::get<2_z>(*it).value();
                auto const y = Tools::valueToPixel(scaleValue(value), valueRange, fbounds);
                if(showLabel)
                {
                    labelArr.addValue(value, x, y);
                }
                pathArr.addLine(x, x2, y);

                it = it != next ? next : getNextIt(it);
                hasExceededEnd = end >= clipTimeEnd;
            }
        }
        else
        {
            auto const value = std::get<2_z>(*it).value();
            auto const y = Tools::valueToPixel(scaleValue(value), valueRange, fbounds);
            auto const start = std::get<0_z>(*it);
            auto const x = Tools::secondsToPixel(start, timeRange, fbounds);
            auto const end = getEndTime(it);
            auto const x2 = Tools::secondsToPixel(end, timeRange, fbounds);
            if(showLabel)
            {
                labelArr.addValue(value, x, y);
            }
            pathArr.addLine(x, x2, y);

            it = getNextIt(it);
            hasExceededEnd = end >= clipTimeEnd;
        }
    }

    if(!rectangles.isEmpty())
    {
        g.setColour(colours.foreground);
        g.fillRectList(rectangles);
    }

    pathArr.draw(g, colours.foreground, colours.shadow, lineWidth);
    if(showLabel)
    {
        labelArr.draw(g, colours.text);
    }
}

void Track::Renderer::paintColumns(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr)
{
    auto const graph = accessor.getAttr<AttrType::graphics>();
    if(graph.empty(channel))
    {
        return;
    }
    auto const clipBounds = g.getClipBounds().constrainedWithin(bounds);
    if(bounds.isEmpty() || clipBounds.isEmpty())
    {
        return;
    }

    auto const& binZoomAcsr = accessor.getAcsr<AcsrType::binZoom>();
    if(timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>().isEmpty() || timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>().isEmpty())
    {
        return;
    }
    if(binZoomAcsr.getAttr<Zoom::AttrType::globalRange>().isEmpty() || binZoomAcsr.getAttr<Zoom::AttrType::visibleRange>().isEmpty())
    {
        return;
    }

    auto const renderImage = [&](juce::Image const& image, Zoom::Accessor const& xZoomAcsr, Zoom::Accessor const& yZoomAcsr)
    {
        if(!image.isValid())
        {
            return;
        }

        auto const scale = [](auto const& range, auto const& source, auto const& destination)
        {
            if(static_cast<double>(source.getLength()) <= 0.0)
            {
                return Zoom::Range(destination.getStart(), destination.getStart());
            }
            auto const ratio = static_cast<double>(destination.getLength()) / static_cast<double>(source.getLength());
            auto const start = (static_cast<double>(range.getStart()) - static_cast<double>(source.getStart())) * ratio + static_cast<double>(destination.getStart());
            auto const end = (static_cast<double>(range.getEnd()) - static_cast<double>(source.getStart())) * ratio + static_cast<double>(destination.getStart());
            return Zoom::Range(start, end);
        };

        auto const reverse = [](Zoom::Range const& globalRange, Zoom::Range const& visibleRange)
        {
            return Zoom::Range{globalRange.getEnd() - visibleRange.getEnd() + globalRange.getStart(), globalRange.getEnd() - visibleRange.getStart() + globalRange.getStart()};
        };

        auto const xClippedRange = scale(clipBounds.getHorizontalRange(), bounds.getHorizontalRange(), xZoomAcsr.getAttr<Zoom::AttrType::visibleRange>());
        auto const xRange = scale(xClippedRange, graph.channels.at(channel).timeRange, juce::Range<int>{0, image.getWidth()});

        auto const yClippedRange = scale(clipBounds.getVerticalRange(), bounds.getVerticalRange(), reverse(yZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), yZoomAcsr.getAttr<Zoom::AttrType::visibleRange>()));
        auto const yRange = scale(yClippedRange, yZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), juce::Range<int>{0, image.getHeight()});

        g.setColour(juce::Colours::black);
        juce::Graphics::ScopedSaveState sss(g);
        g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
        paintClippedImage(g, image, juce::Rectangle<double>{xRange.getStart(), yRange.getStart(), xRange.getLength(), yRange.getLength()}.toFloat());
    };

    auto const timeZoomRatio = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>().getLength() / timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>().getLength();
    auto const binZoomRatio = binZoomAcsr.getAttr<Zoom::AttrType::globalRange>().getLength() / binZoomAcsr.getAttr<Zoom::AttrType::visibleRange>().getLength();
    auto const boundsDimension = std::max(bounds.getWidth() * timeZoomRatio, bounds.getHeight() * binZoomRatio);
    for(auto const& image : graph.channels.at(channel).images)
    {
        auto const imageDimension = std::max(image.getWidth(), image.getHeight());
        if(imageDimension >= boundsDimension)
        {
            renderImage(image, timeZoomAcsr, binZoomAcsr);
            return;
        }
    }
    renderImage(graph.channels.at(channel).images.back(), timeZoomAcsr, binZoomAcsr);
}

ANALYSE_FILE_END
