#pragma once

#include "../Tools/AnlMisc.h"
#include "../Layout/AnlLayout.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class LookAndFeel
    : public juce::LookAndFeel_V4
    , public Layout::PropertySection::LookAndFeelMethods
    {
    public:
        LookAndFeel();
        ~LookAndFeel() override = default;
        
        // Layout::PropertySection::LookAndFeelMethods
        int getSeparatorHeight(Layout::PropertySection const& section) const override;
        juce::Font getHeaderFont(Layout::PropertySection const& section, int headerHeight) const override;
        int getHeaderHeight(Layout::PropertySection const& section) const override;
        void drawHeaderBackground(juce::Graphics& g, Layout::PropertySection const& section, juce::Rectangle<int> area, bool isMouseDown, bool isMouseOver) const override;
        void drawHeaderButton(juce::Graphics& g, Layout::PropertySection const& section, juce::Rectangle<int> area, float sizeRatio, bool isMouseDown, bool isMouseOver) const override;
        void drawHeaderTitle(juce::Graphics& g, Layout::PropertySection const& section, juce::Rectangle<int> area, juce::Font font, bool isMouseDown, bool isMouseOver) const override;
        
        // ScrollBar::LookAndFeelMethods
        bool areScrollbarButtonsVisible() override;
        void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override;
        
        // juce::ComboBox::LookAndFeelMethods
        void drawComboBox(juce::Graphics& g, int width, int height, const bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override;
        
        // juce::AlertWindow::LookAndFeelMethods
        void drawAlertBox(juce::Graphics& g, juce::AlertWindow& alert, juce::Rectangle<int> const& textArea, juce::TextLayout& textLayout) override;
        int getAlertWindowButtonHeight() override;
    };
}

ANALYSE_FILE_END

