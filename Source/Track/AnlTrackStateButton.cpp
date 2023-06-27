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
                auto const tooltip = acsr.getAttr<AttrType::name>() + ": " + Tools::getStateTootip(acsr);
                mProcessingButton.setInactiveImage(Icon::getImage(hasWarning() ? Icon::Type::alert : Icon::Type::verified));
                mProcessingButton.setTooltip(tooltip);
                mProcessingButton.setVisible(isProcessingOrRendering());
                mProcessingButton.setActive(isProcessingOrRendering());

                mStateIcon.setTypes(hasWarning() ? Icon::Type::alert : Icon::Type::verified);
                mStateIcon.setEnabled(hasWarning());
                mStateIcon.setTooltip(tooltip);
                mStateIcon.setVisible(!isProcessingOrRendering());
            }
            break;
            case AttrType::key:
            case AttrType::input:
            case AttrType::description:
            case AttrType::state:
            case AttrType::file:
            case AttrType::results:
            case AttrType::edit:
            case AttrType::graphics:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::unit:
            case AttrType::channelsLayout:
            case AttrType::showInGroup:
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

bool Track::StateButton::isProcessingOrRendering() const
{
    auto const state = mAccessor.getAttr<AttrType::processing>();
    return std::get<0>(state) || std::get<2>(state);
}

bool Track::StateButton::hasWarning() const
{
    return mAccessor.getAttr<AttrType::warnings>() != WarningType::none;
    ;
}

ANALYSE_FILE_END
