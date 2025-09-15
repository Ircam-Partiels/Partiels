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

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        juce::ApplicationCommandManager& mApplicationCommandManager;

        Icon mDocumentFileButton;
        Icon mReaderLayoutButton;
        TransportDisplay mTransportDisplay;
        TransportSelectionInfo mTransportSelectionInfo;
        juce::TextButton mOscButton;
        Icon mBubbleTooltipButton;
        Icon mEditModeButton;
    };
} // namespace Document

ANALYSE_FILE_END
