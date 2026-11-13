#include "AnlDocumentHeader.h"
#include "AnlDocumentAudioReader.h"
#include "AnlDocumentTools.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

Document::Header::Header(Director& director, juce::ApplicationCommandManager& commandManager)
: mDirector(director)
, mApplicationCommandManager(commandManager)
, mDocumentFileButton(ScalableImage::fromSvg(AnlIconsData::DocumentFile_svg, AnlIconsData::DocumentFile_svgSize))
, mReaderLayoutButton(ScalableImage::fromSvg(AnlIconsData::AudioLayer_svg, AnlIconsData::AudioLayer_svgSize))
, mChannelsPlaybackButton(juce::translate("M / S"), juce::translate("Mute or solo the channels of the audio files layout during the playback"))
, mTransportDisplay(mAccessor.getAcsr<AcsrType::transport>(), mAccessor.getAcsr<AcsrType::timeZoom>(), commandManager)
, mTransportSelectionInfo(mAccessor.getAcsr<AcsrType::transport>(), mAccessor.getAcsr<AcsrType::timeZoom>())
, mOscButton(juce::translate("OSC"), juce::translate("Show OSC Settings"))
, mBubbleTooltipButton(ScalableImage::fromSvg(AnlIconsData::Bubble_svg, AnlIconsData::Bubble_svgSize))
, mEditModeButton(ScalableImage::fromSvg(AnlIconsData::Edit_svg, AnlIconsData::Edit_svgSize))
{
    addAndMakeVisible(mReaderLayoutButton);
    addAndMakeVisible(mDocumentFileButton);
    addAndMakeVisible(mChannelsPlaybackButton);
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

    mChannelsPlaybackButton.setWantsKeyboardFocus(false);
    mChannelsPlaybackButton.getProperties().set("Outlined", true);
    mChannelsPlaybackButton.onClick = [this]()
    {
        showChannelsPlaybackMenu();
    };

    mOscButton.getProperties().set("Outlined", true);
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
                updateChannelsPlaybackButton();
            }
            break;
            case AttrType::mutedChannels:
            case AttrType::soloedChannels:
            {
                updateChannelsPlaybackButton();
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
    // The OSC button and the channels playback button are stacked and centered in the column
    static auto constexpr textButtonWidth = 40;
    static auto constexpr textButtonHeight = 16;
    static auto constexpr textButtonSpace = 6;
    auto const hasTextButtonColumn = rightPart.getWidth() > 44;
    mOscButton.setVisible(hasTextButtonColumn);
    mChannelsPlaybackButton.setVisible(hasTextButtonColumn);
    auto textButtonColumn = rightPart.removeFromLeft(44).withSizeKeepingCentre(textButtonWidth, textButtonHeight * 2 + textButtonSpace);
    mOscButton.setBounds(textButtonColumn.removeFromTop(textButtonHeight));
    mChannelsPlaybackButton.setBounds(textButtonColumn.removeFromBottom(textButtonHeight));

    mTransportDisplay.setVisible(bounds.getWidth() >= transportDisplayWidth);
    if(mTransportDisplay.isVisible())
    {
        bounds.removeFromLeft((bounds.getWidth() - transportDisplayWidth) / 2);
        mTransportDisplay.setBounds(bounds.removeFromLeft(transportDisplayWidth).withSizeKeepingCentre(transportDisplayWidth, 40));
    }
}

Document::Header::ChannelPlaybackItem::ChannelPlaybackItem(Accessor& accessor, std::optional<size_t> index, juce::String const& name)
: juce::PopupMenu::CustomComponent(false)
, mAccessor(accessor)
, mIndex(index)
, mName(name)
{
    auto const setupButton = [this](juce::TextButton& button, juce::Colour const& onColour, juce::String const& tooltip)
    {
        button.setWantsKeyboardFocus(false);
        button.setClickingTogglesState(false);
        button.setColour(juce::TextButton::ColourIds::buttonOnColourId, onColour);
        button.setColour(juce::TextButton::ColourIds::textColourOnId, juce::Colours::black);
        button.setTooltip(tooltip);
        addAndMakeVisible(button);
    };
    setupButton(mMuteButton, juce::Colour(0xffe8a33d), mIndex.has_value() ? juce::translate("Mute or unmute the channel during the playback") : juce::translate("Mute or unmute all the channels during the playback"));
    setupButton(mSoloButton, juce::Colour(0xffe6d94a), mIndex.has_value() ? juce::translate("Solo or unsolo the channel during the playback") : juce::translate("Solo or unsolo all the channels during the playback"));

    mMuteButton.onClick = [this]()
    {
        mAccessor.setAttr<AttrType::mutedChannels>(toggle(mAccessor.getAttr<AttrType::mutedChannels>()), NotificationType::synchronous);
    };
    mSoloButton.onClick = [this]()
    {
        mAccessor.setAttr<AttrType::soloedChannels>(toggle(mAccessor.getAttr<AttrType::soloedChannels>()), NotificationType::synchronous);
    };

    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::reader:
            case AttrType::mutedChannels:
            case AttrType::soloedChannels:
            {
                update();
            }
            break;
            case AttrType::layout:
            case AttrType::viewport:
            case AttrType::path:
            case AttrType::grid:
            case AttrType::autoresize:
            case AttrType::samplerate:
            case AttrType::channels:
            case AttrType::editMode:
            case AttrType::drawingState:
            case AttrType::description:
                break;
        }
    };
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::Header::ChannelPlaybackItem::~ChannelPlaybackItem()
{
    mAccessor.removeListener(mListener);
}

