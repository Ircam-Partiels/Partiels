#include "AnlAnalyzerThumbnail.h"

ANALYSE_FILE_BEGIN

Analyzer::Thumbnail::Thumbnail(Accessor& accessor)
: mAccessor(accessor)
{
    mNameLabel.setEditable(true);
    mNameLabel.setJustificationType(juce::Justification::topLeft);
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
    
    mPropertiesButton.onClick = [&]()
    {
        juce::DialogWindow::LaunchOptions launchOption;
        launchOption.dialogTitle = juce::translate("Analyzer Properties");
        launchOption.content.setNonOwned(&mPropertyPanel);
        launchOption.componentToCentreAround = this;
        launchOption.dialogBackgroundColour = findColour(juce::ResizableWindow::backgroundColourId, true);
        launchOption.escapeKeyTriggersCloseButton = true;
        launchOption.useNativeTitleBar = false;
        launchOption.resizable = false;
        launchOption.useBottomRightCornerResizer = false;
        auto dialogWindow = launchOption.launchAsync();
        dialogWindow->runModalLoop();
    };
    
    mListener.onChanged = [&](Accessor const& acsr, Attribute attribute)
    {
        if(attribute == Attribute::name)
        {
            mNameLabel.setText(acsr.getModel().name, juce::NotificationType::dontSendNotification);
        }
    };
    
    mReceiver.onSignal = [&](Accessor const& acsr, Signal signal, juce::var value)
    {
        if(signal == Signal::pluginInstanceChanged)
        {
            auto instance = acsr.getInstance();
            if(instance != nullptr && mAccessor.getModel().name.isEmpty())
            {

            }
        }
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
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
