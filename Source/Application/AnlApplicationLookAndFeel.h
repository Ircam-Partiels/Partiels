#pragma once

#include "../Misc/AnlMisc.h"
#include "../Layout/AnlLayout.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class LookAndFeel
    : public juce::LookAndFeel_V4
    , public ConcertinaPanel::LookAndFeelMethods
    {
    public:
        LookAndFeel();
        ~LookAndFeel() override = default;
        
        // ConcertinaPanel::LookAndFeelMethods
        int getSeparatorHeight(ConcertinaPanel const& panel) const override;
        juce::Font getHeaderFont(ConcertinaPanel const& panel, int headerHeight) const override;
        int getHeaderHeight(ConcertinaPanel const& panel) const override;
        void drawHeaderBackground(juce::Graphics& g, ConcertinaPanel const& panel, juce::Rectangle<int> area, bool isMouseDown, bool isMouseOver) const override;
        void drawHeaderButton(juce::Graphics& g, ConcertinaPanel const& panel, juce::Rectangle<int> area, float sizeRatio, bool isMouseDown, bool isMouseOver) const override;
        void drawHeaderTitle(juce::Graphics& g, ConcertinaPanel const& panel, juce::Rectangle<int> area, juce::Font font, bool isMouseDown, bool isMouseOver) const override;
        
        // ScrollBar::LookAndFeelMethods
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
    };
}

ANALYSE_FILE_END

