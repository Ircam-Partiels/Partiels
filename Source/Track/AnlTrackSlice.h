#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    //! @brief Displays the results of a track at the playhead position
    //! @details Only the tracks with points or vectors are supported. For the points, the horizontal axis
    //! corresponds to the values. For the vectors, the horizontal axis corresponds to the bins and the
    //! vertical axis corresponds to the values. The rulers and the scroll bars control the zooms of the track.
    class Slice
    : public juce::Component
    {
    public:
        Slice(Accessor& accessor, Transport::Accessor& transportAccessor);
        ~Slice() override;

        // juce::Component
        void resized() override;

        //! @brief Gets if the results of a track can be displayed by a slice
        //! @param accessor The accessor of the track
        //! @return True if the track has points or vectors
        static bool isSupported(Accessor const& accessor);

        //! @brief Gets the time used to display the results (the running playhead during playback, the start playhead otherwise)
        //! @param transportAccessor The accessor of the transport
        //! @return The time in seconds
        static double getTime(Transport::Accessor const& transportAccessor);

        //! @brief Paints the results of a track at a given time
        //! @param accessor The accessor of the track
        //! @param time The time of the results in seconds
        //! @param g The graphics context
        //! @param bounds The bounds of the drawing
        //! @param horizontalGridColour The colour of the grid of the horizontal axis and of the channel separators (transparent to ignore them)
        //! @param verticalGridColour The colour of the grid of the vertical axis of the vectors (transparent to ignore it)
        static void paint(Accessor const& accessor, double time, juce::Graphics& g, juce::Rectangle<int> const& bounds, juce::Colour const horizontalGridColour, juce::Colour const verticalGridColour);

        //! @brief Gets the maximum width of the labels of the horizontal axis (used to space the ticks of the grid)
        //! @param accessor The accessor of the track
        //! @return The width in pixels
        static int getHorizontalLabelWidth(Accessor const& accessor);

        //! @brief Gets the description of the results of a track at a given time and position
        //! @param accessor The accessor of the track
        //! @param bounds The bounds of the drawing
        //! @param pt The position in the bounds
        //! @param time The time of the results in seconds
        //! @return The lines of the description
        static juce::StringArray getTooltip(Accessor const& accessor, juce::Rectangle<int> const& bounds, juce::Point<int> const& pt, double time);

        //! @brief Draws the results of a track at the playhead position
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
            void updateTooltip(juce::Point<int> const& pt);

            Accessor& mAccessor;
            Transport::Accessor& mTransportAccessor;
            Accessor::Listener mListener{typeid(*this).name()};
            Zoom::Accessor::Listener mZoomListener{typeid(*this).name()};
            Zoom::Grid::Accessor::Listener mGridListener{typeid(*this).name()};
            Transport::Accessor::Listener mTransportListener{typeid(*this).name()};
        };

        //! @brief Allows zooming and scrolling the horizontal and vertical axes of a slice with the mouse as on the plots of the main interface
        //! @details The navigator listens to the mouse events of a content and overlays it. The mouse wheel and the pinch
        //! zoom control the horizontal axis (or the vertical axis with Ctrl) and the click and drag with the modifiers of the
        //! navigation mode (see Zoom::Ruler::getNavigationMode()) zoom and move both axes.
        class Navigator
        : public juce::Component
        {
        public:
            Navigator(juce::Component& content);
            ~Navigator() override;

            //! @brief Sets the zooms controlled by the navigator
            //! @param horizontalZoomAccessor The zoom of the horizontal axis (nullptr to ignore it)
            //! @param verticalZoomAccessor The zoom of the vertical axis (nullptr to ignore it)
            void setZoomAccessors(Zoom::Accessor* horizontalZoomAccessor, Zoom::Accessor* verticalZoomAccessor);

            // juce::Component
            void resized() override;
            void mouseMove(juce::MouseEvent const& event) override;
            void mouseDown(juce::MouseEvent const& event) override;
            void mouseUp(juce::MouseEvent const& event) override;
            void mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel) override;
            void mouseMagnify(juce::MouseEvent const& event, float magnifyAmount) override;

        private:
            using NavigationMode = Zoom::Ruler::NavigationMode;
            static std::unique_ptr<Zoom::Ruler> createRuler(Zoom::Accessor& zoomAccessor, Zoom::Ruler::Orientation orientation);
            void setRulersListening(bool state);
            void updateInteraction(juce::ModifierKeys const& modifiers);
            void updateMouseCursor(juce::ModifierKeys const& modifiers);

            juce::Component& mContent;
            MouseScroller mScroller;
            Zoom::Accessor* mHorizontalZoomAccessor{nullptr};
            Zoom::Accessor* mVerticalZoomAccessor{nullptr};
            std::unique_ptr<Zoom::Ruler> mHorizontalRuler;
            std::unique_ptr<Zoom::Ruler> mVerticalRuler;
            NavigationMode mNavigationMode{NavigationMode::navigate};
        };

        //! @brief Surrounds a content with the rulers and the scroll bars of the horizontal and vertical axes of a slice
        //! @details The content can also be zoomed and scrolled with the mouse (see Navigator). The horizontal ruler (with the labels) and scroll bar control the value zoom of a track with points or
        //! the bin zoom of a track with vectors. The vertical ruler and scroll bar control the value zoom of a track with vectors.
        class Frame
        : public juce::Component
        {
        public:
            Frame(juce::Component& content);
            ~Frame() override;

            //! @brief Sets the tracks whose zooms are controlled by the rulers and the scroll bars
            //! @param horizontalTrack The accessor of the track of the horizontal axis (nullptr or a track without points or vectors to remove the horizontal ruler and scroll bar)
            //! @param verticalTrack The accessor of the track of the vertical axis (nullptr or a track without vectors to remove the vertical ruler and scroll bar)
            void setTracks(Accessor* horizontalTrack, Accessor* verticalTrack);

            // juce::Component
            void resized() override;
            void lookAndFeelChanged() override;

        private:
            static std::unique_ptr<Zoom::Ruler> createRuler(Zoom::Accessor& zoomAccessor, Zoom::Ruler::Orientation orientation, std::function<juce::String(double)> stringify, int maxStringWidth, std::function<std::unique_ptr<juce::Component>(void)> createRangeEditor);

            juce::Component& mContent;
            Decorator mContentDecoration;
            Navigator mNavigator;
            Zoom::Accessor* mHorizontalZoomAccessor{nullptr};
            std::unique_ptr<Zoom::Ruler> mHorizontalRuler;
            std::unique_ptr<Decorator> mHorizontalRulerDecoration;
            std::unique_ptr<Zoom::ScrollBar> mHorizontalScrollBar;
            Zoom::Accessor* mVerticalZoomAccessor{nullptr};
            std::unique_ptr<Zoom::Ruler> mVerticalRuler;
            std::unique_ptr<Decorator> mVerticalRulerDecoration;
            std::unique_ptr<Zoom::ScrollBar> mVerticalScrollBar;
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
        static void paintGrid(Accessor const& accessor, juce::Graphics& g, juce::Rectangle<int> const& bounds, juce::Colour const horizontalGridColour, juce::Colour const verticalGridColour);
        static void paintPoints(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, double time);
        static void paintColumns(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, double time);

        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        Plot mPlot;
        Frame mFrame{mPlot};
    };
} // namespace Track

ANALYSE_FILE_END
