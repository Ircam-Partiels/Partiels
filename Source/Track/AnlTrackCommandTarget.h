#pragma once

#include "Result/AnlTrackResultModifier.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class CommandTarget
    : public juce::ApplicationCommandTarget
    {
    public:
        // clang-format off
        enum CommandIDs : int
        {
              editSelectAll = 0x2001
            , editDeleteSelection
            , editCopySelection
            , editCutSelection
            , editPasteSelection
            , editDuplicateSelection
        };
        // clang-format on

        CommandTarget(Director& director, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAcsr);
        ~CommandTarget() override;

        // juce::ApplicationCommandTarget
        void getAllCommands(juce::Array<juce::CommandID>& commands) override;
        void getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result) override;
        bool perform(juce::ApplicationCommandTarget::InvocationInfo const& info) override;

    private:
        using CopiedData = Result::Modifier::CopiedData;

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        Zoom::Accessor& mTimeZoomAccessor;
        Zoom::Accessor::Listener mTimeZoomListener{typeid(*this).name()};
        Transport::Accessor& mTransportAccessor;
        Transport::Accessor::Listener mTransportListener{typeid(*this).name()};
        CopiedData mCopiedData;
        juce::Range<double> mCopiedSelection;
    };
} // namespace Track

ANALYSE_FILE_END
