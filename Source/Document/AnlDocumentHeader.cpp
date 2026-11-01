#include "AnlDocumentHeader.h"
#include "AnlDocumentAudioReader.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

Document::Header::Header(Director& director, juce::ApplicationCommandManager& commandManager)
: mDirector(director)
, mApplicationCommandManager(commandManager)
, mDocumentFileButton(ScalableImage::fromSvg(AnlIconsData::DocumentFile_svg, AnlIconsData::DocumentFile_svgSize))
, mReaderLayoutButton(ScalableImage::fromSvg(AnlIconsData::AudioLayer_svg, AnlIconsData::AudioLayer_svgSize))
, mTransportDisplay(mAccessor.getAcsr<AcsrType::transport>(), mAccessor.getAcsr<AcsrType::timeZoom>(), commandManager)
, mTransportSelectionInfo(mAccessor.getAcsr<AcsrType::transport>(), mAccessor.getAcsr<AcsrType::timeZoom>())
, mOscButton(juce::translate("OSC"), juce::translate("Show OSC Settings"))
, mBubbleTooltipButton(ScalableImage::fromSvg(AnlIconsData::Bubble_svg, AnlIconsData::Bubble_svgSize))
, mEditModeButton(ScalableImage::fromSvg(AnlIconsData::Edit_svg, AnlIconsData::Edit_svgSize))
{
    addAndMakeVisible(mReaderLayoutButton);
    addAndMakeVisible(mDocumentFileButton);
    addAndMakeVisible(mTransportDisplay);
    addAndMakeVisible(mTransportSelectionInfo);
    addAndMakeVisible(mOscButton);
    addAndMakeVisible(mBubbleTooltipButton);
    addAndMakeVisible(mEditModeButton);

    mDocumentFileButton.setWantsKeyboardFocus(false);
    mDocumentFileButton.setPadding(3);
    mDocumentFileButton.onClick = [this]()
    {
        mAccessor.sendSignal(SignalType::showDocumentFilePanel, {}, NotificationType::synchronous);
    };
    mDocumentFileButton.setTooltip(juce::translate("Show document file panel"));

    mReaderLayoutButton.setWantsKeyboardFocus(false);
    mReaderLayoutButton.setPadding(3);
    mReaderLayoutButton.onClick = [this]()
    {
        mAccessor.sendSignal(SignalType::showReaderLayoutPanel, {}, NotificationType::synchronous);
    };

    mOscButton.setCommandToTrigger(&mApplicationCommandManager, ApplicationCommandIDs::helpOpenOscSettings, true);

    mBubbleTooltipButton.setClickingTogglesState(true);
    mBubbleTooltipButton.setPadding(2);
    mBubbleTooltipButton.onClick = [this]()
    {
        mApplicationCommandManager.invokeDirectly(ApplicationCommandIDs::viewInfoBubble, true);
    };

    mEditModeButton.setClickingTogglesState(true);
    mEditModeButton.setPadding(2);
    mEditModeButton.onClick = [this]()
    {
        mApplicationCommandManager.invokeDirectly(ApplicationCommandIDs::frameToggleDrawing, true);
    };

    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::reader:
            {
                auto const result = createAudioFormatReader(acsr, mDirector.getAudioFormatManager());
                auto const alertMessage = std::get<1>(result).joinIntoString("");
                if(alertMessage.isEmpty())
                {
                    mReaderLayoutButton.setImages(ScalableImage::fromSvg(AnlIconsData::AudioLayer_svg, AnlIconsData::AudioLayer_svgSize));
                    mReaderLayoutButton.setTooltip(juce::translate("Show audio files layout panel"));
                }
                else
                {
                    mReaderLayoutButton.setImages(ScalableImage::fromSvg(AnlIconsData::Alert_svg, AnlIconsData::Alert_svgSize));
                    mReaderLayoutButton.setTooltip(alertMessage);
                }
            }
            break;
            case AttrType::path:
            case AttrType::grid:
            case AttrType::autoresize:
            case AttrType::layout:
            case AttrType::viewport:
            case AttrType::samplerate:
            case AttrType::channels:
            case AttrType::editMode:
            case AttrType::drawingState:
            case AttrType::description:
                break;
        }
    };

    mApplicationCommandManager.addListener(this);
    applicationCommandListChanged();
    mAccessor.addListener(mListener, NotificationType::synchronous);
    setSize(200, 48);
}

Document::Header::~Header()
{
    mAccessor.removeListener(mListener);
    mApplicationCommandManager.removeListener(this);
}

