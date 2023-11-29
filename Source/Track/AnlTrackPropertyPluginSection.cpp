#include "AnlTrackPropertyPluginSection.h"

ANALYSE_FILE_BEGIN

Track::PropertyPluginSection::PropertyPluginSection(Director& director)
: mDirector(director)
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::description:
            {
                mDescriptionPanel.setDescription(acsr.getAttr<AttrType::description>());
                resized();
                break;
            }
            case AttrType::name:
            case AttrType::key:
            case AttrType::input:
            case AttrType::file:
            case AttrType::results:
            case AttrType::edit:
            case AttrType::state:
            case AttrType::graphics:
            case AttrType::processing:
            case AttrType::warnings:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::unit:
            case AttrType::channelsLayout:
            case AttrType::showInGroup:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::focused:
            case AttrType::grid:
            case AttrType::zoomValueMode:
            case AttrType::zoomAcsr:
            case AttrType::zoomLink:
            case AttrType::extraThresholds:
            case AttrType::hasPluginColourMap:
                break;
        }
    };

    addAndMakeVisible(mDescriptionPanel);
    setSize(300, mDescriptionPanel.getHeight());
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::PropertyPluginSection::~PropertyPluginSection()
{
    mAccessor.removeListener(mListener);
}

void Track::PropertyPluginSection::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    mDescriptionPanel.setBounds(bounds.removeFromTop(mDescriptionPanel.getHeight()));
    setSize(getWidth(), bounds.getY());
}

ANALYSE_FILE_END
