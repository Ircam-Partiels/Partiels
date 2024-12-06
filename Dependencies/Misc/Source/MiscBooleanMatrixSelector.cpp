#include "MiscBooleanMatrixSelector.h"

MISC_FILE_BEGIN

void BooleanMatrixSelector::ViewPort::visibleAreaChanged(juce::Rectangle<int> const& newVisibleArea)
{
    if(onVisibleAreaChanged != nullptr)
    {
        onVisibleAreaChanged(newVisibleArea);
    }
}

BooleanMatrixSelector::BooleanMatrixSelector()
{
    addAndMakeVisible(mRowLabelsViewPort);
    mRowLabelsViewPort.setViewedComponent(&mRowLabelsContainer, false);
    mRowLabelsViewPort.setScrollBarsShown(false, false, true, false);
    mRowLabelsViewPort.onVisibleAreaChanged = [&](juce::Rectangle<int> const& newVisibleArea)
    {
        mButtonsViewPort.setViewPosition(mButtonsViewPort.getViewPositionX(), newVisibleArea.getY());
    };

    addAndMakeVisible(mColumnLabelsViewPort);
    mColumnLabelsViewPort.setViewedComponent(&mColumnLabelsContainer, false);
    mColumnLabelsViewPort.setScrollBarsShown(false, false, false, true);
    mColumnLabelsViewPort.onVisibleAreaChanged = [&](juce::Rectangle<int> const& newVisibleArea)
    {
        mButtonsViewPort.setViewPosition(newVisibleArea.getX(), mButtonsViewPort.getViewPositionY());
    };

    addAndMakeVisible(mButtonsViewPort);
    mButtonsViewPort.setViewedComponent(&mButtonsContainer, false);
    mButtonsViewPort.setScrollBarsShown(false, false, true, true);
    mButtonsViewPort.onVisibleAreaChanged = [&](juce::Rectangle<int> const& newVisibleArea)
    {
        mRowLabelsViewPort.setViewPosition(0, newVisibleArea.getY());
        mColumnLabelsViewPort.setViewPosition(newVisibleArea.getX(), 0);
        mRowScrollBar.setCurrentRange(mButtonsViewPort.getVerticalScrollBar().getCurrentRange(), juce::NotificationType::dontSendNotification);
        mColumnScrollBar.setCurrentRange(mButtonsViewPort.getHorizontalScrollBar().getCurrentRange(), juce::NotificationType::dontSendNotification);
    };

    mRowScrollBar.setAutoHide(true);
    addAndMakeVisible(mRowScrollBar);
    mRowScrollBar.addListener(this);

    mColumnScrollBar.setAutoHide(true);
    addAndMakeVisible(mColumnScrollBar);
    mColumnScrollBar.addListener(this);
}

void BooleanMatrixSelector::resized()
{
    auto const scrollbarSize = getLookAndFeel().getDefaultScrollbarWidth();

    auto bounds = getLocalBounds();
    auto const btnsCtnrWidth = bounds.getWidth() - mRowLabelWidth - scrollbarSize;
    auto const btnsCtnrHeight = bounds.getHeight() - mColumnLabelHeight - scrollbarSize;

    // Scrollbars
    mRowScrollBar.setBounds(bounds.removeFromLeft(scrollbarSize).withHeight(btnsCtnrHeight));
    mColumnScrollBar.setBounds(bounds.removeFromBottom(scrollbarSize).withTrimmedLeft(mRowLabelWidth));

    // Viewports
    mRowLabelsViewPort.setBounds(bounds.removeFromLeft(mRowLabelWidth).withHeight(btnsCtnrHeight));
    mColumnLabelsViewPort.setBounds(bounds.removeFromBottom(mColumnLabelHeight));
    mButtonsViewPort.setBounds(mRowLabelWidth + scrollbarSize, 0, btnsCtnrWidth, btnsCtnrHeight);

    // Containers
    auto const numRows = static_cast<int>(mButtons.size());
    auto const numColumns = mButtons.empty() ? 0 : static_cast<int>(mButtons[0].size());

    auto const buttonsWidth = numColumns * mCellWidth;
    auto const buttonsHeight = numRows * mCellHeight;

    mRowLabelsContainer.setSize(mRowLabelWidth, std::max(buttonsHeight, btnsCtnrHeight));
    mColumnLabelsContainer.setSize(std::max(buttonsWidth, mRowLabelWidth), mColumnLabelHeight);
    mButtonsContainer.setSize(std::max(buttonsWidth, mRowLabelWidth), std::max(buttonsHeight, btnsCtnrHeight));

    auto buttonsBounds = mButtonsContainer.getBounds().withZeroOrigin();
    for(auto& row : mButtons)
    {
        auto rowBounds = buttonsBounds.removeFromTop(mCellHeight).reduced(1);
        for(auto& button : row)
        {
            if(button != nullptr)
            {
                button->setBounds(rowBounds.removeFromLeft(mCellWidth).reduced(1));
            }
        }
    }

    auto rowBounds = mRowLabelsContainer.getBounds().withZeroOrigin();
    for(auto& label : mRowLabels)
    {
        if(label != nullptr)
        {
            label->setBounds(rowBounds.removeFromTop(mCellHeight));
        }
    }

    auto columnBounds = mColumnLabelsContainer.getBounds().withZeroOrigin();
    for(auto& label : mColumnLabels)
    {
        if(label != nullptr)
        {
            label->setBounds(columnBounds.removeFromLeft(mCellWidth));
        }
    }

    mRowScrollBar.setRangeLimits(mButtonsViewPort.getVerticalScrollBar().getRangeLimit(), juce::NotificationType::dontSendNotification);
    mColumnScrollBar.setRangeLimits(mButtonsViewPort.getHorizontalScrollBar().getRangeLimit(), juce::NotificationType::dontSendNotification);

    mRowScrollBar.setCurrentRange(mButtonsViewPort.getVerticalScrollBar().getCurrentRange(), juce::NotificationType::dontSendNotification);
    mColumnScrollBar.setCurrentRange(mButtonsViewPort.getHorizontalScrollBar().getCurrentRange(), juce::NotificationType::dontSendNotification);
}

