#pragma once

#include "../Track/AnlTrackSlice.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    //! @brief Displays the results of the tracks of a group at the playhead position
    //! @details Only the tracks with points or vectors are displayed (see Track::Slice). The horizontal axis uses
    //! the same track as the vertical ruler of the group in the main interface and the vertical axis uses the first
    //! track with vectors.
    class Slice
    : public juce::Component
    {
    public:
        Slice(Accessor& accessor, Transport::Accessor& transportAccessor);
        ~Slice() override;

        // juce::Component
        void resized() override;

        //! @brief Gets if the results of the tracks of a group can be displayed by a slice
        //! @param accessor The accessor of the group
        //! @return True if at least one track displayed in the group has points or vectors
        static bool isSupported(Accessor const& accessor);

        //! @brief Gets the track used for the vertical axis of the slice of a group
        //! @details This is the first track displayed in the group with vectors. The horizontal axis uses the same
        //! track as the vertical ruler of the group in the main interface (see Group::Tools::getVerticalZoomTrackAcsr).
        //! @param accessor The accessor of the group
        //! @return The accessor of the track if any
        static std::optional<std::reference_wrapper<Track::Accessor>> getVectorTrackAcsr(Accessor const& accessor);

        //! @brief Draws the results of the tracks of a group at the playhead position
        class Plot
        : public juce::Component
        , public Tooltip::BubbleClient
        {
        public:
            Plot(Accessor& accessor, Transport::Accessor& transportAccessor);
            ~Plot() override;

            // juce::Component
            void paint(juce::Graphics& g) override;
            void mouseMove(juce::MouseEvent const& event) override;
            void mouseEnter(juce::MouseEvent const& event) override;
            void mouseExit(juce::MouseEvent const& event) override;

        private:
            void updateContent();
            void updateTooltip(juce::Point<int> const& pt);

            Accessor& mAccessor;
            Transport::Accessor& mTransportAccessor;
            Accessor::Listener mListener{typeid(*this).name()};
            Transport::Accessor::Listener mTransportListener{typeid(*this).name()};
            Zoom::Accessor::Listener mZoomListener{typeid(*this).name()};
            Zoom::Grid::Accessor::Listener mGridListener{typeid(*this).name()};
            Track::Accessor::Listener mTrackListener{typeid(*this).name()};
            TrackMap<std::reference_wrapper<Track::Accessor>> mTrackAccessors;
            LayoutNotifier mLayoutNotifier;
        };

        //! @brief A floating window that displays a slice
        class WindowContainer
        : public FloatingWindowContainer
        {
        public:
            WindowContainer(Slice& slice);
            ~WindowContainer() override;

        private:
            Slice& mSlice;
            Accessor::Listener mListener{typeid(*this).name()};
        };

    private:
        void updateFrame();

        Accessor& mAccessor;
        Plot mPlot;
        Track::Slice::Frame mFrame{mPlot};
        LayoutNotifier mLayoutNotifier;
    };
} // namespace Group

ANALYSE_FILE_END
