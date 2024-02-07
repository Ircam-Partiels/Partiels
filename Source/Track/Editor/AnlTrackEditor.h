#pragma once

#include "AnlTrackNavigator.h"
#include "AnlTrackScroller.h"
#include "AnlTrackSelector.h"
#include "AnlTrackWriter.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Editor
    : public ComponentSnapshot
    , public Tooltip::BubbleClient
    , public juce::SettableTooltipClient
    , private juce::ApplicationCommandManagerListener
    {
    public:
        Editor(Director& director, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor, juce::ApplicationCommandManager& commandManager, juce::Component& content, std::function<juce::String(juce::Point<int> const&)> getTooltip, bool paintBackground);
        ~Editor() override;

        void setSnapshotNameAndColour(juce::String const& name, juce::Colour const& colour);
        std::function<void(juce::MouseEvent const& event)> onMouseDown = nullptr;

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void mouseMove(juce::MouseEvent const& event) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseEnter(juce::MouseEvent const& event) override;
        void mouseExit(juce::MouseEvent const& event) override;

    private:
        // ComponentSnapshot
        void takeSnapshot() override;

        // juce::ApplicationCommandManagerListener
        void applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info) override;
        void applicationCommandListChanged() override;

        void editionUpdated();

        juce::Component& mContent;
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        juce::ApplicationCommandManager& mApplicationCommandManager;
        Accessor::Listener mListener{typeid(*this).name()};
        Zoom::Accessor::Listener mZoomListener{typeid(*this).name()};
        bool const mPaintBackground;
        juce::String mSnapshotName;
        juce::Colour mSnapshotColour;
        std::function<juce::String(juce::Point<int> const&)> mGetTooltip;

        Writer mWriter;
        Navigator mNavigator;
        Scroller mScroller;
        Selector mSelector;
    };
} // namespace Track

ANALYSE_FILE_END
