#include "AnlTrackGrid.h"
#include "../Zoom/AnlZoomTools.h"
#include "../Zoom/AnlZoomGrid.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Grid::Grid(Accessor& accessor)
: mAccessor(accessor)
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
            case AttrType::colours:
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
                    }
                        break;
                    case Tools::DisplayType::columns:
                    {
                        mAccessor.getAcsr<AcsrType::valueZoom>().removeListener(mZoomListener);
                        mAccessor.getAcsr<AcsrType::binZoom>().addListener(mZoomListener, NotificationType::synchronous);
                    }
                        break;
                }
            }
                break;
            case AttrType::channelsLayout:
            {
                repaint();
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
            case Zoom::AttrType::gridInfo:
            case Zoom::AttrType::visibleRange:
            {
                repaint();
            }
                break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::valueZoom>().addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::binZoom>().addListener(mZoomListener, NotificationType::synchronous);
}

Track::Grid::~Grid()
{
    mAccessor.getAcsr<AcsrType::binZoom>().removeListener(mZoomListener);
    mAccessor.getAcsr<AcsrType::valueZoom>().removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Track::Grid::paint(juce::Graphics& g)
{
    auto stringify = [unit = mAccessor.getAttr<AttrType::description>().output.unit](double value)
    {
        return value >= 1000.0 ? juce::String(value / 1000.0, 2) + "k" : juce::String(value, 2);
    };
    
    switch(Tools::getDisplayType(mAccessor))
    {
        case Tools::DisplayType::markers:
            break;
        case Tools::DisplayType::points:
        {
            auto const& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
            Zoom::Grid::paintVertical(g, zoomAcsr, getLocalBounds(), stringify, juce::Justification::left);
        }
            break;
        case Tools::DisplayType::columns:
        {
            auto const& zoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
            Zoom::Grid::paintVertical(g, zoomAcsr, getLocalBounds(), stringify, juce::Justification::left);
        }
            break;
    }
}

ANALYSE_FILE_END
