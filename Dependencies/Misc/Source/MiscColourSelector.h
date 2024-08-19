#pragma once

#include "MiscComponentListener.h"

MISC_FILE_BEGIN

class ColourSelector
: public juce::Component
{
public:
    ColourSelector();
    ~ColourSelector() override = default;

    std::function<void(juce::Colour const& colour)> onColourChanged = nullptr;

    juce::Colour getCurrentColour() const;
    void setCurrentColour(juce::Colour newColour, juce::NotificationType notificationType);

    // juce::Component
    void resized() override;

private:
    std::array<float, 4> getHSBA() const;
    void setHSBA(std::array<float, 4> const& hsba, juce::NotificationType notificationType);

    class Header
    : public juce::Component
    {
    public:
        Header(ColourSelector& owner);
        ~Header() override = default;

        // juce::Component
        void paint(juce::Graphics& g) override;
        void resized() override;

        void update();

    private:
        ColourSelector& mOwner;
        juce::Label mLabel;
    };

    class HueSelector
    : public juce::Component
    {
    public:
        HueSelector(ColourSelector& owner);
        ~HueSelector() override = default;

        // juce::Component
        void paint(juce::Graphics& g) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;

    private:
        ColourSelector& mOwner;
    };

    class SatBrightSelector
    : public juce::Component
    {
    public:
        SatBrightSelector(ColourSelector& owner);
        ~SatBrightSelector() override = default;

        // juce::Component
        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;

    private:
        ColourSelector& mOwner;
        float mHue;
        juce::Image mImage;
    };

    class ComponentSlider
    : public juce::Component
    {
    public:
        ComponentSlider(ColourSelector& owner, juce::String label, size_t index);
        ~ComponentSlider() override = default;

        // juce::Component
        void resized() override;

        void update();

    private:
        ColourSelector& mOwner;
        size_t const mIndex;
        juce::Label mLabel;
        juce::Label mValue;
        juce::Slider mSlider;
    };

    std::array<float, 4> mHSBA;
    Header mHeader{*this};
    HueSelector mHueSelector{*this};
    SatBrightSelector mSatBrightSelector{*this};
    ComponentSlider mRedSlider{*this, "Red", 0_z};
    ComponentSlider mGreenSlider{*this, "Green", 1_z};
    ComponentSlider mBlueSlider{*this, "Blue", 2_z};
    ComponentSlider mAlphaSlider{*this, "Alpha", 3_z};
};

class ColourButton
: public juce::Button
, public juce::DragAndDropTarget
{
public:
    // clang-format off
    enum ColourIds
    {
          borderOffColourId = 0x2000000
        , borderOnColourId
    };
    // clang-format on

    ColourButton();
    ~ColourButton() override = default;

    juce::Colour getCurrentColour() const;
    void setCurrentColour(juce::Colour const& newColour, juce::NotificationType notificationType);
    bool isColourSelectorVisible() const;

    std::function<void()> onColourSelectorShow = nullptr;
    std::function<void()> onColourSelectorHide = nullptr;
    std::function<void(juce::Colour const& colour)> onColourChanged = nullptr;

    // juce::Component
    void mouseDown(juce::MouseEvent const& event) override;
    void mouseDrag(juce::MouseEvent const& event) override;
    void mouseUp(juce::MouseEvent const& event) override;

private:
    // juce::Button
    using juce::Button::clicked;
    void clicked() override;
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    // juce::DragAndDropTarget
    bool isInterestedInDragSource(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
    void itemDragEnter(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
    void itemDragExit(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
    void itemDropped(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;

    juce::Colour mColour;
    juce::Component::SafePointer<ColourButton> mDraggedColour;
    bool mIsColourSelectorVisible{false};
    ComponentListener mComponentListener;
};

MISC_FILE_END
