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
        void lookAndFeelChanged() override;
        void parentHierarchyChanged() override;
        void visibilityChanged() override;
        
        std::function<void(Plugin::Key const& key, Plugin::Description const& description)> onPluginSelected = nullptr;
        
    private:
        void updateContent();
        
        // juce::TableListBoxModel
        int getNumRows() override;
        void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
        void paintCell(juce::Graphics& g, int row, int columnId, int width, int height, bool rowIsSelected) override;
        void deleteKeyPressed(int lastRowSelected) override;
        void returnKeyPressed(int lastRowSelected) override;
        void cellDoubleClicked (int rowNumber, int columnId, juce::MouseEvent const& e) override;
        void sortOrderChanged(int newSortColumnId, bool isForwards) override;
        
        Accessor& mAccessor;
        Scanner& mScanner;
        Accessor::Listener mListener;
        std::vector<std::pair<Plugin::Key, Plugin::Description>> mFilteredList;
        juce::TableListBox mPluginTable;
        ColouredPanel mSeparator;
        juce::TextButton mClearButton;
        juce::TextButton mScanButton;
        juce::TextEditor mSearchField;
        juce::String mLookingWord;
        bool mIsBeingUpdated = false;
        std::set<Plugin::Key> mBlacklist;
        
        JUCE_LEAK_DETECTOR(Table)
    };
}

ANALYSE_FILE_END
