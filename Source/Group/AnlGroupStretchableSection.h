#pragma once

#include "../Track/AnlTrackSection.h"
#include "AnlGroupSection.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class StretchableSection
    : public juce::Component
    , private juce::ChangeListener
    , private juce::Timer
    {
    public:
        using ResizerFn = Section::ResizerFn;

        StretchableSection(Director& director, juce::ApplicationCommandManager& commandManager, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr, Track::PresetList::Accessor& presetListAcsr, ResizerFn resizerFn);
        ~StretchableSection() override;

        bool isStretching() const;
        juce::Component const& getSection(juce::String const& identifier) const;
        juce::Component const* getPlot(juce::String const& identifier) const;
        void setCanAnimate(bool state);
        void setLastItemResizable(bool state);

        std::function<void(void)> onLayoutChanged = nullptr;
        std::function<void(juce::String const& identifier, size_t index, bool copy)> onTrackInserted = nullptr;

        // juce::Component
        void resized() override;

    private:
        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;

        // juce::Timer
        void timerCallback() override;

        void updateContent();
        void updateResizable();

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Transport::Accessor& mTransportAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Track::PresetList::Accessor& mTrackPresetListAccessor;
        juce::ApplicationCommandManager& mApplicationCommandManager;
        Accessor::Listener mListener{typeid(*this).name()};
        ResizerFn mResizerFn;

        Section mSection;
        TrackMap<std::unique_ptr<Track::Section>> mTrackSections;
        DraggableTable mDraggableTable{"Track"};
        ConcertinaTable mConcertinaTable{"", false};
        ComponentListener mComponentListener;
        bool mIsResizable{true};
        bool mCanAnimate{false};

        LayoutNotifier mLayoutNotifier;
    };
} // namespace Group

ANALYSE_FILE_END
