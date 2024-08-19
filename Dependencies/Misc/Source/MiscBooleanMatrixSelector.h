#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

class BooleanMatrixSelector
: public juce::Component
, private juce::ScrollBar::Listener
{
public:
    BooleanMatrixSelector();
    ~BooleanMatrixSelector() override = default;

    // juce::Component
    void resized() override;

    void setSize(size_t numRows, size_t numColumns);
    std::pair<size_t, size_t> getSize() const;

    void setCellState(size_t row, size_t column, bool state, juce::NotificationType notification);
    bool getCellState(size_t row, size_t column) const;

    std::function<void(size_t row, size_t column)> onClick = nullptr;

    int getOptimalWidth() const;
    int getOptimalHeight() const;

private:
    // juce::ScrollBar::Listener
    void scrollBarMoved(juce::ScrollBar* scrollBar, double newRangeStart) override;

    class ViewPort
    : public juce::Viewport
    {
    public:
        ViewPort() = default;
        ~ViewPort() override = default;

        void visibleAreaChanged(juce::Rectangle<int> const& newVisibleArea) override;
        std::function<void(juce::Rectangle<int> const&)> onVisibleAreaChanged = nullptr;
    };

    std::vector<std::unique_ptr<juce::Label>> mRowLabels;
    juce::Component mRowLabelsContainer;
    ViewPort mRowLabelsViewPort;

    std::vector<std::unique_ptr<juce::Label>> mColumnLabels;
    juce::Component mColumnLabelsContainer;
    ViewPort mColumnLabelsViewPort;

    std::vector<std::vector<std::unique_ptr<juce::ToggleButton>>> mButtons;
    juce::Component mButtonsContainer;
    ViewPort mButtonsViewPort;

    juce::ScrollBar mRowScrollBar{true};
    juce::ScrollBar mColumnScrollBar{false};

    static auto constexpr mCellWidth = 20;
    static auto constexpr mCellHeight = 20;
    static auto constexpr mRowLabelWidth = 30;
    static auto constexpr mColumnLabelHeight = 20;
};

MISC_FILE_END
