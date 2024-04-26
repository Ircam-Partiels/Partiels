#include "AnlGroupPropertyOscSection.h"
#include "../Track/AnlTrackTools.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

Group::PropertyOscSection::PropertyOscSection(Director& director)
: mDirector(director)
, mPropertySendViaOsc(juce::translate("Send the results via OSC"), juce::translate("Toggle tracks' results to be sent via OSC."), [this](bool state)
                      {
                          mDirector.startAction(true);
                          for(auto& trackAcsr : Tools::getTrackAcsrs(mAccessor))
                          {
                              trackAcsr.get().setAttr<Track::AttrType::sendViaOsc>(state, NotificationType::synchronous);
                          }
                          mDirector.endAction(true, ActionState::newTransaction, juce::translate("Toggle tracks' results to be sent via OSC"));
                      })
, mLayoutNotifier(mAccessor, [this]()
                  {
                      updateContent();
                  },
                  {Track::AttrType::identifier, Track::AttrType::sendViaOsc})
{
    addAndMakeVisible(mPropertySendViaOsc);
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
    setBounds(mPropertySendViaOsc);
    setSize(getWidth(), bounds.getY());
}

void Group::PropertyOscSection::updateContent()
{
    juce::StringArray trackNames;
    std::set<bool> sentViaOsc;
    for(auto const& trackAcsr : Tools::getTrackAcsrs(mAccessor))
    {
        trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        sentViaOsc.insert(trackAcsr.get().getAttr<Track::AttrType::sendViaOsc>());
    }
    mPropertySendViaOsc.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("Toggle tracks' results to be sent via OSC."));
    mPropertySendViaOsc.setVisible(!sentViaOsc.empty());
    if(!sentViaOsc.empty())
    {
        mPropertySendViaOsc.entry.setToggleState(*sentViaOsc.cbegin(), juce::NotificationType::dontSendNotification);
    }
    mPropertySendViaOsc.entry.setAlpha(sentViaOsc.size() > 1_z ? 0.5f : 1.0f);
    resized();
}

ANALYSE_FILE_END
