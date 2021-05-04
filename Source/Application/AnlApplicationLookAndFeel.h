#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class LookAndFeel
    : public juce::LookAndFeel_V4
    , public ConcertinaTable::LookAndFeelMethods
    , public IconManager::LookAndFeelMethods
    {
    public:
        class ColourChart
        {
        public:
            // clang-format off
            enum class Type
            {
                  background = 0
                , border
                , inactive
                , active
                , text
            };
            // clang-format on

            using Container = std::array<juce::Colour, magic_enum::enum_count<Type>()>;

            ColourChart(Container colours);
            ColourChart(ColourChart const&) = default;
            ColourChart& operator=(ColourChart const&) = default;

            ~ColourChart() = default;
            juce::Colour get(Type const& type) const;

        private:
            Container mColours;
        };

        LookAndFeel();
        ~LookAndFeel() override = default;

        void setColourChart(ColourChart const& colourChart);

        // ConcertinaTable::LookAndFeelMethods
        juce::Font getHeaderFont(ConcertinaTable const& panel, int headerHeight) const override;
        int getHeaderHeight(ConcertinaTable const& panel) const override;
        void drawHeaderBackground(juce::Graphics& g, ConcertinaTable const& panel, juce::Rectangle<int> area, bool isMouseDown, bool isMouseOver) const override;
        void drawHeaderButton(juce::Graphics& g, ConcertinaTable const& panel, juce::Rectangle<int> area, float sizeRatio, bool isMouseDown, bool isMouseOver) const override;
        void drawHeaderTitle(juce::Graphics& g, ConcertinaTable const& panel, juce::Rectangle<int> area, juce::Font font, bool isMouseDown, bool isMouseOver) const override;

        // IconManager::LookAndFeelMethods
        void setButtonIcon(juce::ImageButton& button, IconManager::IconType const type) override;

        // juce::ScrollBar::LookAndFeelMethods
        bool areScrollbarButtonsVisible() override;
        void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override;

        // juce::ComboBox::LookAndFeelMethods
        void drawComboBox(juce::Graphics& g, int width, int height, const bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override;

        // juce::AlertWindow::LookAndFeelMethods
        void drawAlertBox(juce::Graphics& g, juce::AlertWindow& alert, juce::Rectangle<int> const& textArea, juce::TextLayout& textLayout) override;
        int getAlertWindowButtonHeight() override;

        // juce::TableHeaderComponent::LookAndFeelMethods
        void drawTableHeaderColumn(juce::Graphics& g, juce::TableHeaderComponent& header, juce::String const& columnName, int columnId, int width, int height, bool isMouseOver, bool isMouseDown, int columnFlags) override;

        // juce::DocumentWindow::LookAndFeelMethods
        juce::Button* createDocumentWindowButton(int buttonType) override;
        void positionDocumentWindowButtons(juce::DocumentWindow& window, int titleBarX, int titleBarY, int titleBarW, int titleBarH, juce::Button* minimiseButton, juce::Button* maximiseButton, juce::Button* closeButton, bool positionTitleBarButtonsOnLeft) override;

        // juce::Label::LookAndFeelMethods
        void drawLabel(juce::Graphics& g, juce::Label& label) override;

        // juce::ProgressBar::LookAndFeelMethods
        void drawProgressBar(juce::Graphics& g, juce::ProgressBar& progressBar, int width, int height, double progress, juce::String const& textToShow) override;
        bool isProgressBarOpaque(juce::ProgressBar& progressBar) override;

        // juce::TooltipWindow::LookAndFeelMethods
        void drawTooltip(juce::Graphics& g, juce::String const& text, int width, int height) override;

        // juce::Button::LookAndFeelMethods
        void drawButtonBackground(juce::Graphics& g, juce::Button& button, juce::Colour const& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
        void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
        void drawTickBox(juce::Graphics& g, juce::Component& button, float x, float y, float w, float h, bool ticked, bool isEnabled, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    };
} // namespace Application

ANALYSE_FILE_END
