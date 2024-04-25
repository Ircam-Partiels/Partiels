#include "AnlTrackPropertyOscSection.h"

ANALYSE_FILE_BEGIN

Track::PropertyOscSection::PropertyOscSection(Director& director)
: mDirector(director)
, mPropertyIdentifier(juce::translate("Identifier"), juce::translate("The OSC identifier of the track"), [this]()
                      {
                          juce::SystemClipboard::copyTextToClipboard(mAccessor.getAttr<AttrType::identifier>());
                          mTooltip = std::make_unique<juce::TooltipWindow>(nullptr);
                          if(mTooltip != nullptr)
                          {
                              mTooltip->displayTip(juce::Desktop::getMousePosition(), juce::translate("OSC identifier copied to clipboard"));
                              startTimer(500);
                          }
                      })
, mPropertySendViaOsc(juce::translate("Send the results via OSC"), juce::translate("Toggle track results to be sent via OSC."), [&](bool state)
                      {
                          mDirector.startAction();
                          mAccessor.setAttr<AttrType::sendViaOsc>(state, NotificationType::synchronous);
                          mDirector.endAction(ActionState::newTransaction, juce::translate("Toggle track results to be sent via OSC"));
                      })
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            {
                mPropertyIdentifier.entry.setButtonText(acsr.getAttr<AttrType::identifier>());
                break;
            }
            case AttrType::sendViaOsc:
            {
                mPropertySendViaOsc.entry.setToggleState(acsr.getAttr<AttrType::sendViaOsc>(), juce::NotificationType::dontSendNotification);
                break;
            }
            case AttrType::description:
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
            case AttrType::labelLayout:
            case AttrType::channelsLayout:
            case AttrType::showInGroup:
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

    addAndMakeVisible(mPropertyIdentifier);
    addAndMakeVisible(mPropertySendViaOsc);
    setSize(300, 200);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::PropertyOscSection::~PropertyOscSection()
{
    mAccessor.removeListener(mListener);
}

void Track::PropertyOscSection::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto const setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyIdentifier);
    setBounds(mPropertySendViaOsc);
    setSize(getWidth(), bounds.getY());
}

void Track::PropertyOscSection::timerCallback()
{
    mTooltip.reset();
    stopTimer();
}

ANALYSE_FILE_END