void BooleanMatrixSelector::scrollBarMoved(juce::ScrollBar* scrollBar, double newRangeStart)
{
    if(scrollBar == std::addressof(mRowScrollBar))
    {
        mButtonsViewPort.getVerticalScrollBar().setCurrentRange(newRangeStart, juce::NotificationType::sendNotificationSync);
    }
    else if(scrollBar == std::addressof(mColumnScrollBar))
    {
        mButtonsViewPort.getHorizontalScrollBar().setCurrentRange(newRangeStart, juce::NotificationType::sendNotificationSync);
    }
    else
    {
        MiscWeakAssert(false);
    }
}

void BooleanMatrixSelector::setSize(size_t numRows, size_t numColumns)
{
    mButtons.resize(numRows);
    for(auto row = 0_z; row < mButtons.size(); ++row)
    {
        auto& rowButtons = mButtons[row];
        rowButtons.resize(numColumns);
        for(auto column = 0_z; column < rowButtons.size(); ++column)
        {
            auto& columnButton = rowButtons[column];
            if(columnButton == nullptr)
            {
                columnButton = std::make_unique<juce::ToggleButton>();
            }
            if(columnButton != nullptr)
            {
                columnButton->onClick = [this, row, column]()
                {
                    if(onClick != nullptr)
                    {
                        onClick(row, column);
                    }
                };
                columnButton->setTooltip("Input " + juce::String(row + 1_z) + " to Output " + juce::String(column + 1_z));
                mButtonsContainer.addAndMakeVisible(columnButton.get());
            }
        }
    }

    auto const font = juce::Font(juce::FontOptions("Arial", 10.0f, juce::Font::plain));
    mRowLabels.resize(numRows);
    for(auto i = 0_z; i < mRowLabels.size(); ++i)
    {
        auto& rowLabel = mRowLabels[i];
        if(rowLabel == nullptr)
        {
            rowLabel = std::make_unique<juce::Label>("input", juce::String(i + 1_z) + juce::String(juce::CharPointer_UTF8(" \xe2\x86\x92")));
        }
        if(rowLabel != nullptr)
        {
            rowLabel->setJustificationType(juce::Justification::centredRight);
            rowLabel->setFont(font);
            mRowLabelsContainer.addAndMakeVisible(rowLabel.get());
        }
    }

    mColumnLabels.resize(numColumns);
    for(auto i = 0_z; i < mColumnLabels.size(); ++i)
    {
        auto& columnLabel = mColumnLabels[i];
        if(columnLabel == nullptr)
        {
            columnLabel = std::make_unique<juce::Label>("output", juce::String(juce::CharPointer_UTF8("\xe2\x86\x93\n")) + juce::String(i + 1_z));
        }
        if(columnLabel != nullptr)
        {
            columnLabel->setJustificationType(juce::Justification::centredTop);
            columnLabel->setFont(font);
            mColumnLabelsContainer.addAndMakeVisible(columnLabel.get());
        }
    }

    resized();
}

int BooleanMatrixSelector::getOptimalWidth() const
{
    return mRowLabelWidth + static_cast<int>(std::get<1_z>(getSize())) * mCellWidth + getLookAndFeel().getDefaultScrollbarWidth();
}

int BooleanMatrixSelector::getOptimalHeight() const
{
    return mColumnLabelHeight + static_cast<int>(std::get<0_z>(getSize())) * mCellHeight + getLookAndFeel().getDefaultScrollbarWidth();
}

std::pair<size_t, size_t> BooleanMatrixSelector::getSize() const
{
    return {mButtons.size(), mButtons.empty() ? 0_z : mButtons[0_z].size()};
}

void BooleanMatrixSelector::setCellState(size_t row, size_t column, bool state, juce::NotificationType notification)
{
    MiscWeakAssert(mButtons.at(row).at(column) != nullptr);
    if(mButtons.at(row).at(column) != nullptr)
    {
        mButtons[row][column]->setToggleState(state, notification);
    }
}

bool BooleanMatrixSelector::getCellState(size_t row, size_t column) const
{
    MiscWeakAssert(mButtons.at(row).at(column) != nullptr);
    if(mButtons.at(row).at(column) != nullptr)
    {
        return mButtons.at(row).at(column)->getToggleState();
    }
    return false;
}

MISC_FILE_END
