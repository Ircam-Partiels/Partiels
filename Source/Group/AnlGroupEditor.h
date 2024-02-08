#pragma once

#include "../Track/Editor/AnlTrackEditor.h"
#include "AnlGroupDirector.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class Editor
    : public juce::Component
    {
    public:
        Editor(Director& director, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor, juce::ApplicationCommandManager& commandManager, juce::Component& content);
        ~Editor() override;

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;

    private:
        void updateContent();
        void updateEditorNameAndColour();
        void showPopupMenu(juce::Point<int> position = juce::Desktop::getMousePosition());
        juce::String getBubbleTooltip(juce::Point<int> const& pt);

        // clang-format off
        enum class PopupMenuAction
        {
              none
            , referenceTrack
            , trackLayout
            , channelLayout
        };
        // clang-format on

        juce::Component& mContent;
        Director& mDirector;
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;
        juce::ApplicationCommandManager& mCommandManager;
        Accessor::Listener mListener{typeid(*this).name()};

        std::unique_ptr<Track::Editor> mTrackEditor;
        juce::String mTrackIdentifier;
        LayoutNotifier mLayoutNotifier;
        PopupMenuAction mPopupMenuAction = PopupMenuAction::none;
    };
} // namespace Group

ANALYSE_FILE_END
