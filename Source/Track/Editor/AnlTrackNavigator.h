#pragma once

#include "../AnlTrackDirector.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Navigator
    : public juce::Component
    {
    public:
        Navigator(Accessor& accessor, Zoom::Accessor& timeZoomAccessor);
        ~Navigator() override;

        bool isPerformingAction() const;
        std::function<void(void)> onModeUpdated = nullptr;

        // juce::Component
        void resized() override;
        void mouseMove(juce::MouseEvent const& event) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseUp(juce::MouseEvent const& event) override;
        void modifierKeysChanged(juce::ModifierKeys const& modifiers) override;

    private:
        using NavigationMode = Zoom::Ruler::NavigationMode;
        void updateInteraction(juce::ModifierKeys const& modifiers);
        void updateMouseCursor(juce::ModifierKeys const& modifiers);

        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        std::vector<std::unique_ptr<Zoom::Ruler>> mVerticalRulers;
        std::optional<std::reference_wrapper<Zoom::Accessor>> mVerticalZoomAcsr;
        Zoom::Ruler mTimeRuler;
        NavigationMode mNavigationMode{NavigationMode::navigate};
    };
} // namespace Track

ANALYSE_FILE_END
