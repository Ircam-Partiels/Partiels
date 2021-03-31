#include "AnlTrackStateButton.h"

ANALYSE_FILE_BEGIN

Track::StateButton::StateButton(Accessor& accessor)
: mAccessor(accessor)
{
    addAndMakeVisible(mProcessingButton);
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::processing:
            case AttrType::warnings:
            {
                auto const state = acsr.getAttr<AttrType::processing>();
                auto const warnings = acsr.getAttr<AttrType::warnings>();
                auto getInactiveIconType = [warnings]()
                {
                    return warnings == WarningType::none ? IconManager::IconType::checked : IconManager::IconType::alert;
                };
                auto getTooltip = [&, state, warnings]() -> juce::String
                {
                    if(std::get<0>(state))
                    {
                        return "Analysing... (" + juce::String(static_cast<int>(std::round(std::get<1>(state) * 100.f))) + "%)";
                    }
                    else if(std::get<2>(state))
                    {
                        return "Rendering... (" + juce::String(static_cast<int>(std::round(std::get<3>(state) * 100.f))) + "%)";
                    }
                    switch(warnings)
                    {
                        case WarningType::none:
                            return "Analysis and rendering successfully completed!";
                        case WarningType::plugin:
                            return "Analysis failed: The plugin cannot be found or allocated!";
                        case WarningType::state:
                            return "Analysis failed: The step size or the block size might not be supported!";
                    }
                    return "Analysis and rendering successfully completed!";
                };
                mProcessingButton.setInactiveImage(IconManager::getIcon(getInactiveIconType()));
                auto const name = acsr.getAttr<AttrType::name>();
                mProcessingButton.setTooltip(name + ": " + juce::translate(getTooltip()));
                mProcessingButton.setActive(std::get<0>(state) || std::get<2>(state));
            }
                break;
            case AttrType::name:
            case AttrType::key:
            case AttrType::description:
            case AttrType::state:
            case AttrType::results:
            case AttrType::graphics:
            case AttrType::time:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::colours:
            case AttrType::propertyState:
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
