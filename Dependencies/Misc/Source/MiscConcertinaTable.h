#pragma once

#include "MiscComponentListener.h"

MISC_FILE_BEGIN

//! @todo check if we should ke facultative header (draggable table)
class ConcertinaTable
: public juce::Component
, public juce::SettableTooltipClient
, public juce::ChangeBroadcaster
, private juce::Timer
{
public:
    // clang-format off
    enum ColourIds : int
    {
          headerBackgroundColourId = 0x2000200
        , headerBorderColourId
        , headerTitleColourId
        , headerButtonColourId
    };
    // clang-format on

    using ComponentRef = std::reference_wrapper<juce::Component>;

    ConcertinaTable(juce::String const& title, bool resizeOnClick, juce::String const& tooltip = "");
    ~ConcertinaTable() override;

    juce::String getTitle() const;
    void setComponents(std::vector<ComponentRef> const& component);
    std::vector<juce::Component::SafePointer<juce::Component>> getComponents();
    void setOpen(bool isOpen, bool shouldAnimate = false);
    bool isOpen() const;
    bool isAnimating() const;

    // juce::Component
    void resized() override;

    struct LookAndFeelMethods
    {
        virtual ~LookAndFeelMethods() = default;

        virtual int getHeaderHeight(ConcertinaTable const& section) const = 0;
        virtual juce::Font getHeaderFont(ConcertinaTable const& section, int headerHeight) const = 0;
        virtual void drawHeaderBackground(juce::Graphics& g, ConcertinaTable const& section, juce::Rectangle<int> area, bool isMouseDown, bool isMouseOver) const = 0;
        virtual void drawHeaderButton(juce::Graphics& g, ConcertinaTable const& section, juce::Rectangle<int> area, float sizeRatio, bool isMouseDown, bool isMouseOver) const = 0;
        virtual void drawHeaderTitle(juce::Graphics& g, ConcertinaTable const& section, juce::Rectangle<int> area, juce::Font font, bool isMouseDown, bool isMouseOver) const = 0;
    };

private:
    // juce::Timer
    void timerCallback() override;

    class Header
    : public juce::Component
    , public juce::SettableTooltipClient
    {
    public:
        Header() = default;
        ~Header() override = default;

        std::function<void(void)> onClicked = nullptr;

        // juce::Component
        void paint(juce::Graphics& g) override;
        void mouseDown(juce::MouseEvent const& event) override;
    };

    Header mHeader;
    juce::String const mTitle;
    ComponentListener mComponentListener;
    std::vector<juce::Component::SafePointer<juce::Component>> mContents;
    float mSizeRatio = 1.0f;
    bool mOpened = true;
};

MISC_FILE_END
