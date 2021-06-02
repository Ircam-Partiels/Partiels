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
    
    using TickPowerInfo = std::tuple<size_t, double, double>;
    auto getTickPowerInfo = [&]() -> TickPowerInfo
    {
        switch(accessor.getAttr<AttrType::tickBase>())
        {
            case NumericBase::binary: // 2
            {
                return std::make_tuple(0, 10.0, 4.0);
            }
                break;
            case NumericBase::octal: // 8
            {
                return std::make_tuple(2, 2.0, 5.0);
            }
                break;
            case NumericBase::decimal: // 10
            {
                return std::make_tuple(3, 2.0, 5.0);
            }
                break;
            case NumericBase::duadecimal: // 12
            {
                return std::make_tuple(2, 10.0, 5.0);
            }
                break;
            case NumericBase::hexadecimal: // 16
            {
                return std::make_tuple(3, 10.0, 2);
            }
                break;
            case NumericBase::vigedecimal: // 20
            {
                return std::make_tuple(1, 5.0, 2.0);
            }
                break;
        }
    };
    auto const tickPowerInfo = getTickPowerInfo();
    
    auto getTickDrawingInfo = [&]()
    {
        auto const length = visibleRange.getLength();
        auto const numTicks = std::max(1.0, std::floor(static_cast<double>(height - 1) / stringHeight));
        auto const intervalValue = visibleRange.getLength() / numTicks;
        auto const intervalPower = std::ceil(std::log(intervalValue) / std::log(std::get<1>(tickPowerInfo)));
        auto const discreteIntervalValueNonDivided = std::pow(std::get<1>(tickPowerInfo), intervalPower);
        
        auto const tickReferenceValue = accessor.getAttr<AttrType::tickReference>();
        
        auto discreteIntervalValue = discreteIntervalValueNonDivided;
        if(std::get<2>(tickPowerInfo) > 1.0)
        {
            while(discreteIntervalValue / std::get<2>(tickPowerInfo) / length > 1.0)
            {
                discreteIntervalValue /= std::get<2>(tickPowerInfo);
            }
        }
        
        auto const firstValue = std::floor((visibleRange.getStart() - tickReferenceValue) / discreteIntervalValue) * discreteIntervalValue + tickReferenceValue;
        auto const discreteNumTick = static_cast<size_t>(std::ceil(length / discreteIntervalValue)) + 1;
        return std::make_tuple(discreteNumTick, firstValue, discreteIntervalValue);
    };
    
    auto const tickDrawingInfo = getTickDrawingInfo();
    auto const largeTickSpacing = std::get<2>(tickDrawingInfo) * static_cast<double>(std::get<0>(tickPowerInfo) + 1_z);
    auto const primaryTickEpsilon = visibleRange.getLength() / std::max(static_cast<double>(bounds.getHeight() - 2), 1.0);
    auto const width = static_cast<float>(bounds.getWidth());
    
    auto getPosition = [&](double value)
    {
        return static_cast<double>(height - 1) - ((value - visibleRange.getStart()) / visibleRange.getLength() * static_cast<double>(height - 1)) + static_cast<double>(y + 1);
    };
    
    for(auto index = 0_z; index < std::get<0>(tickDrawingInfo); ++index)
    {
        auto const value = std::get<1>(tickDrawingInfo) + static_cast<double>(index) * std::get<2>(tickDrawingInfo);
        auto const isPrimaryTick = std::abs(std::remainder(value - accessor.getAttr<AttrType::tickReference>(), largeTickSpacing)) < primaryTickEpsilon;
        
        auto const yPos = static_cast<float>(getPosition(value));
        if(justification.testFlags(juce::Justification::left))
        {
            g.drawLine(0.0f, yPos, 8.f, yPos);
        }
        else if(justification.testFlags(juce::Justification::right))
        {
            g.drawLine(width - 8.f, yPos, 8.f, yPos);
        }
        else // juce::Justification::horizontallyJustified
        {
            g.drawLine(0.0f, yPos, width, yPos);
        }
        
        if(isPrimaryTick && stringify != nullptr && yPos > y && yPos < y + height)
        {
            auto const text = stringify(value);
            g.drawText(text, 0, static_cast<int>(std::floor(yPos) - font.getAscent()) - 1, static_cast<int>(width), static_cast<int>(std::ceil(font.getHeight())), justification);
        }
    }
}


Zoom::Grid::PropertyPanel::PropertyPanel(Accessor& accessor)
: FloatingWindowContainer("Grid Properties", *this)
, mAccessor(accessor)
, mPropertyTickReference("Tick Reference", "The reference used as the grid origin", "", {std::numeric_limits<float>::min(), std::numeric_limits<float>::max()}, 0.0f, [&](float value)
{
    mAccessor.setAttr<AttrType::tickReference>(static_cast<double>(value), NotificationType::synchronous);
})
, mPropertyTickBase("Tick Base", "The base of the tick numeral system.", "", std::vector<std::string>{"2", "8", "10", "12", "16", "20"}, [&](size_t index)
{
    mAccessor.setAttr<AttrType::tickBase>(magic_enum::enum_value<NumericBase>(index), NotificationType::synchronous);
})
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::tickReference:
            {
                mPropertyTickReference.entry.setValue(acsr.getAttr<AttrType::tickReference>(), juce::NotificationType::dontSendNotification);
            }
                break;
            case AttrType::tickBase:
            {
               mPropertyTickBase.entry.setSelectedItemIndex(static_cast<int>(magic_enum::enum_integer(acsr.getAttr<AttrType::tickBase>())), juce::NotificationType::dontSendNotification);
            }
                break;
        }
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    setSize(300, 400);
}

Zoom::Grid::PropertyPanel::~PropertyPanel()
{
    mAccessor.removeListener(mListener);
}

void Zoom::Grid::PropertyPanel::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyTickReference);
    setBounds(mPropertyTickBase);
    setSize(bounds.getWidth(), bounds.getY() + 2);
}

ANALYSE_FILE_END
