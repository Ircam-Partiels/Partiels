#include "AnlDocumentGroupThumbnail.h"

ANALYSE_FILE_BEGIN

Document::GroupThumbnail::GroupThumbnail(Accessor& accessor)
: mAccessor(accessor)
{
    addAndMakeVisible(mExportButton);
    addAndMakeVisible(mGroupStateButton);
    addAndMakeVisible(mExpandButton);
    
    mExportButton.setTooltip(juce::translate("Export the analyses"));
    
    mExpandButton.onClick = [&]()
    {
        mAccessor.setAttr<AttrType::expanded>(!mAccessor.getAttr<AttrType::expanded>(), NotificationType::synchronous);
    };
    
    mExportButton.onClick = [&]()
    {
        // Force to repaint to update the state
        mExportButton.setState(juce::Button::ButtonState::buttonNormal);
    };
    
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType const attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::file:
            case AttrType::layoutHorizontal:
            case AttrType::layoutVertical:
            case AttrType::layout:
                break;
            case AttrType::expanded:
            {
                lookAndFeelChanged();
            }
                break;
        }
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::GroupThumbnail::~GroupThumbnail()
{
    mAccessor.removeListener(mListener);
}

void Document::GroupThumbnail::resized()
{
    auto bounds = getLocalBounds().withTrimmedLeft(getWidth() / 2);
    auto constexpr separator = 2;
    auto const size = bounds.getWidth() - separator;
    mExpandButton.setVisible(bounds.getHeight() >= size);
    mExpandButton.setBounds(bounds.removeFromBottom(size).reduced(separator));
    mGroupStateButton.setVisible(bounds.getHeight() >= size);
    mGroupStateButton.setBounds(bounds.removeFromBottom(size).reduced(separator));
    mExportButton.setVisible(bounds.getHeight() >= size);
    mExportButton.setBounds(bounds.removeFromBottom(size).reduced(separator));
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
        if(mAccessor.getAttr<AttrType::expanded>())
        {
            laf->setButtonIcon(mExpandButton, IconManager::IconType::shrink);
            mExpandButton.setTooltip(juce::translate("Shrink the analyses"));
        }
        else
        {
            laf->setButtonIcon(mExpandButton, IconManager::IconType::expand);
            mExpandButton.setTooltip(juce::translate("Expand the analyses"));
        }
    }
}

void Document::GroupThumbnail::parentHierarchyChanged()
{
    lookAndFeelChanged();
}

ANALYSE_FILE_END
