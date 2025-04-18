#include "MiscZoomGrid.h"

MISC_FILE_BEGIN

Zoom::Grid::PropertyPanel::PropertyPanel()
: mPropertyTickReference("Tick Reference Value", "The reference value used as the ruler origin", "", {std::numeric_limits<float>::min(), std::numeric_limits<float>::max()}, 0.0f, [&](float value)
                         {
                             if(onChangeBegin != nullptr)
                             {
                                 onChangeBegin(mAccessor);
                             }
                             mAccessor.setAttr<AttrType::tickReference>(static_cast<double>(value), NotificationType::synchronous);
                             if(onChangeEnd != nullptr)
                             {
                                 onChangeEnd(mAccessor);
                             }
                         })
, mPropertyTickBase("Ruler Mode", "The mode of the ruler numeral system.", "", std::vector<std::string>{"1", "2", "3", "4", "5", "6"}, [&](size_t index)
                    {
                        MiscWeakAssert(index < sGridBaseInfoArray.size());
                        if(index >= sGridBaseInfoArray.size())
                        {
                            return;
                        }
                        if(onChangeBegin != nullptr)
                        {
                            onChangeBegin(mAccessor);
                        }
                        auto const gridBaseInfo = sGridBaseInfoArray[index];
                        mAccessor.setAttr<Zoom::Grid::AttrType::mainTickInterval>(std::get<0>(gridBaseInfo), NotificationType::synchronous);
                        mAccessor.setAttr<Zoom::Grid::AttrType::tickPowerBase>(std::get<1>(gridBaseInfo), NotificationType::synchronous);
                        mAccessor.setAttr<Zoom::Grid::AttrType::tickDivisionFactor>(std::get<2>(gridBaseInfo), NotificationType::synchronous);
                        if(onChangeEnd != nullptr)
                        {
                            onChangeEnd(mAccessor);
                        }
                    })
, mPropertyMainTickInterval("Main Tick Interval", "The interval between two main ticks", "", {0.0f, 100000.0f}, 1.0f, [&](float value)
                            {
                                if(onChangeBegin != nullptr)
                                {
                                    onChangeBegin(mAccessor);
                                }
                                mAccessor.setAttr<AttrType::mainTickInterval>(static_cast<size_t>(value), NotificationType::synchronous);
                                if(onChangeEnd != nullptr)
                                {
                                    onChangeEnd(mAccessor);
                                }
                            })
, mPropertyTickPowerBase("Power Base", "The power base of the ruler", "", {1.0, 20.0f}, 0.0f, [&](float value)
                         {
                             if(onChangeBegin != nullptr)
                             {
                                 onChangeBegin(mAccessor);
                             }
                             mAccessor.setAttr<AttrType::tickPowerBase>(static_cast<double>(value), NotificationType::synchronous);
                             if(onChangeEnd != nullptr)
                             {
                                 onChangeEnd(mAccessor);
                             }
                         })
, mPropertyTickDivisionFactor("Division Factor", "The division factor of the ruler", "", {1.0, 20.0f}, 0.0f, [&](float value)
                              {
                                  if(onChangeBegin != nullptr)
                                  {
                                      onChangeBegin(mAccessor);
                                  }
                                  mAccessor.setAttr<AttrType::tickDivisionFactor>(static_cast<double>(value), NotificationType::synchronous);
                                  if(onChangeEnd != nullptr)
                                  {
                                      onChangeEnd(mAccessor);
                                  }
                              })
{
    addAndMakeVisible(mPropertyTickReference);
    addAndMakeVisible(mPropertyTickBase);
    addAndMakeVisible(mPropertyMainTickInterval);
    addAndMakeVisible(mPropertyTickPowerBase);
    //    addAndMakeVisible(mPropertyTickDivisionFactor);

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::tickReference:
            {
                mPropertyTickReference.entry.setValue(acsr.getAttr<AttrType::tickReference>(), juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::mainTickInterval:
            case AttrType::tickPowerBase:
            case AttrType::tickDivisionFactor:
            {
                auto const mainTickInterval = acsr.getAttr<AttrType::mainTickInterval>();
                auto const tickPowerBase = acsr.getAttr<AttrType::tickPowerBase>();
                auto const tickDivisionFactor = acsr.getAttr<AttrType::tickDivisionFactor>();
                auto it = std::find_if(sGridBaseInfoArray.cbegin(), sGridBaseInfoArray.cend(), [&](auto const& rhs)
                                       {
                                           return mainTickInterval == std::get<0>(rhs) && std::abs(tickPowerBase - std::get<1>(rhs)) < std::numeric_limits<double>::epsilon() && std::abs(tickDivisionFactor - std::get<2>(rhs)) < std::numeric_limits<double>::epsilon();
                                       });
                mPropertyTickBase.entry.setSelectedItemIndex(static_cast<int>(std::distance(sGridBaseInfoArray.cbegin(), it)), juce::NotificationType::dontSendNotification);
                mPropertyTickBase.entry.setTextWhenNothingSelected(juce::translate("Custom"));
                mPropertyMainTickInterval.entry.setValue(static_cast<double>(mainTickInterval), juce::NotificationType::dontSendNotification);
                mPropertyTickPowerBase.entry.setValue(tickPowerBase, juce::NotificationType::dontSendNotification);
                mPropertyTickDivisionFactor.entry.setValue(tickDivisionFactor, juce::NotificationType::dontSendNotification);
            }
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);

    setSize(sInnerWidth, 400);
}

Zoom::Grid::PropertyPanel::~PropertyPanel()
{
    mAccessor.removeListener(mListener);
}

void Zoom::Grid::PropertyPanel::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto layout = [&](juce::Component& component)
    {
        component.setBounds(bounds.removeFromTop(component.getHeight()));
    };
    layout(mPropertyTickReference);
    layout(mPropertyTickBase);
    layout(mPropertyMainTickInterval);
    layout(mPropertyTickPowerBase);
    //    layout(mPropertyTickDivisionFactor);
    setSize(sInnerWidth, bounds.getY() + 2);
}

