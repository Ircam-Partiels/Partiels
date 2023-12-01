#pragma once

#include "../AnlTrackDirector.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Writer
    : public juce::Component
    {
    public:
        Writer(Director& director, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor);
        ~Writer() override = default;

        bool isPerformingAction() const;
        std::function<void(void)> onModeUpdated = nullptr;

        // juce::Component
        void mouseMove(juce::MouseEvent const& event) override;
        void mouseEnter(juce::MouseEvent const& event) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseUp(juce::MouseEvent const& event) override;
        void mouseDoubleClick(juce::MouseEvent const& event) override;

    private:
        // clang-format off
        enum class ActionMode
        {
              none
            , create
            , move
        };
        // clang-format on

        void updateActionMode(juce::MouseEvent const& event);

        Director& mDirector;
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;

        ActionMode mActionMode;
        bool mMouseWasDragged{false};
        double mMouseDownTime{0.0};
        Edition mCurrentEdition;
    };
} // namespace Track

ANALYSE_FILE_END
