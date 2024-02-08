#include "AnlDocumentHeader.h"
#include "AnlDocumentAudioReader.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

Document::Header::Header(Director& director, juce::ApplicationCommandManager& commandManager)
: mDirector(director)
, mApplicationCommandManager(commandManager)
, mReaderLayoutButton(juce::ImageCache::getFromMemory(AnlIconsData::audiolayer_png, AnlIconsData::audiolayer_pngSize))
, mNameButton(juce::translate("Untitled"), juce::translate("Document not saved"))
, mTransportDisplay(mAccessor.getAcsr<AcsrType::transport>(), mAccessor.getAcsr<AcsrType::timeZoom>(), commandManager)
, mTransportSelectionInfo(mAccessor.getAcsr<AcsrType::transport>(), mAccessor.getAcsr<AcsrType::timeZoom>())
, mBubbleTooltipButton(juce::ImageCache::getFromMemory(AnlIconsData::bubble_png, AnlIconsData::bubble_pngSize))
, mEditModeButton(juce::ImageCache::getFromMemory(AnlIconsData::edit_png, AnlIconsData::edit_pngSize))
{
    addAndMakeVisible(mReaderLayoutButton);
    addAndMakeVisible(mNameButton);
    addAndMakeVisible(mTransportDisplay);
    addAndMakeVisible(mTransportSelectionInfo);
    addAndMakeVisible(mBubbleTooltipButton);
    addAndMakeVisible(mEditModeButton);

    mReaderLayoutButton.setWantsKeyboardFocus(false);
    mReaderLayoutButton.onClick = [this]()
    {
        mAccessor.sendSignal(SignalType::showReaderLayoutPanel, {}, NotificationType::synchronous);
    };

    mNameButton.onClick = [this]()
    {
        auto const file = mAccessor.getAttr<AttrType::path>();
        if(file != juce::File{})
        {
            mAccessor.getAttr<AttrType::path>().revealToUser();
        }
        else
        {
            mApplicationCommandManager.invokeDirectly(ApplicationCommandIDs::documentSave, true);
        }
    };

    mBubbleTooltipButton.setClickingTogglesState(true);
    mBubbleTooltipButton.onClick = [this]()
    {
        mApplicationCommandManager.invokeDirectly(ApplicationCommandIDs::viewInfoBubble, true);
    };

    mEditModeButton.setClickingTogglesState(true);
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
                    mReaderLayoutButton.setImages(juce::ImageCache::getFromMemory(AnlIconsData::audiolayer_png, AnlIconsData::audiolayer_pngSize));
                    mReaderLayoutButton.setTooltip(juce::translate("Show audio files layout panel"));
                }
                else
                {
                    mReaderLayoutButton.setImages(juce::ImageCache::getFromMemory(AnlIconsData::alert_png, AnlIconsData::alert_pngSize));
                    mReaderLayoutButton.setTooltip(alertMessage);
                }
            }
            break;
            case AttrType::path:
            {
                auto const file = acsr.getAttr<AttrType::path>();
                mNameButton.setButtonText(file != juce::File{} ? file.getFileName() : juce::translate("Untitled"));
                mNameButton.setTooltip(file != juce::File{} ? file.getFullPathName() : juce::translate("Document not saved"));
                resized();
            }
            break;
            case AttrType::grid:
            case AttrType::autoresize:
            case AttrType::layout:
            case AttrType::viewport:
            case AttrType::samplerate:
            case AttrType::channels:
            case AttrType::editMode:
            case AttrType::drawingState:
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
    static auto constexpr buttonHeight = 24;
    static auto constexpr transportDisplayWidth = 284;
    auto const sideWidth = std::max((bounds.getWidth() - transportDisplayWidth) / 2, 0);

    auto leftPart = bounds.removeFromLeft(sideWidth).withSizeKeepingCentre(sideWidth - spaceWidth * 2, buttonHeight);
    mReaderLayoutButton.setVisible(leftPart.getWidth() >= 24);
    if(mReaderLayoutButton.isVisible())
    {
        mReaderLayoutButton.setBounds(leftPart.removeFromLeft(24));
        leftPart = leftPart.withTrimmedLeft(spaceWidth);
    }

    auto const font = mNameButton.getLookAndFeel().getTextButtonFont(mNameButton, 24);
    auto const textWidth = font.getStringWidth(mNameButton.getButtonText()) + static_cast<int>(std::ceil(font.getHeight()) * 2.0f);
    mNameButton.setVisible(leftPart.getWidth() >= textWidth);
    if(mNameButton.isVisible())
    {
        mNameButton.setBounds(leftPart.removeFromLeft(textWidth));
        leftPart = leftPart.withTrimmedLeft(spaceWidth);
    }

    auto rightPart = bounds.removeFromRight(sideWidth).reduced(spaceWidth, 0);
    auto const scrollbarWidth = getLookAndFeel().getDefaultScrollbarWidth();
    auto const hasRightButtons = rightPart.getWidth() >= 24 + scrollbarWidth;
    mBubbleTooltipButton.setVisible(hasRightButtons);
    mEditModeButton.setVisible(hasRightButtons);
    if(hasRightButtons)
    {
        auto rightButtonBounds = rightPart.removeFromRight(24 + scrollbarWidth).reduced((2 + scrollbarWidth) / 2, 4);
        mBubbleTooltipButton.setBounds(rightButtonBounds.removeFromTop(rightButtonBounds.getHeight() / 2).withSizeKeepingCentre(20, 20));
        mEditModeButton.setBounds(rightButtonBounds.withSizeKeepingCentre(20, 20));
    }
    mTransportSelectionInfo.setVisible(rightPart.getWidth() >= 164);
    if(mTransportSelectionInfo.isVisible())
    {
        mTransportSelectionInfo.setBounds(rightPart.removeFromRight(164));
    }

    mTransportDisplay.setVisible(bounds.getWidth() >= transportDisplayWidth);
    mTransportDisplay.setBounds(bounds.withSizeKeepingCentre(transportDisplayWidth, 40));
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
        default:
            break;
    }
}

void Document::Header::applicationCommandListChanged()
{
    mBubbleTooltipButton.setTooltip(Utils::getCommandDescriptionWithKey(mApplicationCommandManager, ApplicationCommandIDs::viewInfoBubble));
    mEditModeButton.setTooltip(Utils::getCommandDescriptionWithKey(mApplicationCommandManager, ApplicationCommandIDs::frameToggleDrawing));
    Utils::notifyListener(mApplicationCommandManager, *this, {ApplicationCommandIDs::viewInfoBubble, ApplicationCommandIDs::frameToggleDrawing});
}

ANALYSE_FILE_END
