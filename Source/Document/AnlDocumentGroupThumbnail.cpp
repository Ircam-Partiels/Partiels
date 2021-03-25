#include "AnlDocumentGroupThumbnail.h"

ANALYSE_FILE_BEGIN

Document::GroupThumbnail::GroupThumbnail(Accessor& accessor)
: mAccessor(accessor)
{
    addAndMakeVisible(mPropertiesButton);
    addAndMakeVisible(mExportButton);
    addAndMakeVisible(mRemoveButton);
    addAndMakeVisible(mProcessingButton);
    
    mPropertiesButton.setTooltip(juce::translate("Change the analysis properties"));
    mExportButton.setTooltip(juce::translate("Export the analysis"));
    mRemoveButton.setTooltip(juce::translate("Remove the analysis"));
    
    mRemoveButton.onClick = [&]()
    {

    };
    
    mPropertiesButton.onClick = [&]()
    {
        
    };
    
    mExportButton.onClick = [&]()
    {
        // Force to repaint to update the state
        mExportButton.setState(juce::Button::ButtonState::buttonNormal);
    };
}

void Document::GroupThumbnail::resized()
{
    auto bounds = getLocalBounds().withTrimmedLeft(getWidth() / 2);
    auto constexpr separator = 2;
    auto const size = bounds.getWidth() - separator;
    mRemoveButton.setVisible(bounds.getHeight() >= size);
    mRemoveButton.setBounds(bounds.removeFromBottom(size).reduced(separator));
    mProcessingButton.setVisible(bounds.getHeight() >= size);
    mProcessingButton.setBounds(bounds.removeFromBottom(size).reduced(separator));
    mExportButton.setVisible(bounds.getHeight() >= size);
    mExportButton.setBounds(bounds.removeFromBottom(size).reduced(separator));
    mPropertiesButton.setVisible(bounds.getHeight() >= size);
    mPropertiesButton.setBounds(bounds.removeFromBottom(size).reduced(separator));
}

void Document::GroupThumbnail::paint(juce::Graphics& g)
{
    auto constexpr separator = 2;
    auto constexpr rotation = -1.5707963268f;
    
    auto const width = getWidth() / 2;
    auto const height = getHeight();
    auto const bottom = height - 2 * separator;
    auto const size = height - 4 * separator;
    
    g.setColour(findColour(ColourIds::textColourId));
    g.addTransform(juce::AffineTransform::rotation(rotation, 0.0f, static_cast<float>(bottom)));
    g.drawFittedText("Overview", 0, bottom, size, width, juce::Justification::centredLeft, 1, 1.0f);
}

void Document::GroupThumbnail::lookAndFeelChanged()
{
    auto* laf = dynamic_cast<IconManager::LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(laf != nullptr);
    if(laf != nullptr)
    {
        laf->setButtonIcon(mExportButton, IconManager::IconType::share);
        laf->setButtonIcon(mPropertiesButton, IconManager::IconType::properties);
        laf->setButtonIcon(mRemoveButton, IconManager::IconType::cancel);
    }
}

void Document::GroupThumbnail::parentHierarchyChanged()
{
    lookAndFeelChanged();
}

ANALYSE_FILE_END
