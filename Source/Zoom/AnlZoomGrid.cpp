#include "AnlZoomGrid.h"

ANALYSE_FILE_BEGIN

Zoom::Grid::TickDrawingInfo Zoom::Grid::getTickDrawingInfo(Accessor const& accessor, juce::Range<double> const& visibleRange, int size, double maxStringSize)
{
    auto const rangeLength = visibleRange.getLength();
    auto const numTicks = std::max(1.0, std::floor(static_cast<double>(size) / maxStringSize));
    auto const intervalValue = rangeLength / numTicks;
    auto const tickPowerBase = accessor.getAttr<AttrType::tickPowerBase>();
    auto const tickDivisionFactor = accessor.getAttr<AttrType::tickDivisionFactor>();
    auto const intervalPower = std::ceil(std::log(intervalValue) / std::log(tickPowerBase));
    auto const discreteIntervalValueNonDivided = std::pow(tickPowerBase, intervalPower);

    auto const tickReferenceValue = accessor.getAttr<AttrType::tickReference>();

    auto discreteIntervalValue = discreteIntervalValueNonDivided;
    if(tickDivisionFactor > 1.0)
    {
        while(discreteIntervalValue / tickDivisionFactor / rangeLength > 1.0)
        {
            discreteIntervalValue /= tickDivisionFactor;
        }
    }

    auto const firstValue = std::floor((visibleRange.getStart() - tickReferenceValue) / discreteIntervalValue) * discreteIntervalValue + tickReferenceValue;
    auto const discreteNumTick = static_cast<size_t>(std::ceil(rangeLength / discreteIntervalValue)) + 1;
    auto const mainTickSpacing = discreteIntervalValue * static_cast<double>(accessor.getAttr<AttrType::mainTickInterval>() + 1_z);
    auto const mainTickEpsilon = visibleRange.getLength() / std::max(static_cast<double>(size), 1.0);
    // the number of ticks to display within the visible range,
    // the value of the first tick within the visible range,
    // the interval between two ticks,
    // the interval between two main ticks,
    // the epsilon used to get the main tick.
    return std::make_tuple(discreteNumTick, firstValue, discreteIntervalValue, mainTickSpacing, mainTickEpsilon);
}

bool Zoom::Grid::isMainTick(Accessor const& accessor, Zoom::Grid::TickDrawingInfo const& info, double const value)
{
    return std::abs(std::remainder(value - accessor.getAttr<AttrType::tickReference>(), std::get<3>(info))) < std::get<4>(info);
}

void Zoom::Grid::paintVertical(juce::Graphics& g, Accessor const& accessor, juce::Range<double> const& visibleRange, juce::Rectangle<int> const& bounds, std::function<juce::String(double)> const stringify, Justification justification)
{
    anlWeakAssert(justification.getOnlyVerticalFlags() == 0);
    justification = justification.getOnlyHorizontalFlags();
    if(justification == 0)
    {
        return;
    }
    auto const clipBounds = g.getClipBounds();
    if(bounds.isEmpty() || clipBounds.isEmpty() || visibleRange.isEmpty())
    {
        return;
    }

    auto const y = bounds.getY();
    auto const height = bounds.getHeight();
    auto const font = g.getCurrentFont();

    auto const width = static_cast<float>(bounds.getWidth());

    auto const rangeStart = visibleRange.getStart();
    auto const rangeLength = visibleRange.getLength();
    auto getPosition = [&](double value)
    {
        return (1.0 - (value - rangeStart) / rangeLength) * static_cast<double>(height + 1) + static_cast<double>(y);
    };

    auto const tickDrawingInfo = getTickDrawingInfo(accessor, visibleRange, height - 1, font.getHeight() + 2.0f);
    juce::Path path;
    for(auto index = 0_z; index < std::get<0>(tickDrawingInfo); ++index)
    {
        auto const value = std::get<1>(tickDrawingInfo) + static_cast<double>(index) * std::get<2>(tickDrawingInfo);
        auto const isPrimaryTick = isMainTick(accessor, tickDrawingInfo, value);
        auto const tickWidth = isPrimaryTick ? 12.0f : 8.0f;

        auto const yPos = static_cast<float>(getPosition(value));
        if(justification.testFlags(Justification::left) || justification.testFlags(Justification::horizontallyCentred))
        {
            path.addLineSegment(juce::Line<float>(0.0f, yPos, tickWidth, yPos), 1.0f);
        }
        if(justification.testFlags(Justification::right) || justification.testFlags(Justification::horizontallyCentred))
        {
            path.addLineSegment(juce::Line<float>(width - tickWidth, yPos, width, yPos), 1.0f);
        }
        if(justification.testFlags(Justification::horizontallyCentred) && isPrimaryTick)
        {
            path.addLineSegment(juce::Line<float>(0.0f, yPos, width, yPos), 1.0f);
        }

        if(isPrimaryTick && stringify != nullptr && yPos > static_cast<float>(y) && yPos < static_cast<float>(y + height))
        {
            auto const text = stringify(value);
            g.drawText(text, 2, static_cast<int>(std::floor(yPos) - font.getAscent()) - 1, static_cast<int>(width - 4), static_cast<int>(std::ceil(font.getHeight())), justification);
        }
    }
    g.fillPath(path);
}