void Document::Header::ChannelPlaybackItem::getIdealSize(int& idealWidth, int& idealHeight)
{
    static auto constexpr buttonsWidth = 2 * 22 + 8;
    idealHeight = 24;
    auto const font = getLookAndFeel().getPopupMenuFont();
    auto const textOffset = static_cast<int>(std::round(static_cast<float>(idealHeight) / 1.3f)) + 1;
    idealWidth = textOffset + juce::GlyphArrangement::getStringWidthInt(font, mName) + 12 + buttonsWidth;
}

juce::Rectangle<int> Document::Header::ChannelPlaybackItem::getTextBounds() const
{
    // The text is aligned with the text of the standard items of the menu
    auto bounds = getLocalBounds().reduced(1);
    bounds.removeFromLeft(static_cast<int>(std::round(static_cast<float>(bounds.getHeight()) / 1.3f)));
    return bounds.withRight(mMuteButton.getX() - 4);
}

void Document::Header::ChannelPlaybackItem::resized()
{
    auto bounds = getLocalBounds().withTrimmedRight(4);
    mSoloButton.setBounds(bounds.removeFromRight(22).withSizeKeepingCentre(20, 18));
    bounds.removeFromRight(4);
    mMuteButton.setBounds(bounds.removeFromRight(22).withSizeKeepingCentre(20, 18));
}

bool Document::Header::ChannelPlaybackItem::isToggled(std::set<size_t> const& indices) const
{
    if(mIndex.has_value())
    {
        return indices.count(*mIndex) > 0_z;
    }
    auto const numChannels = mAccessor.getAttr<AttrType::reader>().size();
    for(auto index = 0_z; index < numChannels; ++index)
    {
        if(indices.count(index) == 0_z)
        {
            return false;
        }
    }
    return numChannels > 0_z;
}

std::set<size_t> Document::Header::ChannelPlaybackItem::toggle(std::set<size_t> indices) const
{
    if(mIndex.has_value())
    {
        if(indices.erase(*mIndex) == 0_z)
        {
            indices.insert(*mIndex);
        }
        return indices;
    }
    if(isToggled(indices))
    {
        return {};
    }
    auto const numChannels = mAccessor.getAttr<AttrType::reader>().size();
    for(auto index = 0_z; index < numChannels; ++index)
    {
        indices.insert(index);
    }
    return indices;
}

void Document::Header::ChannelPlaybackItem::paint(juce::Graphics& g)
{
    // The name is dimmed when the channel is not audible (muted or another channel is soloed)
    auto const isAudible = !mIndex.has_value() || Tools::isChannelAudible(mAccessor, *mIndex);
    auto const textColour = findColour(juce::PopupMenu::ColourIds::textColourId);
    g.setColour(isAudible ? textColour : textColour.withMultipliedAlpha(0.4f));
    g.setFont(getLookAndFeel().getPopupMenuFont());
    g.drawFittedText(mName, getTextBounds(), juce::Justification::centredLeft, 1);
}

void Document::Header::ChannelPlaybackItem::update()
{
    mMuteButton.setToggleState(isToggled(mAccessor.getAttr<AttrType::mutedChannels>()), juce::NotificationType::dontSendNotification);
    mSoloButton.setToggleState(isToggled(mAccessor.getAttr<AttrType::soloedChannels>()), juce::NotificationType::dontSendNotification);
    repaint();
}

void Document::Header::showChannelsPlaybackMenu()
{
    auto const& layouts = mAccessor.getAttr<AttrType::reader>();
    if(layouts.empty())
    {
        return;
    }

    auto const getLayoutName = [&](size_t index)
    {
        auto const& layout = layouts[index];
        auto const channelName = [&]() -> juce::String
        {
            switch(layout.channel)
            {
                case AudioFileLayout::ChannelLayout::all:
                    return juce::translate("All");
                case AudioFileLayout::ChannelLayout::mono:
                    return juce::translate("Mono");
                default:
                    return juce::String(layout.channel + 1);
            }
        }();
        return juce::String(index + 1_z) + ". " + layout.file.getFileName() + " [" + channelName + "]";
    };

    juce::PopupMenu menu;
    for(auto index = 0_z; index < layouts.size(); ++index)
    {
        auto const name = getLayoutName(index);
        menu.addCustomItem(static_cast<int>(index) + 1, std::make_unique<ChannelPlaybackItem>(mAccessor, index, name), nullptr, name);
    }
    menu.addSeparator();
    auto const allName = juce::translate("All");
    menu.addCustomItem(static_cast<int>(layouts.size()) + 1, std::make_unique<ChannelPlaybackItem>(mAccessor, std::nullopt, allName), nullptr, allName);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mChannelsPlaybackButton).withDeletionCheck(*this));
}

void Document::Header::updateChannelsPlaybackButton()
{
    auto const numChannels = mAccessor.getAttr<AttrType::reader>().size();
    mChannelsPlaybackButton.setEnabled(numChannels > 0_z);
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
            // The colour is removed when disconnected so the button uses the colour of the look and feel again
            if(info.commandFlags & juce::ApplicationCommandInfo::CommandFlags::isTicked)
            {
                mOscButton.setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colours::green);
            }
            else
            {
                mOscButton.removeColour(juce::TextButton::ColourIds::buttonColourId);
            }
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
