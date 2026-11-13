#pragma once

#include "AnlDocumentDirector.h"
#include "AnlDocumentTransportDisplay.h"
#include "AnlDocumentTransportSelectionInfo.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Header
    : public juce::Component
    , private juce::ApplicationCommandManagerListener
    {
    public:
        Header(Director& director, juce::ApplicationCommandManager& commandManager);
        ~Header() override;

        // juce::Component
        void resized() override;
        void colourChanged() override;

    private:
        // juce::ApplicationCommandManagerListener
        void applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info) override;
        void applicationCommandListChanged() override;

        //! @brief A popup menu item that displays the name of a channel of the audio files layout with mute and solo buttons
        //! @details The item is not triggered automatically so the menu stays open when the buttons are clicked.
        //! Without index, the item controls all the channels: the buttons are on if all the channels are muted or soloed,
        //! and clicking a button mutes or solos all the channels if they are not all muted or soloed, otherwise it clears them.
        class ChannelPlaybackItem
        : public juce::PopupMenu::CustomComponent
        {
        public:
            ChannelPlaybackItem(Accessor& accessor, std::optional<size_t> index, juce::String const& name);
            ~ChannelPlaybackItem() override;

            // juce::PopupMenu::CustomComponent
            void getIdealSize(int& idealWidth, int& idealHeight) override;

            // juce::Component
            void resized() override;
            void paint(juce::Graphics& g) override;

        private:
            void update();
            juce::Rectangle<int> getTextBounds() const;
            bool isToggled(std::set<size_t> const& indices) const;
            std::set<size_t> toggle(std::set<size_t> indices) const;

            Accessor& mAccessor;
            Accessor::Listener mListener{typeid(*this).name()};
            std::optional<size_t> const mIndex;
            juce::String const mName;
            juce::TextButton mMuteButton{juce::translate("M")};
            juce::TextButton mSoloButton{juce::translate("S")};
        };

        void showChannelsPlaybackMenu();
        void updateChannelsPlaybackButton();

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        juce::ApplicationCommandManager& mApplicationCommandManager;

        Icon mDocumentFileButton;
        Icon mReaderLayoutButton;
        juce::TextButton mChannelsPlaybackButton;
        TransportDisplay mTransportDisplay;
        TransportSelectionInfo mTransportSelectionInfo;
        juce::TextButton mOscButton;
        Icon mBubbleTooltipButton;
        Icon mEditModeButton;
    };
} // namespace Document

ANALYSE_FILE_END
