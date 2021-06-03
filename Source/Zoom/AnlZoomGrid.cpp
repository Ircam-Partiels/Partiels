#include "AnlZoomGrid.h"

ANALYSE_FILE_BEGIN

void Zoom::Grid::paintVertical(juce::Graphics& g, Accessor const& accessor, juce::Range<double> visibleRange, juce::Rectangle<int> const& bounds, std::function<juce::String(double)> const stringify, juce::Justification justification)
{
    anlWeakAssert(justification.getOnlyVerticalFlags() == 0);
    justification = justification.getOnlyVerticalFlags();
    auto const clipBounds = g.getClipBounds();
    if(bounds.isEmpty() || clipBounds.isEmpty() || visibleRange.isEmpty())
    {
        return;
    }

    auto const y = bounds.getY();
    auto const height = bounds.getHeight();
    auto const font = g.getCurrentFont();
    auto const stringHeight = font.getHeight() + 2.0f;

    auto const rangeStart = visibleRange.getStart();
    auto const rangeLength = visibleRange.getLength();

    auto getTickDrawingInfo = [&]()
    {
        auto const numTicks = std::max(1.0, std::floor(static_cast<double>(height - 1) / stringHeight));
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

        auto const firstValue = std::floor((rangeStart - tickReferenceValue) / discreteIntervalValue) * discreteIntervalValue + tickReferenceValue;
        auto const discreteNumTick = static_cast<size_t>(std::ceil(rangeLength / discreteIntervalValue)) + 1;
        return std::make_tuple(discreteNumTick, firstValue, discreteIntervalValue);
    };

    auto const tickDrawingInfo = getTickDrawingInfo();
    auto const largeTickSpacing = std::get<2>(tickDrawingInfo) * static_cast<double>(accessor.getAttr<AttrType::mainTickInterval>() + 1_z);
    auto const primaryTickEpsilon = visibleRange.getLength() / std::max(static_cast<double>(bounds.getHeight() - 2), 1.0);
    auto const width = static_cast<float>(bounds.getWidth());

    auto getPosition = [&](double value)
    {
        return (1.0 - (value - rangeStart) / rangeLength) * static_cast<double>(height) + static_cast<double>(y);
    };

    juce::Path path;
    for(auto index = 0_z; index < std::get<0>(tickDrawingInfo); ++index)
    {
        auto const value = std::get<1>(tickDrawingInfo) + static_cast<double>(index) * std::get<2>(tickDrawingInfo);
        auto const isPrimaryTick = std::abs(std::remainder(value - accessor.getAttr<AttrType::tickReference>(), largeTickSpacing)) < primaryTickEpsilon;

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

        if(isPrimaryTick && stringify != nullptr && yPos > y && yPos < y + height)
        {
            auto const text = stringify(value);
            g.drawText(text, 2, static_cast<int>(std::floor(yPos) - font.getAscent()) - 1, static_cast<int>(width), static_cast<int>(std::ceil(font.getHeight())), justification);
        }
    }
    g.fillPath(path);
}

ANALYSE_FILE_END