void Zoom::Grid::paintHorizontal(juce::Graphics& g, Accessor const& accessor, juce::Range<double> const& visibleRange, juce::Rectangle<int> const& bounds, std::function<juce::String(double)> const stringify, int maxStringWidth, Justification justification)
{
    anlWeakAssert(justification.getOnlyHorizontalFlags() == 0);
    justification = justification.getOnlyVerticalFlags();
    if(justification == 0)
    {
        return;
    }
    auto const clipBounds = g.getClipBounds();
    if(bounds.isEmpty() || clipBounds.isEmpty() || visibleRange.isEmpty())
    {
        return;
    }

    auto const x = bounds.getX();
    auto const width = bounds.getWidth();
    auto const height = bounds.getHeight();
    auto const font = g.getCurrentFont();

    auto const rangeStart = visibleRange.getStart();
    auto const rangeLength = visibleRange.getLength();
    auto getPosition = [&](double value)
    {
        return (value - rangeStart) / rangeLength * static_cast<double>(width + 1) + static_cast<double>(x);
    };

    auto const tickDrawingInfo = getTickDrawingInfo(accessor, visibleRange, width - 1, static_cast<double>(maxStringWidth));
    juce::Path path;
    auto const fheight = static_cast<float>(height);
    for(auto index = 0_z; index < std::get<0>(tickDrawingInfo); ++index)
    {
        auto const value = std::get<1>(tickDrawingInfo) + static_cast<double>(index) * std::get<2>(tickDrawingInfo);
        auto const isPrimaryTick = isMainTick(accessor, tickDrawingInfo, value);
        auto const tickHeight = isPrimaryTick ? 8.0f : 6.0f;

        auto const xPos = static_cast<float>(getPosition(value));
        if(justification.testFlags(Justification::top) || justification.testFlags(Justification::verticallyCentred))
        {
            path.addLineSegment(juce::Line<float>(xPos, 0.0f, xPos, tickHeight), 1.0f);
        }
        if(justification.testFlags(Justification::bottom) || justification.testFlags(Justification::verticallyCentred))
        {
            path.addLineSegment(juce::Line<float>(xPos, fheight - tickHeight, xPos, fheight), 1.0f);
        }
        else if(justification.testFlags(Justification::verticallyCentred) || isPrimaryTick)
        {
            path.addLineSegment(juce::Line<float>(xPos, 0.0f, xPos, fheight), 1.0f);
        }

        if(isPrimaryTick && stringify != nullptr)
        {
            auto const text = stringify(value);
            g.drawText(text, static_cast<int>(std::floor(xPos + 2.0f)), static_cast<int>(std::floor(fheight - font.getAscent())), maxStringWidth, static_cast<int>(font.getHeight()), justification);
        }
    }
    g.fillPath(path);
}

ANALYSE_FILE_END
