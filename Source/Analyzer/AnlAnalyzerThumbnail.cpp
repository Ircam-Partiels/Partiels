#include "AnlAnalyzerThumbnail.h"

ANALYSE_FILE_BEGIN

Analyzer::Thumbnail::Thumbnail(Accessor& accessor)
: mAccessor(accessor)
{
    auto setupImage = [](juce::ImageButton& button, juce::Image image)
    {
        button.setImages(true, true, true, image, 1.0f, juce::Colours::grey, image, 0.8f, juce::Colours::grey.brighter(), image, 0.8f, juce::Colours::grey.brighter());
    };
    
    addAndMakeVisible(mRemoveButton);
    addAndMakeVisible(mPropertiesButton);
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
    setupImage(mPropertiesButton, juce::ImageCache::getFromMemory(BinaryData::reglages_png, BinaryData::reglages_pngSize));
    
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::name:
            {
                repaint();
            }
                break;
            case AttrType::key:
            case AttrType::description:
            case AttrType::state:
            case AttrType::zoomMode:
            case AttrType::colour:
            case AttrType::colourMap:
            case AttrType::results:
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
    bounds.removeFromBottom(width);
    mRemoveButton.setBounds(bounds.removeFromBottom(width).reduced(separator));
    mPropertiesButton.setBounds(bounds.removeFromBottom(width).reduced(separator));
}

void Analyzer::Thumbnail::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
    
    auto constexpr separator = 2;
    auto constexpr rotation = -1.5707963268f;
    
    auto const width = getWidth();
    auto const height = getHeight();
    auto const font = g.getCurrentFont();
    auto const size = (height - separator) - 3 * width;
    
    g.setColour(juce::Colours::grey);
    g.fillRoundedRectangle(getLocalBounds().removeFromBottom(width - separator).reduced(separator).toFloat(), 4.0f);
    g.setColour(findColour(ColourIds::textColourId));
    
    auto const title = mAccessor.getAttr<AttrType::name>();
    g.addTransform(juce::AffineTransform::rotation(rotation, 0.0f, size));
    g.drawFittedText(mAccessor.getAttr<AttrType::name>(), 0, size, size, width, juce::Justification::centredLeft, 1, 1.0f);
}

ANALYSE_FILE_END