void Document::Header::resized()
{
    auto bounds = getLocalBounds();
    static auto constexpr spaceWidth = 4;
    static auto constexpr buttonHeight = 30;
    static auto constexpr transportDisplayWidth = 284;
    static auto constexpr rightButtonSize = 20;
    static auto constexpr rightButtonSpace = 4;
    auto const sideWidth = std::max((bounds.getWidth() - transportDisplayWidth) / 2, 0);

    auto leftPart = bounds.removeFromLeft(sideWidth).withSizeKeepingCentre(sideWidth - spaceWidth * 2, buttonHeight);
    mDocumentFileButton.setVisible(leftPart.getWidth() >= buttonHeight);
    if(mDocumentFileButton.isVisible())
    {
        mDocumentFileButton.setBounds(leftPart.removeFromLeft(buttonHeight));
        leftPart = leftPart.withTrimmedLeft(spaceWidth);
    }
    mReaderLayoutButton.setVisible(leftPart.getWidth() >= buttonHeight);
    if(mReaderLayoutButton.isVisible())
    {
        mReaderLayoutButton.setBounds(leftPart.removeFromLeft(buttonHeight));
        leftPart = leftPart.withTrimmedLeft(spaceWidth);
    }

    auto rightPart = bounds.removeFromRight(sideWidth);
    auto const scrollbarWidth = getLookAndFeel().getDefaultScrollbarWidth();
    auto const hasRightButtons = rightPart.getWidth() >= 24 + scrollbarWidth + spaceWidth;
    mBubbleTooltipButton.setVisible(hasRightButtons);
    mEditModeButton.setVisible(hasRightButtons);
    if(hasRightButtons)
    {
        // The buttons are aligned with the magnetize button of the document section (same column, size and spacing)
        auto rightButtonBounds = rightPart.removeFromRight(24 + scrollbarWidth);
        mEditModeButton.setBounds(rightButtonBounds.removeFromBottom(rightButtonSize).withSizeKeepingCentre(rightButtonSize, rightButtonSize));
        rightButtonBounds.removeFromBottom(rightButtonSpace);
        mBubbleTooltipButton.setBounds(rightButtonBounds.removeFromBottom(rightButtonSize).withSizeKeepingCentre(rightButtonSize, rightButtonSize));
    }
    rightPart = rightPart.reduced(spaceWidth, 0);
    mTransportSelectionInfo.setVisible(rightPart.getWidth() >= 164);
    if(mTransportSelectionInfo.isVisible())
    {
        mTransportSelectionInfo.setBounds(rightPart.removeFromRight(164));
    }
    mOscButton.setVisible(rightPart.getWidth() > 44);
    mOscButton.setBounds(rightPart.removeFromLeft(44).removeFromBottom(22).withSizeKeepingCentre(40, 16));

    mTransportDisplay.setVisible(bounds.getWidth() >= transportDisplayWidth);
    if(mTransportDisplay.isVisible())
    {
        bounds.removeFromLeft((bounds.getWidth() - transportDisplayWidth) / 2);
        mTransportDisplay.setBounds(bounds.removeFromLeft(transportDisplayWidth).withSizeKeepingCentre(transportDisplayWidth, 40));
    }
}

void Document::Header::colourChanged()
{
    Utils::notifyListener(mApplicationCommandManager, *this, {ApplicationCommandIDs::transportOscConnected});
}

void Document::Header::applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info)
{
    switch(info.commandID)
    {
        case ApplicationCommandIDs::viewInfoBubble:
            mBubbleTooltipButton.setToggleState(info.commandFlags & juce::ApplicationCommandInfo::CommandFlags::isTicked, juce::NotificationType::dontSendNotification);
            break;
        case ApplicationCommandIDs::frameToggleDrawing:
            mEditModeButton.setToggleState(info.commandFlags & juce::ApplicationCommandInfo::CommandFlags::isTicked, juce::NotificationType::dontSendNotification);
            break;
        case ApplicationCommandIDs::transportOscConnected:
            mOscButton.setColour(juce::TextButton::ColourIds::buttonColourId, info.commandFlags & juce::ApplicationCommandInfo::CommandFlags::isTicked ? juce::Colours::green : findColour(juce::TextButton::ColourIds::buttonColourId));
            break;
        default:
            break;
    }
}

void Document::Header::applicationCommandListChanged()
{
    mBubbleTooltipButton.setTooltip(Utils::getCommandDescriptionWithKey(mApplicationCommandManager, ApplicationCommandIDs::viewInfoBubble));
    mEditModeButton.setTooltip(Utils::getCommandDescriptionWithKey(mApplicationCommandManager, ApplicationCommandIDs::frameToggleDrawing));
    Utils::notifyListener(mApplicationCommandManager, *this, {ApplicationCommandIDs::viewInfoBubble, ApplicationCommandIDs::frameToggleDrawing, ApplicationCommandIDs::transportOscConnected});
}

ANALYSE_FILE_END
