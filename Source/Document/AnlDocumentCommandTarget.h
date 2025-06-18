#pragma once

#include "../Track/Result/AnlTrackResultModifier.h"
#include "AnlDocumentDirector.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class CommandTarget
    : public juce::ApplicationCommandTarget
    {
    public:
        CommandTarget(Director& director, juce::ApplicationCommandManager& commandManager);
        ~CommandTarget() override;

        // juce::ApplicationCommandTarget
        void getAllCommands(juce::Array<juce::CommandID>& commands) override;
        void getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result) override;
        bool perform(juce::ApplicationCommandTarget::InvocationInfo const& info) override;

    private:
        using CommandIDs = ApplicationCommandIDs;

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        Transport::Accessor::Listener mTransportListener{typeid(*this).name()};
        Tools::Clipboard mClipboard;

    protected:
        juce::ApplicationCommandManager& mCommandManager;
    };
} // namespace Document

ANALYSE_FILE_END
