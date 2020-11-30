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
    
    mNameLabel.onTextChange = [&]()
    {
        mAccessor.setAttr<AttrType::name>(mNameLabel.getText(), NotificationType::synchronous);
    };
    
    mRemoveButton.onClick = [&]()
    {
        if(onRemove != nullptr)
        {
            onRemove();
        }
    };
    mRemoveButton.setTooltip(juce::translate("Remove analysis"));
    
    mPropertiesButton.onClick = [&]()
    {
        juce::DialogWindow::LaunchOptions launchOption;
        launchOption.dialogTitle = juce::translate("ANLNAME Properties").replace("ANLNAME", mAccessor.getAttr<AttrType::name>());
        launchOption.content.setNonOwned(&mPropertyPanel);
        launchOption.componentToCentreAround = nullptr;
        launchOption.dialogBackgroundColour = findColour(juce::ResizableWindow::backgroundColourId, true);
        launchOption.escapeKeyTriggersCloseButton = true;
        launchOption.useNativeTitleBar = false;
        launchOption.resizable = false;
        launchOption.useBottomRightCornerResizer = false;
        launchOption.runModal();
    };
    mPropertiesButton.setTooltip(juce::translate("Analysis properties"));
    
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::name:
                mNameLabel.setText(acsr.getAttr<AttrType::name>(), juce::NotificationType::dontSendNotification);
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
    mNameLabel.setBounds(bounds.removeFromTop(24));
    auto bottomBounds = bounds.removeFromBottom(24);
    auto const bottomButtonWidth = bottomBounds.getWidth() / 3;
    mRemoveButton.setBounds(bottomBounds.removeFromLeft(bottomButtonWidth).reduced(2));
    mPropertiesButton.setBounds(bottomBounds.removeFromLeft(bottomButtonWidth).reduced(2));
}

void Analyzer::Thumbnail::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
}

ANALYSE_FILE_END
