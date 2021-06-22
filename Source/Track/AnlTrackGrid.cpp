#include "AnlTrackGrid.h"
#include "../Zoom/AnlZoomGrid.h"
#include "../Zoom/AnlZoomTools.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Grid::Grid(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Zoom::Grid::Justification const justification)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
, mJustification(justification)
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

    if(mJustification.getOnlyVerticalFlags() != 0)
    {
        mTimeZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
        mTimeZoomAccessor.getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
    }
    if(mJustification.getOnlyHorizontalFlags() != 0)
    {
        mAccessor.addListener(mListener, NotificationType::synchronous);
    }
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
    if(mJustification.getOnlyVerticalFlags() != 0)
    {
        mTimeZoomAccessor.getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
        mTimeZoomAccessor.removeListener(mZoomListener);
    }
}

void Track::Grid::paint(juce::Graphics& g)
{
    auto const stringify = [unit = mAccessor.getAttr<AttrType::description>().output.unit](double value)
    {
        return Format::valueToString(value, 4) + unit;
    };

    auto const paintChannel = [&](Zoom::Accessor const& zoomAcsr, juce::Rectangle<int> const& region)
    {
        g.setColour(mAccessor.getAttr<AttrType::colours>().grid);
        Zoom::Grid::paintVertical(g, zoomAcsr.getAcsr<Zoom::AcsrType::grid>(), zoomAcsr.getAttr<Zoom::AttrType::visibleRange>(), region, stringify, mJustification.getOnlyHorizontalFlags());
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

    g.setColour(mAccessor.getAttr<AttrType::colours>().grid);
    Zoom::Grid::paintHorizontal(g, mTimeZoomAccessor.getAcsr<Zoom::AcsrType::grid>(), mTimeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>(), getLocalBounds(), nullptr, 70, mJustification.getOnlyVerticalFlags());
}

ANALYSE_FILE_END
