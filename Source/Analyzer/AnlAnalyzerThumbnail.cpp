#include "AnlAnalyzerThumbnail.h"

ANALYSE_FILE_BEGIN

Analyzer::Thumbnail::Thumbnail(Accessor& accessor)
: mAccessor(accessor)
{
    mNameLabel.setEditable(true);
    mNameLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(mNameLabel);
    addAndMakeVisible(mRemoveButton);
    addAndMakeVisible(mPropertiesButton);
    addAndMakeVisible(mRelaunchButton);
    
    mRemoveButton.onClick = [&]()
    {
        if(onRemove != nullptr)
        {
            onRemove();
        }
    };
    
    mListener.onChanged = [&](Accessor const& acsr, Attribute attribute)
    {
        if(attribute == Attribute::name)
        {
            mNameLabel.setText(acsr.getModel().name, juce::NotificationType::dontSendNotification);
        }
    };
    
    mReceiver.onSignal = [&](Accessor& acsr, Signal signal, juce::var value)
    {
        if(signal == Signal::pluginInstanceChanged)
        {
            auto instance = acsr.getInstance();
            if(instance != nullptr && mAccessor.getModel().name.isEmpty())
            {
                auto copy = mAccessor.getModel();
                copy.name = instance->getName();
                mAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
            }
        }
    };
    
    mAccessor.addListener(mListener, juce::NotificationType::sendNotificationSync);
    mAccessor.addReceiver(mReceiver);
    mReceiver.onSignal(mAccessor, Signal::pluginInstanceChanged, {});
}

Analyzer::Thumbnail::~Thumbnail()
{
    mAccessor.removeReceiver(mReceiver);
    mAccessor.removeListener(mListener);
}

void Analyzer::Thumbnail::resized()
{
    auto bounds = getLocalBounds();
    auto bottomBounds = bounds.removeFromBottom(24);
    auto const bottomButtonWidth = bottomBounds.getWidth() / 3;
    mRemoveButton.setBounds(bottomBounds.removeFromLeft(bottomButtonWidth).reduced(2));
    mPropertiesButton.setBounds(bottomBounds.removeFromLeft(bottomButtonWidth).reduced(2));
    mRelaunchButton.setBounds(bottomBounds.removeFromLeft(bottomButtonWidth).reduced(2));
    mNameLabel.setBounds(bounds);
}

ANALYSE_FILE_END