void Zoom::Grid::PropertyPanel::setGrid(Accessor const& accessor)
{
    mAccessor.copyFrom(accessor, NotificationType::synchronous);
}

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
    MiscWeakAssert(justification.getOnlyVerticalFlags() == 0);
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

    auto const textJustification = justification.testFlags(Justification::left) || justification.testFlags(Justification::horizontallyCentred) ? juce::Justification::left : juce::Justification::right;
    auto const tickDrawingInfo = getTickDrawingInfo(accessor, visibleRange, height - 1, font.getHeight() + 2.0f);
    juce::Path path;
    auto const x = static_cast<float>(bounds.getX());
    for(auto index = 0_z; index < std::get<0>(tickDrawingInfo); ++index)
    {
        auto const value = std::get<1>(tickDrawingInfo) + static_cast<double>(index) * std::get<2>(tickDrawingInfo);
        auto const isPrimaryTick = isMainTick(accessor, tickDrawingInfo, value);
        auto const tickWidth = isPrimaryTick ? 12.0f : 8.0f;

        auto const yPos = static_cast<float>(getPosition(value));
        if(justification.testFlags(Justification::left) || justification.testFlags(Justification::horizontallyCentred))
        {
            path.addLineSegment(juce::Line<float>(x, yPos, tickWidth + x, yPos), 1.0f);
        }
        if(justification.testFlags(Justification::right) || justification.testFlags(Justification::horizontallyCentred))
        {
            path.addLineSegment(juce::Line<float>(width - tickWidth + x, yPos, width + x, yPos), 1.0f);
        }
        if(justification.testFlags(Justification::horizontallyCentred) && isPrimaryTick)
        {
            path.addLineSegment(juce::Line<float>(x, yPos, width + x, yPos), 1.0f);
        }

        if(isPrimaryTick && stringify != nullptr && yPos > static_cast<float>(y) && yPos < static_cast<float>(y + height))
        {
            auto const text = stringify(value);
            g.drawText(text, 2 + static_cast<int>(x), static_cast<int>(std::floor(yPos) - font.getAscent()) - 1, static_cast<int>(width - 4.0f + x), static_cast<int>(std::ceil(font.getHeight())), textJustification);
        }
    }
    g.fillPath(path);
}

void Zoom::Grid::paintHorizontal(juce::Graphics& g, Accessor const& accessor, juce::Range<double> const& visibleRange, juce::Rectangle<int> const& bounds, std::function<juce::String(double)> const stringify, int maxStringWidth, Justification justification)
{
    MiscWeakAssert(justification.getOnlyHorizontalFlags() == 0);
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
    auto const y = static_cast<float>(bounds.getY());
    for(auto index = 0_z; index < std::get<0>(tickDrawingInfo); ++index)
    {
        auto const value = std::get<1>(tickDrawingInfo) + static_cast<double>(index) * std::get<2>(tickDrawingInfo);
        auto const isPrimaryTick = isMainTick(accessor, tickDrawingInfo, value);
        auto const tickHeight = isPrimaryTick ? 8.0f : 6.0f;

        auto const xPos = static_cast<float>(getPosition(value));
        if(justification.testFlags(Justification::top) || justification.testFlags(Justification::verticallyCentred))
        {
            path.addLineSegment(juce::Line<float>(xPos, y, xPos, tickHeight + y), 1.0f);
        }
        if(justification.testFlags(Justification::bottom) || justification.testFlags(Justification::verticallyCentred))
        {
            path.addLineSegment(juce::Line<float>(xPos, fheight - tickHeight + y, xPos, fheight + y), 1.0f);
        }
        if(justification.testFlags(Justification::verticallyCentred) && isPrimaryTick)
        {
            path.addLineSegment(juce::Line<float>(xPos, y, xPos, fheight + y), 1.0f);
        }

        if(isPrimaryTick && stringify != nullptr)
        {
            auto const text = stringify(value);
            g.drawText(text, static_cast<int>(std::floor(xPos + 2.0f)), static_cast<int>(std::floor(fheight - font.getAscent() + y)), maxStringWidth, static_cast<int>(font.getHeight()), justification);
        }
    }
    g.fillPath(path);
}

MISC_FILE_END
