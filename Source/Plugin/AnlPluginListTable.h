#pragma once

#include "AnlPluginListModel.h"
#include "AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    class Table
    : public juce::Component
    , private juce::TableListBoxModel
    {
    public:
        Table(Accessor& accessor, Scanner& scanner);
        ~Table() override;

        // juce::Component
        void resized() override;
        void colourChanged() override;
        void parentHierarchyChanged() override;
        void visibilityChanged() override;
        bool keyPressed(juce::KeyPress const& key) override;

        std::function<void(std::set<Plugin::Key> keys)> onPluginSelected = nullptr;
        void setMultipleSelectionEnabled(bool shouldBeEnabled) noexcept;

        class WindowContainer
        : public FloatingWindowContainer
        {
        public:
            WindowContainer(Table& table);
            ~WindowContainer() override = default;
        };

    private:
        void updateContent();

        // juce::TableListBoxModel
        int getNumRows() override;
        void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
        void paintCell(juce::Graphics& g, int row, int columnId, int width, int height, bool rowIsSelected) override;
        void returnKeyPressed(int lastRowSelected) override;
        void deleteKeyPressed(int lastRowSelected) override;
        void cellDoubleClicked(int rowNumber, int columnId, juce::MouseEvent const& e) override;
        void sortOrderChanged(int newSortColumnId, bool isForwards) override;
        juce::String getCellTooltip(int rowNumber, int columnId) override;

        Accessor& mAccessor;
        Scanner& mScanner;
        Accessor::Listener mListener{"PluginList::Table"};
        Accessor::Receiver mReceiver;
        std::map<Plugin::Key, Plugin::Description> mList;
        std::vector<std::pair<Plugin::Key, Plugin::Description>> mFilteredList;
        ColouredPanel mSeparator1;
        Misc::Icon mSearchIcon;
        juce::TextEditor mSearchField;
        ColouredPanel mSeparator2;
        juce::TableListBox mPluginTable;
        juce::String mLookingWord;

        JUCE_LEAK_DETECTOR(Table)
    };
} // namespace PluginList

ANALYSE_FILE_END
