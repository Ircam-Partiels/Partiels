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

            void draw(juce::Graphics& g, juce::Colour const& foreground, juce::Colour const& shadow)
            {
                stopLine();
                if(mPath.isEmpty())
                {
                    return;
                }

                juce::PathStrokeType const pathStrokeType(1.0f);
                pathStrokeType.createStrokedPath(mPath, mPath);

                if(!shadow.isTransparent())
                {
                    g.setColour(shadow.withMultipliedAlpha(0.5f));
                    g.fillPath(mPath, juce::AffineTransform::translation(0.0f, 2.0f));
                    g.setColour(shadow.withMultipliedAlpha(0.75f));
                    g.fillPath(mPath, juce::AffineTransform::translation(1.0f, 1.0f));
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
            float const mCharWidth{mFont.getStringWidthFloat("0")};
            float const mUnitWidth{mFont.getStringWidthFloat(mUnit)};
            LabelInfo mLowInfo{2.0f + mFont.getAscent()};
            LabelInfo mHighInfo{2.0f - mFont.getDescent()};
            float mLastValue = invalid_value;
            juce::GlyphArrangement mGlyphArrangement;
        };

        void paintGrid(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour const colour);
        void paintMarkers(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
        void paintMarkers(juce::Graphics& g, juce::Rectangle<int> const& bounds, std::vector<Result::Data::Marker> const& results, std::vector<std::optional<float>> const& thresholds, juce::Range<double> const& timeRange, juce::Range<double> const& ignoredTimeRange, ColourSet const& colours, juce::String const& unit);
        void paintPoints(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
        void paintPoints(juce::Graphics& g, juce::Rectangle<int> const& bounds, std::vector<Result::Data::Point> const& results, std::vector<std::optional<float>> const& thresholds, std::vector<Result::Data::Point> const& extra, juce::Range<double> const& timeRange, juce::Range<double> const& valueRange, ColourSet const& colours, juce::String const& unit);
        void paintColumns(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
    } // namespace Renderer
} // namespace Track

void Track::Renderer::paintChannels(Accessor const& acsr, juce::Graphics& g, juce::Rectangle<int> const& bounds, juce::Colour const& separatorColour, std::function<void(juce::Rectangle<int>, size_t channel)> fn)
{
    auto const verticalRanges = Tools::getChannelVerticalRanges(acsr, bounds);
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

void Track::Renderer::paintGrid(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour const colour)
{
    if(colour.isTransparent() || accessor.getAttr<AttrType::grid>() == GridMode::hidden)
    {
        return;
    }
    using Justification = Zoom::Grid::Justification;
    auto const justificationHorizontal = accessor.getAttr<AttrType::grid>() == GridMode::partial ? Justification(Zoom::Grid::Justification::left | Zoom::Grid::Justification::right) : Justification(Justification::horizontallyCentred);
    auto const justificationVertical = accessor.getAttr<AttrType::grid>() == GridMode::partial ? Justification(Zoom::Grid::Justification::top | Zoom::Grid::Justification::bottom) : Justification(Justification::verticallyCentred);

    auto const getStringify = [&]() -> std::function<juce::String(double)>
    {
        switch(Tools::getFrameType(accessor))
        {
            case Track::FrameType::label:
            {
                return nullptr;
            }
            case Track::FrameType::value:
            {
                return [unit = Tools::getUnit(accessor)](double value)
                {
                    return Format::valueToString(value, 4) + unit;
                };
            }
            case Track::FrameType::vector:
            {
                return [&](double value)
                {
                    auto const& output = accessor.getAttr<AttrType::description>().output;
                    auto const binIndex = value >= 0.0 ? static_cast<size_t>(std::round(value)) : 0_z;
                    if(binIndex >= output.binNames.size() || output.binNames[binIndex].empty())
                    {
                        return juce::String(binIndex);
                    }
                    return juce::String(output.binNames[binIndex]);
                };
            }
        }
        return nullptr;
    };

    auto const stringify = getStringify();
    auto const paintChannel = [&](Zoom::Accessor const& zoomAcsr, juce::Rectangle<int> const& region)
    {
        g.setColour(colour);
        Zoom::Grid::paintVertical(g, zoomAcsr.getAcsr<Zoom::AcsrType::grid>(), zoomAcsr.getAttr<Zoom::AttrType::visibleRange>(), region, stringify, justificationHorizontal);
    };

    switch(Tools::getFrameType(accessor))
    {
        case Track::FrameType::label:
        {
            paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int>, size_t)
                          {
                          });
        }
        break;
        case Track::FrameType::value:
        {
            paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int> region, size_t)
                          {
                              paintChannel(accessor.getAcsr<AcsrType::valueZoom>(), region);
                          });
        }
        break;
        case Track::FrameType::vector:
        {
            paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int> region, size_t)
                          {
                              paintChannel(accessor.getAcsr<AcsrType::binZoom>(), region);
                          });
        }
        break;
    }

    g.setColour(colour);
    Zoom::Grid::paintHorizontal(g, timeZoomAccessor.getAcsr<Zoom::AcsrType::grid>(), timeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>(), bounds, nullptr, 70, justificationVertical);
}

