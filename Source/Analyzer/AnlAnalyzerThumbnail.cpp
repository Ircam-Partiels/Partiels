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
    
    mNameLabel.onTextChange = [&]()
    {
        mAccessor.setValue<AttrType::name>(mNameLabel.getText(), NotificationType::synchronous);
    };
    
    mRemoveButton.onClick = [&]()
    {
        if(onRemove != nullptr)
        {
            onRemove();
        }
    };
    
    mRelaunchButton.onClick= [&]()
    {
        if(onRelaunch != nullptr)
        {
            onRelaunch();
        }
    };
    
    mPropertiesButton.onClick = [&]()
    {
        juce::DialogWindow::LaunchOptions launchOption;
        launchOption.dialogTitle = juce::translate("ANLNAME Properties").replace("ANLNAME", mAccessor.getValue<AttrType::name>());
        launchOption.content.setNonOwned(&mPropertyPanel);
        launchOption.componentToCentreAround = this;
        launchOption.dialogBackgroundColour = findColour(juce::ResizableWindow::backgroundColourId, true);
        launchOption.escapeKeyTriggersCloseButton = true;
        launchOption.useNativeTitleBar = false;
        launchOption.resizable = false;
        launchOption.useBottomRightCornerResizer = false;
        launchOption.runModal();
    };
    
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::name:
                mNameLabel.setText(acsr.getValue<AttrType::name>(), juce::NotificationType::dontSendNotification);
                break;
            case AttrType::key:
            case AttrType::parameters:
                break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Analyzer::Thumbnail::~Thumbnail()
{
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
