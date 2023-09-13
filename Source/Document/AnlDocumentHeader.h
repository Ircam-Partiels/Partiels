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
        Header(Director& director, juce::ApplicationCommandManager& commandManager, AuthorizationProcessor& authorizationProcessor);
        ~Header() override;

        // juce::Component
        void resized() override;

    private:
        // juce::ApplicationCommandManagerListener
        void applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info) override;
        void applicationCommandListChanged() override;

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        juce::ApplicationCommandManager& mApplicationCommandManager;

        Icon mReaderLayoutButton;
        juce::TextButton mNameButton;
        AuthorizationButton mAuthorizationButton;
        TransportDisplay mTransportDisplay;
        TransportSelectionInfo mTransportSelectionInfo;
        Icon mBubbleTooltipButton;
        ComponentListener mComponentListener;
    };
} // namespace Document

ANALYSE_FILE_END
