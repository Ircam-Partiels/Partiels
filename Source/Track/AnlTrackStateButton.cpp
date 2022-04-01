#include "AnlTrackStateButton.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::StateButton::StateButton(Director& director)
: mDirector(director)
{
    addAndMakeVisible(mProcessingButton);
    addAndMakeVisible(mStateIcon);
    mStateIcon.setWantsKeyboardFocus(false);
    mStateIcon.onClick = [this]()
    {
        mDirector.askToResolveWarnings();
    };

    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::name:
            case AttrType::processing:
            case AttrType::warnings:
            {
                auto const state = acsr.getAttr<AttrType::processing>();
                auto const isProcessing = std::get<0>(state) || std::get<2>(state);
                auto const warnings = acsr.getAttr<AttrType::warnings>();
                auto const hasWarnings = warnings != WarningType::none;
                auto const tooltip = Tools::getStateTootip(acsr);

                mProcessingButton.setInactiveImage(Icon::getImage(hasWarnings ? Icon::Type::alert : Icon::Type::verified));
                mProcessingButton.setTooltip(tooltip);
                mProcessingButton.setVisible(isProcessing);
                mProcessingButton.setActive(isProcessing);

                mStateIcon.setTypes(hasWarnings ? Icon::Type::alert : Icon::Type::verified);
                mStateIcon.setEnabled(hasWarnings);
                mStateIcon.setTooltip(tooltip);
                mStateIcon.setVisible(!isProcessing);
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
    mStateIcon.setBounds(getLocalBounds());
}

ANALYSE_FILE_END
