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

void Zoom::Grid::paintVertical(juce::Graphics& g, Accessor const& accessor, juce::Range<double> const& visibleRange, juce::Rectangle<int> const& bounds, std::function<juce::String(double)> const stringify, juce::Justification justification)
{
    anlWeakAssert(justification.getOnlyVerticalFlags() == 0);
    justification = justification.getOnlyHorizontalFlags();
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
        return (1.0 - (value - rangeStart) / rangeLength) * static_cast<double>(height) + static_cast<double>(y);
    };

    auto const tickDrawingInfo = getTickDrawingInfo(accessor, visibleRange, height - 1, font.getHeight() + 2.0f);
    juce::Path path;
    for(auto index = 0_z; index < std::get<0>(tickDrawingInfo); ++index)
    {
        auto const value = std::get<1>(tickDrawingInfo) + static_cast<double>(index) * std::get<2>(tickDrawingInfo);
        auto const isPrimaryTick = isMainTick(accessor, tickDrawingInfo, value);

        auto const yPos = static_cast<float>(getPosition(value));
        if(justification.testFlags(juce::Justification::left))
        {
            auto const tickWidth = isPrimaryTick ? 12.0f : 8.0f;
            path.addLineSegment(juce::Line<float>(0.0f, yPos, tickWidth, yPos), 1.0f);
        }
        else if(justification.testFlags(juce::Justification::right))
        {
            auto const tickWidth = isPrimaryTick ? 12.0f : 8.0f;
            path.addLineSegment(juce::Line<float>(width - tickWidth, yPos, width, yPos), 1.0f);
        }
        else
        {
            if(isPrimaryTick)
            {
                path.addLineSegment(juce::Line<float>(0.0f, yPos, width, yPos), 1.0f);
            }
            else
            {
                auto const tickWidth = 8.0f;
                path.addLineSegment(juce::Line<float>(width - tickWidth, yPos, width, yPos), 1.0f);
                path.addLineSegment(juce::Line<float>(0.0f, yPos, tickWidth, yPos), 1.0f);
            }
        }

        if(isPrimaryTick && stringify != nullptr && yPos > y && yPos < y + height)
        {
            auto const text = stringify(value);
            g.drawText(text, 2, static_cast<int>(std::floor(yPos) - font.getAscent()) - 1, static_cast<int>(width - 4), static_cast<int>(std::ceil(font.getHeight())), justification);
        }
    }
    g.fillPath(path);
}

void Zoom::Grid::paintHorizontal(juce::Graphics& g, Accessor const& accessor, juce::Range<double> const& visibleRange, juce::Rectangle<int> const& bounds, std::function<juce::String(double)> const stringify, double maxStringWidth, juce::Justification justification)
{
    anlWeakAssert(justification.getOnlyHorizontalFlags() == 0);
    justification = justification.getOnlyVerticalFlags();
    auto const clipBounds = g.getClipBounds();
    if(bounds.isEmpty() || clipBounds.isEmpty() || visibleRange.isEmpty())
    {
        return;
    }
    
    auto const x = bounds.getX();
    auto const width = bounds.getWidth();
    auto const font = g.getCurrentFont();
    
    
    //auto const height = static_cast<float>(bounds.getHeight());
    
    auto const rangeStart = visibleRange.getStart();
    auto const rangeLength = visibleRange.getLength();
    auto getPosition = [&](double value)
    {
        return (1.0 - (value - rangeStart) / rangeLength) * static_cast<double>(width) + static_cast<double>(x);
    };
    
    auto const tickDrawingInfo = getTickDrawingInfo(accessor, visibleRange, width - 1, maxStringWidth);
    juce::Path path;
    for(auto index = 0_z; index < std::get<0>(tickDrawingInfo); ++index)
    {
        auto const value = std::get<1>(tickDrawingInfo) + static_cast<double>(index) * std::get<2>(tickDrawingInfo);
        auto const isPrimaryTick = isMainTick(accessor, tickDrawingInfo, value);
        
        auto const yPos = static_cast<float>(getPosition(value));
        if(justification.testFlags(juce::Justification::left))
        {
            path.addLineSegment(juce::Line<float>(0.0f, yPos, 8.f, yPos), 1.0f);
        }
        else if(justification.testFlags(juce::Justification::right))
        {
            path.addLineSegment(juce::Line<float>(width - 8.f, yPos, 8.f, yPos), 1.0f);
        }
        else // juce::Justification::horizontallyJustified
        {
            path.addLineSegment(juce::Line<float>(0.0f, yPos, width, yPos), 1.0f);
        }
        
        if(isPrimaryTick && stringify != nullptr)
        {
//            auto const text = stringify(value);
//            g.drawText(text, 2, static_cast<int>(std::floor(yPos) - font.getAscent()) - 1, static_cast<int>(width), static_cast<int>(std::ceil(font.getHeight())), justification);
        }
    }
    g.fillPath(path);
}

ANALYSE_FILE_END
