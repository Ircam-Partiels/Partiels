#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

namespace Tooltip
{
    using Client = juce::TooltipClient;

    class Server
    : public juce::Timer
    {
    public:
        Server() = default;
        ~Server() override = default;

        std::function<void(juce::String const&)> onChanged = nullptr;

        // juce::Timer
        void timerCallback() override;

    private:
        juce::String mTip;
    };

    class Display
    : public juce::Component
    {
    public:
        Display();
        ~Display() override = default;

        // juce::Component
        void resized() override;

    private:
        juce::Label mLabel;
        Server mServer;
    };

    class BubbleClient
    {
    protected:
        BubbleClient() = default;

    public:
        ~BubbleClient() = default;

        void setTooltip(juce::String const& tooltip);
        juce::String getTooltip() const;

    private:
        juce::String mTooltip;
    };

    class BubbleWindow
    : public juce::Component
    , public juce::Timer
    {
    public:
        BubbleWindow(juce::Component* owner = nullptr, bool acceptsMouseDown = false);
        ~BubbleWindow() override;

        // juce::Component
        void paint(juce::Graphics& g) override;

        // juce::Timer
        void timerCallback() override;

    private:
        bool const mAcceptsMouseDown;
        juce::String mTooltip;
    };

    //! @brief Shows a call-out box pointing to an area
    //! @param content The component to display inside the call-out box
    //! @param associatedComponent The component the call-out box is opened from (the call-out box is kept above the window containing it)
    //! @param area The area to point to in screen coordinates
    //! @param timeOutMs The delay before dismissing the call-out box (ignored if negative)
    //! @return The call-out box
    juce::CallOutBox& showCallOutBox(std::unique_ptr<juce::Component> content, juce::Component& associatedComponent, juce::Rectangle<int> const& area, int timeOutMs = -1);

    //! @brief Shows a message in a call-out box pointing to an area
    //! @param message The message to display inside the call-out box
    //! @param associatedComponent The component the call-out box is opened from (the call-out box is kept above the window containing it)
    //! @param area The area to point to in screen coordinates
    //! @param timeOutMs The delay before dismissing the call-out box (ignored if negative)
    //! @return The call-out box
    juce::CallOutBox& showMessageBox(juce::String const& message, juce::Component& associatedComponent, juce::Rectangle<int> const& area, int timeOutMs = -1);
} // namespace Tooltip

MISC_FILE_END
