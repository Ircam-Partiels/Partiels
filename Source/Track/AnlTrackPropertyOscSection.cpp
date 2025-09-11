#include "AnlTrackPropertyOscSection.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::PropertyOscSection::PropertyOscSection(Director& director)
: mDirector(director)
, mPropertyIdentifier(juce::translate("Identifier"), juce::translate("The OSC identifier of the track"), [this](juce::String text)
                      {
                          mDirector.startAction();
                          mAccessor.setAttr<AttrType::oscIdentifier>(text, NotificationType::synchronous);
                          mDirector.endAction(ActionState::newTransaction, juce::translate("Set track OSC identifier"));
                      })
, mCopyButton(juce::translate("Copy"))
, mPropertySendViaOsc(juce::translate("Send the results via OSC"), juce::translate("Toggle track results to be sent via OSC"), [&](bool state)
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
            case AttrType::oscIdentifier:
            {
                mPropertyIdentifier.entry.setText(Tools::getEffectiveOscIdentifier(acsr), juce::NotificationType::dontSendNotification);
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
            case AttrType::lineWidth:
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
            case AttrType::sampleRate:
            case AttrType::zoomLogScale:
                break;
        }
    };

    mCopyButton.setSize(60, 24);
    mCopyButton.onClick = [this]()
    {
        juce::SystemClipboard::copyTextToClipboard(Tools::getEffectiveOscIdentifier(mAccessor));
        mTooltip = std::make_unique<juce::TooltipWindow>(nullptr);
        if(mTooltip != nullptr)
        {
            mTooltip->displayTip(juce::Desktop::getMousePosition(), juce::translate("OSC identifier copied to clipboard"));
            startTimer(500);
        }
    };

    addAndMakeVisible(mPropertyIdentifier);
    addAndMakeVisible(mCopyButton);
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
    
    // Set up the identifier property with copy button next to it
    if(mPropertyIdentifier.isVisible())
    {
        auto identifierBounds = bounds.removeFromTop(mPropertyIdentifier.getHeight());
        auto copyButtonBounds = identifierBounds.removeFromRight(mCopyButton.getWidth() + 4);
        mPropertyIdentifier.setBounds(identifierBounds);
        mCopyButton.setBounds(copyButtonBounds.withSizeKeepingCentre(mCopyButton.getWidth(), mCopyButton.getHeight()));
    }
    
    setBounds(mPropertySendViaOsc);
    setSize(getWidth(), bounds.getY());
}

void Track::PropertyOscSection::timerCallback()
{
    mTooltip.reset();
    stopTimer();
}

ANALYSE_FILE_END
