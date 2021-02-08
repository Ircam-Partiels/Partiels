#include "AnlAnalyzerThumbnail.h"

ANALYSE_FILE_BEGIN

Analyzer::Thumbnail::Thumbnail(Accessor& accessor)
: mAccessor(accessor)
{
    auto setupImage = [](juce::ImageButton& button, juce::Image image)
    {
        JUCE_COMPILER_WARNING("use a global approach");
        button.setImages(true, true, true, image, 1.0f, juce::Colours::grey, image, 0.8f, juce::Colours::grey.brighter(), image, 0.8f, juce::Colours::grey.brighter());
    };
    
    addAndMakeVisible(mPropertiesButton);
    addAndMakeVisible(mEffectButton);
    addAndMakeVisible(mRemoveButton);
    addAndMakeVisible(mProcessingButton);
    
    mRemoveButton.onClick = [&]()
    {
        if(onRemove != nullptr)
        {
            onRemove();
        }
    };
    mRemoveButton.setTooltip(juce::translate("Remove analysis"));
    setupImage(mRemoveButton, juce::ImageCache::getFromMemory(BinaryData::annuler_png, BinaryData::annuler_pngSize));
    
    mPropertiesButton.onClick = [&]()
    {
        mPropertyPanel.show();
    };
    mPropertiesButton.setTooltip(juce::translate("Analysis properties"));
    setupImage(mPropertiesButton, juce::ImageCache::getFromMemory(BinaryData::reglages_png, BinaryData::reglages_pngSize));
    
    mEffectButton.onClick = [&]()
    {
    };
    mEffectButton.setTooltip(juce::translate("Effects"));
    setupImage(mEffectButton, juce::ImageCache::getFromMemory(BinaryData::star_png, BinaryData::star_pngSize));
    
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::name:
            {
                repaint();
            }
                break;
            case AttrType::processing:
            {
                mProcessingButton.setActive(acsr.getAttr<AttrType::processing>());
            }
                break;
            case AttrType::key:
            case AttrType::description:
            case AttrType::state:
            case AttrType::results:
            case AttrType::warnings:
            case AttrType::time:
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
    auto constexpr separator = 2;
    auto const width = bounds.getWidth() - separator;
    mPropertiesButton.setBounds(bounds.removeFromTop(width).reduced(separator));
    mEffectButton.setBounds(bounds.removeFromTop(width).reduced(separator));
    mRemoveButton.setBounds(bounds.removeFromTop(width).reduced(separator));
    mProcessingButton.setBounds(bounds.removeFromTop(width).reduced(separator));
}

void Analyzer::Thumbnail::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
    
    g.setColour(findColour(ColourIds::borderColourId));
    g.drawRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 4.0f, 1.0f);
    
    auto constexpr separator = 2;
    auto constexpr rotation = -1.5707963268f;
    
    auto const width = getWidth();
    auto const height = getHeight();
    auto const bottom = height - 2 * separator;
    auto const size = bottom - 4 * width;
    
    g.setColour(findColour(ColourIds::textColourId));
    g.addTransform(juce::AffineTransform::rotation(rotation, 0.0f, static_cast<float>(bottom)));
    g.drawFittedText(mAccessor.getAttr<AttrType::name>(), 0, bottom, size, width, juce::Justification::centredLeft, 1, 1.0f);
}

ANALYSE_FILE_END
