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
        // clang-format off
        enum CommandIDs : int
        {
            selectAll = 0x2001,
            editDelete,
            editCopy,
            editCut,
            editPaste,
            editDuplicate
        };
        // clang-format on

        CommandTarget(Director& director, juce::ApplicationCommandManager& commandManager);
        ~CommandTarget() override;

        // juce::ApplicationCommandTarget
        void getAllCommands(juce::Array<juce::CommandID>& commands) override;
        void getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result) override;
        bool perform(juce::ApplicationCommandTarget::InvocationInfo const& info) override;

    private:
        std::set<size_t> getSelectedChannels(Track::Accessor const& trackAcsr) const;

        using CopiedData = Track::Result::Modifier::CopiedData;
        using MultiChannelData = std::map<size_t, CopiedData>;

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        Transport::Accessor::Listener mTransportListener{typeid(*this).name()};
        std::map<juce::String, MultiChannelData> mClipboardData;
        juce::Range<double> mClipboardRange;

    protected:
        juce::ApplicationCommandManager& mCommandManager;
    };
} // namespace Document

ANALYSE_FILE_END
