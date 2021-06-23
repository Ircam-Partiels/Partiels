#include "AnlTrackGrid.h"
#include "../Zoom/AnlZoomGrid.h"
#include "../Zoom/AnlZoomTools.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Grid::Grid(Accessor& accessor, Zoom::Accessor* timeZoomAccessor, Zoom::Grid::Justification const justification, bool showText)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
, mJustification(justification)
, mShowText(showText)
{
    setInterceptsMouseClicks(false, false);
    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::key:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::focused:
            case AttrType::graphics:
                break;
            case AttrType::grid:
            {
                repaint();
            }
            break;
            case AttrType::description:
            case AttrType::results:
            {
                switch(Tools::getDisplayType(mAccessor))
                {
                    case Tools::DisplayType::markers:
                    {
                        mAccessor.getAcsr<AcsrType::binZoom>().removeListener(mZoomListener);
                        mAccessor.getAcsr<AcsrType::valueZoom>().removeListener(mZoomListener);
                    }
                    break;
                    case Tools::DisplayType::points:
                    {
                        mAccessor.getAcsr<AcsrType::binZoom>().removeListener(mZoomListener);
                        mAccessor.getAcsr<AcsrType::valueZoom>().addListener(mZoomListener, NotificationType::synchronous);
                        mAccessor.getAcsr<AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
                    }
                    break;
                    case Tools::DisplayType::columns:
                    {
                        mAccessor.getAcsr<AcsrType::valueZoom>().removeListener(mZoomListener);
                        mAccessor.getAcsr<AcsrType::binZoom>().addListener(mZoomListener, NotificationType::synchronous);
                        mAccessor.getAcsr<AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
                    }
                    break;
                }
                repaint();
            }
            break;
            case AttrType::channelsLayout:
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
            case Zoom::AttrType::anchor:
                break;
            case Zoom::AttrType::visibleRange:
            {
                repaint();
            }
            break;
        }
    };

    mGridListener.onAttrChanged = [this](Zoom::Grid::Accessor const& acsr, Zoom::Grid::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };

    if(mTimeZoomAccessor != nullptr && mJustification.getOnlyVerticalFlags() != 0)
    {
        mTimeZoomAccessor->addListener(mZoomListener, NotificationType::synchronous);
        mTimeZoomAccessor->getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
    }
    if(mJustification.getOnlyHorizontalFlags() != 0)
    {
        mAccessor.addListener(mListener, NotificationType::synchronous);
    }
}

Track::Grid::Grid(Accessor& accessor, Zoom::Grid::Justification const justification)
: Grid(accessor, nullptr, justification, false)
{
}

Track::Grid::Grid(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Zoom::Grid::Justification const justification)
: Grid(accessor, &timeZoomAccessor, justification, true)
{
}

Track::Grid::~Grid()
{
    if(mJustification.getOnlyHorizontalFlags() != 0)
    {
        mAccessor.getAcsr<AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
        mAccessor.getAcsr<AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
        mAccessor.getAcsr<AcsrType::binZoom>().removeListener(mZoomListener);
        mAccessor.getAcsr<AcsrType::valueZoom>().removeListener(mZoomListener);
        mAccessor.removeListener(mListener);
    }
    if(mTimeZoomAccessor != nullptr && mJustification.getOnlyVerticalFlags() != 0)
    {
        mTimeZoomAccessor->getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
        mTimeZoomAccessor->removeListener(mZoomListener);
    }
}

void Track::Grid::paint(juce::Graphics& g)
{
    using Justification = Zoom::Grid::Justification;
    auto getJustification = [this](Justification const& justification)
    {
        auto const mode = mAccessor.getAttr<AttrType::grid>();
        switch(mode)
        {
            case GridMode::hidden:
                return Justification(0);
            case GridMode::partial:
                return justification;
            case GridMode::full:
            {
                if(justification.testFlags(Justification::left) && justification.testFlags(Justification::right))
                {
                    return Justification(Justification::horizontallyCentred);
                }
                if(justification.testFlags(Justification::top) && justification.testFlags(Justification::bottom))
                {
                    return Justification(Justification::verticallyCentred);
                }
                return justification;
            }
        }
    };

    auto const justificationHorizontal = getJustification(mJustification.getOnlyHorizontalFlags());
    auto const justificationVertical = getJustification(mJustification.getOnlyVerticalFlags());
    auto const colour = findColour(Decorator::ColourIds::normalBorderColourId);

    auto getStringify = [&]() -> std::function<juce::String(double)>
    {
        if(!mShowText)
        {
            return nullptr;
        }
        switch(Tools::getDisplayType(mAccessor))
        {
            case Tools::DisplayType::markers:
            {
                return nullptr;
            }
            case Tools::DisplayType::points:
            {
                return [unit = mAccessor.getAttr<AttrType::description>().output.unit](double value)
                {
                    return Format::valueToString(value, 4) + unit;
                };
            }
            case Tools::DisplayType::columns:
            {
                return [this](double value)
                {
                    auto const& output = mAccessor.getAttr<AttrType::description>().output;
                    auto const binIndex = value >= 0.0 ? static_cast<size_t>(std::round(value)) : 0_z;
                    if(binIndex >= output.binNames.size())
                    {
                        return juce::String(binIndex);
                    }
                    return juce::String(binIndex) + " - " + output.binNames[binIndex];
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

    switch(Tools::getDisplayType(mAccessor))
    {
        case Tools::DisplayType::markers:
            break;
        case Tools::DisplayType::points:
        {
            Tools::paintChannels(mAccessor, g, getLocalBounds(), [&](juce::Rectangle<int> region, size_t)
                                 {
                                     paintChannel(mAccessor.getAcsr<AcsrType::valueZoom>(), region);
                                 });
        }
        break;
        case Tools::DisplayType::columns:
        {
            Tools::paintChannels(mAccessor, g, getLocalBounds(), [&](juce::Rectangle<int> region, size_t)
                                 {
                                     paintChannel(mAccessor.getAcsr<AcsrType::binZoom>(), region);
                                 });
        }
        break;
    }

    if(mTimeZoomAccessor != nullptr)
    {
        g.setColour(colour);
        Zoom::Grid::paintHorizontal(g, mTimeZoomAccessor->getAcsr<Zoom::AcsrType::grid>(), mTimeZoomAccessor->getAttr<Zoom::AttrType::visibleRange>(), getLocalBounds(), nullptr, 70, justificationVertical);
    }
}

ANALYSE_FILE_END
