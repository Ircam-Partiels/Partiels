#include "AnlTrackStateButton.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::StateButton::StateButton(Accessor& accessor)
: mAccessor(accessor)
{
    addAndMakeVisible(mProcessingButton);
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::name:
            case AttrType::processing:
            case AttrType::warnings:
            {
                auto const state = acsr.getAttr<AttrType::processing>();
                auto const warnings = acsr.getAttr<AttrType::warnings>();
                auto getInactiveIconType = [warnings]()
                {
                    return warnings == WarningType::none ? Icon::Type::verified : Icon::Type::alert;
                };
                mProcessingButton.setInactiveImage(Icon::getImage(getInactiveIconType()));
                auto const tooltip = Tools::getStateTootip(acsr);
                mProcessingButton.setTooltip(tooltip);
                setTooltip(tooltip);
                mProcessingButton.setActive(std::get<0>(state) || std::get<2>(state));
            }
            break;
            case AttrType::key:
            case AttrType::description:
            case AttrType::state:
            case AttrType::file:
            case AttrType::results:
            case AttrType::graphics:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::colours:
            case AttrType::channelsLayout:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::focused:
            case AttrType::grid:
                break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::StateButton::~StateButton()
{
    mAccessor.removeListener(mListener);
}

void Track::StateButton::resized()
{
    mProcessingButton.setBounds(getLocalBounds());
}

ANALYSE_FILE_END
