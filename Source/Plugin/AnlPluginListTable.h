#pragma once

#include "AnlPluginListModel.h"
#include "AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    class Table
    : public FloatingWindowContainer
    , private juce::TableListBoxModel
    {
    public:
        Table(Accessor& accessor, Scanner& scanner);
        ~Table() override;

        // juce::Component
        void resized() override;
        void lookAndFeelChanged() override;
        void parentHierarchyChanged() override;
        void visibilityChanged() override;

        // FloatingWindowContainer
        void showAt(juce::Point<int> const& pt) override;
        void hide() override;

        std::function<void(Plugin::Key const& key, Plugin::Description const& description)> onPluginSelected = nullptr;

    private:
        void updateContent();

        // juce::TableListBoxModel
        int getNumRows() override;
        void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
        void paintCell(juce::Graphics& g, int row, int columnId, int width, int height, bool rowIsSelected) override;
        void returnKeyPressed(int lastRowSelected) override;
        void cellDoubleClicked(int rowNumber, int columnId, juce::MouseEvent const& e) override;
        void sortOrderChanged(int newSortColumnId, bool isForwards) override;
        juce::String getCellTooltip(int rowNumber, int columnId) override;

        Accessor& mAccessor;
        Scanner& mScanner;
        Accessor::Listener mListener{"PluginList::Table"};
        std::map<Plugin::Key, Plugin::Description> mList;
        std::vector<std::pair<Plugin::Key, Plugin::Description>> mFilteredList;
        juce::TableListBox mPluginTable;
        ColouredPanel mSeparator;
        juce::TextEditor mSearchField;
        juce::TextButton mPathsButton;
        juce::String mLookingWord;
        bool mIsBeingUpdated = false;

        juce::TooltipWindow mTooltipWindow{this};
        JUCE_LEAK_DETECTOR(Table)
    };
} // namespace PluginList

ANALYSE_FILE_END