void Track::Renderer::paint(Accessor const& accessor, Zoom::Accessor const& timeZoomAcsr, juce::Graphics& g, juce::Rectangle<int> const& bounds, juce::Colour const colour)
{
    g.setFont(accessor.getAttr<AttrType::font>());
    switch(Tools::getFrameType(accessor))
    {
        case Track::FrameType::label:
        {
            paintGrid(accessor, timeZoomAcsr, g, bounds, colour);
            paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int> region, size_t channel)
                          {
                              paintMarkers(accessor, channel, g, region, timeZoomAcsr);
                          });
        }
        break;
        case Track::FrameType::value:
        {
            paintGrid(accessor, timeZoomAcsr, g, bounds, colour);
            paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int> region, size_t channel)
                          {
                              paintPoints(accessor, channel, g, region, timeZoomAcsr);
                          });
        }
        break;
        case Track::FrameType::vector:
        {
            paintChannels(accessor, g, bounds, colour, [&](juce::Rectangle<int> region, size_t channel)
                          {
                              paintColumns(accessor, channel, g, region, timeZoomAcsr);
                          });
            paintGrid(accessor, timeZoomAcsr, g, bounds, colour);
        }
        break;
    }
}

void Track::Renderer::paintMarkers(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr)
{
    auto const& timeRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const& colours = accessor.getAttr<AttrType::colours>();
    auto const& unit = Tools::getUnit(accessor);

    auto const& thesholds = accessor.getAttr<AttrType::extraThresholds>();
    auto const& edition = accessor.getAttr<AttrType::edit>();
    if(edition.channel == channel)
    {
        auto* data = std::get_if<std::vector<Result::Data::Marker>>(&edition.data);
        if(data != nullptr && !data->empty())
        {
            paintMarkers(g, bounds, *data, thesholds, timeRange, {}, colours, unit);
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
        paintMarkers(g, bounds, markers->at(channel), thesholds, timeRange, edition.range, colours, unit);
    }
}

void Track::Renderer::paintMarkers(juce::Graphics& g, juce::Rectangle<int> const& bounds, std::vector<Result::Data::Marker> const& results, std::vector<std::optional<float>> const& thresholds, juce::Range<double> const& timeRange, juce::Range<double> const& ignoredTimeRange, ColourSet const& colours, juce::String const& unit)
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
    auto const font = g.getCurrentFont();

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
                currentTick = currentTick.getUnion({x, y, 1.0f, height});
            }
            else
            {
                if(!currentTick.isEmpty())
                {
                    ticks.addWithoutMerging(currentTick);
                }
                currentTick = {x, y, 1.0f, height};
            }

            if(w >= 1.0f && !currentDuration.isEmpty() && currentDuration.getRight() >= x)
            {
                currentDuration = currentDuration.getUnion({x, y, w, height});
            }
            else
            {
                if(!currentDuration.isEmpty())
                {
                    durations.addWithoutMerging(currentDuration);
                }
                if(w >= 1.0f)
                {
                    currentDuration = {x, y, w, height};
                }
                else
                {
                    currentDuration = {};
                }
            }

            if(showLabel && !std::get<2>(*it).empty())
            {
                auto const previousLabelLimit = labels.empty() ? x : static_cast<float>(std::get<1>(labels.back()) + std::get<2>(labels.back()));
                if(previousLabelLimit <= x)
                {
                    auto const text = juce::String(std::get<2>(*it)) + unit;
                    auto const textWidth = font.getStringWidth(text) + 2;
                    auto const textX = static_cast<int>(std::round(x)) + 2;
                    labels.push_back(std::make_tuple(text, textX, textWidth));
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

    g.setColour(colours.foreground.withAlpha(0.4f));
    g.fillRectList(durations);

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
        g.setColour(colours.text);
        for(auto const& label : labels)
        {
            g.drawSingleLineText(std::get<0>(label), std::get<1>(label), bounds.getY() + 22, juce::Justification::left);
        }
    }
}

void Track::Renderer::paintPoints(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr)
{
    auto const& timeRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const& valueRange = accessor.getAcsr<AcsrType::valueZoom>().getAttr<Zoom::AttrType::visibleRange>();
    auto const& colours = accessor.getAttr<AttrType::colours>();
    auto const& unit = Tools::getUnit(accessor);

    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return;
    }

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
                paintPoints(g, bounds, markers->at(channel), thesholds, *data, timeRange, valueRange, colours, unit);
                return;
            }
        }
        paintPoints(g, bounds, markers->at(channel), thesholds, {}, timeRange, valueRange, colours, unit);
    }
}

