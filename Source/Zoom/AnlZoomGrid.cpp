#include "AnlZoomGrid.h"
#include "AnlZoomTools.h"

ANALYSE_FILE_BEGIN

Zoom::Grid::Grid(Accessor& accessor, Orientation orientation, std::function<juce::String(double)> toString)
: mAccessor(accessor)
, mOrientation(orientation)
, mToString(toString != nullptr ? toString : [](double value)
{
    return value >= 1000.0 ? juce::String(value / 1000.0, 2) + "k" : juce::String(value, 2);
})
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::globalRange:
            case AttrType::minimumLength:
            case AttrType::anchor:
                break;
            case AttrType::gridInfo:
            case AttrType::visibleRange:
            {
                repaint();
            }
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    setInterceptsMouseClicks(false, false);
}

Zoom::Grid::~Grid()
{
    mAccessor.removeListener(mListener);
}

void Zoom::Grid::paint(juce::Graphics& g)
{
    switch(mOrientation)
    {
        case Orientation::horizontal:
            paintHorizontal(mAccessor, *this, mToString, g);
            break;
        case Orientation::vertical:
        {
            paintVertical(g, mAccessor, getLocalBounds(), mToString, juce::Justification::centredRight);
        }
            break;
    }
}

void Zoom::Grid::paintHorizontal(Accessor const& accessor, juce::Component const& c, StringifyFn stringify, juce::Graphics& g)
{
    auto const clipBounds = g.getClipBounds();
    auto const visibleRange = accessor.getAttr<AttrType::visibleRange>();
    if(clipBounds.isEmpty() || visibleRange.isEmpty())
    {
        return;
    }
    
    auto const font = g.getCurrentFont();
    auto const& gridInfo = accessor.getAttr<AttrType::gridInfo>();
    
    auto getTypicalStringWidth = [&]()
    {
        auto const firstString = stringify(gridInfo.ticksSpacing);
        auto const secondString = stringify(gridInfo.tickReference + gridInfo.ticksSpacing);
        return static_cast<double>(std::floor(std::max(font.getStringWidthFloat(firstString), font.getStringWidthFloat(secondString)) * 1.2f)) + 4.0;
    };
    auto const stringWidth = getTypicalStringWidth();
    
    auto getTickSpacing = [&]()
    {
        auto const size = static_cast<double>(c.getWidth());
        auto const length = visibleRange.getLength();
        auto const factor = gridInfo.largeTickInterval > 1_z ? static_cast<double>(gridInfo.largeTickInterval) : 2.0;
        auto ticksSpacing = gridInfo.ticksSpacing;
        while(ticksSpacing / length * size < stringWidth)
        {
            ticksSpacing *= factor;
        }
        return ticksSpacing;
    };
    
    auto const tickSpacing = getTickSpacing();
    auto const tickReference = gridInfo.tickReference;
    
    auto const clipValueX = Tools::getScaledValueFromWidth(accessor, c, clipBounds.getX());
    auto const clipValueRight = Tools::getScaledValueFromWidth(accessor, c, clipBounds.getRight());
    
    auto startValue = std::ceil((clipValueX - tickReference) / tickSpacing) * tickSpacing + tickReference;
    auto const endValue = std::ceil((clipValueRight - tickReference) / tickSpacing + 1.f) * tickSpacing + tickReference;
    
    g.setColour(c.findColour(ColourIds::tickColourId));
    while(startValue <= endValue)
    {
        auto const x = static_cast<float>(Tools::getScaledXFromValue(accessor, c, startValue));
        g.drawLine(x, 0.0f, x, static_cast<float>(c.getHeight()));
        g.drawText(stringify(startValue), static_cast<int>(std::ceil(x)) + 2, 2, static_cast<int>(std::ceil(stringWidth)), static_cast<int>(std::ceil(font.getHeight())), juce::Justification::centredLeft);
        startValue += tickSpacing;
    }
}

void Zoom::Grid::paintVertical(juce::Graphics& g, Accessor const& accessor, juce::Rectangle<int> const& bounds, StringifyFn const stringify, juce::Justification justification)
{
    anlWeakAssert(justification.getOnlyVerticalFlags() == 0);
    justification = justification.getOnlyVerticalFlags();
    auto const clipBounds = g.getClipBounds();
    auto const visibleRange = accessor.getAttr<AttrType::visibleRange>();
    if(bounds.isEmpty() || clipBounds.isEmpty() || visibleRange.isEmpty())
    {
        return;
    }
    
    auto const height = bounds.getHeight();
    auto const font = g.getCurrentFont();
    auto const stringHeight = font.getHeight() + 2.0f;
    
    auto const& gridInfo = accessor.getAttr<AttrType::gridInfo>();
    auto getTickSpacing = [&]()
    {
        auto const length = visibleRange.getLength();
        auto const factor = gridInfo.largeTickInterval > 1_z ? static_cast<double>(gridInfo.largeTickInterval) : 10.0;
        auto ticksSpacing = gridInfo.ticksSpacing;
        while(ticksSpacing / length * static_cast<double>(height) < stringHeight)
        {
            ticksSpacing *= factor;
        }
        return ticksSpacing;
    };
    
    auto const tickSpacing = getTickSpacing();
    auto const tickReference = gridInfo.tickReference;
    
    auto getValue = [&](int y)
    {
        return static_cast<double>((height - 1) - y) / static_cast<double>(height) * visibleRange.getLength() + visibleRange.getStart();
    };
    
    auto getPosition = [&](double value)
    {
        return static_cast<double>(height - 1) - ((value - visibleRange.getStart()) / visibleRange.getLength() * static_cast<double>(height));
    };
    
    auto const clipValueBottom = getValue(clipBounds.getBottom());
    auto const clipValueY = getValue(clipBounds.getY());
    
    auto startValue = std::ceil((clipValueBottom - tickReference) / tickSpacing) * tickSpacing + tickReference;
    auto const endValue = std::ceil((clipValueY - tickReference) / tickSpacing + 1.f) * tickSpacing + tickReference;
    
    auto const width = static_cast<float>(bounds.getWidth());
    while(startValue <= endValue)
    {
        auto const y = static_cast<float>(getPosition(startValue));
        if(justification.testFlags(juce::Justification::left))
        {
            g.drawLine(0.0f, y, 8.f, y);
        }
        else if(justification.testFlags(juce::Justification::right))
        {
            g.drawLine(width - 8.f, y, 8.f, y);
        }
        else // juce::Justification::horizontallyJustified
        {
            g.drawLine(0.0f, y, width, y);
        }
        
        if(stringify != nullptr)
        {
            auto const text = stringify(startValue);
            g.drawText(stringify(startValue), 0, static_cast<int>(std::floor(y) - font.getAscent()), static_cast<int>(width), static_cast<int>(std::ceil(font.getHeight())), justification);
        }
        startValue += tickSpacing;
    }
}

ANALYSE_FILE_END
