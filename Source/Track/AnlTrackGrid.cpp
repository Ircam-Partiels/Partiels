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
    auto paintChannels = [&](Zoom::Accessor const& zoomAcsr)
    {
        auto const channelLayout = mAccessor.getAttr<AttrType::channelsLayout>();
        auto const numVisibleChannels = static_cast<size_t>(std::count(channelLayout.cbegin(), channelLayout.cend(), true));
        if(numVisibleChannels == 0_z)
        {
            return;
        }
        
        auto stringify = [unit = mAccessor.getAttr<AttrType::description>().output.unit](double value)
        {
            auto const text = value >= 1000.0 ? juce::String(value / 1000.0, 4) + "k" : juce::String(value, 4);
            return text.trimCharactersAtEnd("0").trimCharactersAtEnd(".");
        };
        
        auto bounds = getLocalBounds();
        auto const fullHeight = bounds.getHeight();
        auto const channelHeight = (fullHeight - static_cast<int>(numVisibleChannels) + 1) / static_cast<int>(numVisibleChannels);
        
        auto channelCounter = 0_z;
        for(auto channel = 0_z; channel < channelLayout.size(); ++channel)
        {
            if(channelLayout[channel])
            {
                ++channelCounter;
                juce::Graphics::ScopedSaveState sss(g);
                auto const region = bounds.removeFromTop(channelHeight + 1).withTrimmedBottom(channelCounter == numVisibleChannels ? 1 : 0);
                g.reduceClipRegion(region);
                Zoom::Grid::paintVertical(g, zoomAcsr.getAcsr<Zoom::AcsrType::grid>(), zoomAcsr.getAttr<Zoom::AttrType::visibleRange>(), region, stringify, juce::Justification::left);
            }
        }
    };
    
    switch(Tools::getDisplayType(mAccessor))
    {
        case Tools::DisplayType::markers:
            break;
        case Tools::DisplayType::points:
        {
            paintChannels(mAccessor.getAcsr<AcsrType::valueZoom>());
        }
            break;
        case Tools::DisplayType::columns:
        {
            paintChannels(mAccessor.getAcsr<AcsrType::binZoom>());
        }
            break;
    }
}

ANALYSE_FILE_END