void Track::Renderer::paintPoints(juce::Graphics& g, juce::Rectangle<int> const& bounds, std::vector<Result::Data::Point> const& results, std::vector<std::optional<float>> const& thresholds, std::vector<Result::Data::Point> const& extra, juce::Range<double> const& timeRange, juce::Range<double> const& valueRange, ColourSet const& colours, juce::String const& unit)
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
        auto const y = Tools::valueToPixel(value, valueRange, fbounds);
        auto const fullLine = std::get<0_z>(results.at(0)) + std::get<1_z>(results.at(0)) < std::numeric_limits<double>::epsilon() * 2.0;
        auto const x1 = fullLine ? fbounds.getX() : Tools::secondsToPixel(std::get<0_z>(results.at(0)), timeRange, fbounds);
        auto const x2 = fullLine ? fbounds.getRight() : Tools::secondsToPixel(std::get<0_z>(results.at(0)) + std::get<1_z>(results.at(0)), timeRange, fbounds);
        if(!colours.shadow.isTransparent())
        {
            g.setColour(colours.shadow.withMultipliedAlpha(0.5f));
            g.drawLine(x1, y + 2.f, x2, y + 2.f);
            g.setColour(colours.shadow.withMultipliedAlpha(0.75f));
            g.drawLine(x1, y + 1.f, x2, y + 1.f);
        }
        g.setColour(colours.foreground);
        g.drawLine(x1, y, x2, y);

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
        while(iterator != results.cend() && std::get<0_z>(*iterator) < limit)
        {
            if(itShowsValues(iterator))
            {
                auto const& lvalue = std::get<2_z>(*iterator).value();
                min = std::min(min, lvalue);
                max = std::max(max, lvalue);
                previous = iterator;
            }
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
                auto const y1 = Tools::valueToPixel(max, valueRange, fbounds);
                auto const y2 = Tools::valueToPixel(min, valueRange, fbounds);
                if(pathArr.isLineStarted())
                {
                    pathArr.addLine(x, x, y1);
                }
                pathArr.stopLine();

                rectangles.addWithoutMerging(juce::Rectangle<float>(x, y1, x2 - x, y2 - y1));
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
                auto const y = Tools::valueToPixel(value, valueRange, fbounds);
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
            auto const y = Tools::valueToPixel(value, valueRange, fbounds);
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
        if(!colours.shadow.isTransparent())
        {
            rectangles.offsetAll(0.0f, 2.0f);

            g.setColour(colours.shadow.withMultipliedAlpha(0.5f));
            g.fillRectList(rectangles);
            rectangles.offsetAll(0.0f, -1.0f);

            g.setColour(colours.shadow.withMultipliedAlpha(0.75f));
            g.fillRectList(rectangles);
            rectangles.offsetAll(0.0f, -1.0f);
        }
        g.setColour(colours.foreground);
        g.fillRectList(rectangles);
    }

    pathArr.draw(g, colours.foreground, colours.shadow);
    if(showLabel)
    {
        labelArr.draw(g, colours.text);
    }
}

void Track::Renderer::paintColumns(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr)
{
    auto const images = accessor.getAttr<AttrType::graphics>();
    if(images.empty() || images.size() <= channel || images.at(channel).empty())
    {
        return;
    }

    if(bounds.isEmpty() || g.getClipBounds().isEmpty())
    {
        return;
    }

    auto const& globalTimeRange = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
    auto const& visibleTimeRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    if(globalTimeRange.isEmpty() || visibleTimeRange.isEmpty())
    {
        return;
    }

    auto const renderImage = [&](juce::Image const& image, Zoom::Accessor const& xZoomAcsr, Zoom::Accessor const& yZoomAcsr)
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

        auto const xClippedRange = clipZoomRange(bounds.getHorizontalRange(), clipBounds.getHorizontalRange(), getZoomRange(xZoomAcsr, false));
        auto const xRange = toImageRange(xZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), xClippedRange, image.getWidth());

        auto const yClippedRange = clipZoomRange(bounds.getVerticalRange(), clipBounds.getVerticalRange(), getZoomRange(yZoomAcsr, true));
        auto const yRange = toImageRange(yZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), yClippedRange, image.getHeight());

        g.setColour(juce::Colours::black);
        g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
        paintClippedImage(g, image, {xRange.getStart(), yRange.getStart(), xRange.getLength(), yRange.getLength()});
    };

    auto const getZoomRatio = [](Zoom::Accessor const& acsr)
    {
        return acsr.getAttr<Zoom::AttrType::globalRange>().getLength() / acsr.getAttr<Zoom::AttrType::visibleRange>().getLength();
    };

    auto const& binZoomAcsr = accessor.getAcsr<AcsrType::binZoom>();

    auto const timeZoomRatio = getZoomRatio(timeZoomAcsr);
    auto const binZoomRatio = getZoomRatio(binZoomAcsr);
    auto const boundsDimension = std::max(bounds.getWidth() * timeZoomRatio, bounds.getHeight() * binZoomRatio);
    for(auto const& image : images.at(channel))
    {
        auto const imageDimension = std::max(image.getWidth(), image.getHeight());
        if(imageDimension >= boundsDimension)
        {
            renderImage(image, timeZoomAcsr, binZoomAcsr);
            return;
        }
    }
    renderImage(images.at(channel).back(), timeZoomAcsr, binZoomAcsr);
}

ANALYSE_FILE_END
