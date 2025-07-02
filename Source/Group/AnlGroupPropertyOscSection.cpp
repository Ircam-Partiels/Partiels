#include "AnlGroupPropertyOscSection.h"
#include "../Track/AnlTrackTools.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

Group::PropertyOscSection::PropertyOscSection(Director& director)
: mDirector(director)
, mPropertyTrackOsc(juce::translate("Track OSC"), juce::translate("The OSC state of the tracks of the group."), [this]()
                    {
                        showTrackOsc();
                    })
{
    addAndMakeVisible(mPropertyTrackOsc);
}

void Group::PropertyOscSection::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto const setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyTrackOsc);
    setSize(getWidth(), bounds.getY());
}

void Group::PropertyOscSection::showTrackOsc()
{
    juce::PopupMenu menu;
    Tools::fillMenuForTrackOsc(mAccessor, menu, nullptr, [this]()
                               {
                                   showTrackOsc();
                               });
    if(!std::exchange(mTrackOscActionStarted, true))
    {
        mDirector.startAction(true);
    }
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mPropertyTrackOsc.entry).withDeletionCheck(*this), [=, this](int menuResult)
                       {
                           if(menuResult == 0 && std::exchange(mTrackOscActionStarted, false))
                           {
                               mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change the OSC state of the tracks of the group"));
                           }
                       });
}

ANALYSE_FILE_END
