#pragma once

#include "../Misc/AnlBasics.h"
#include "AnlPluginListModel.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    class Table
    : public juce::Component
    , private juce::TableListBoxModel
    {
    public:
        
        Table(Accessor& accessor);
        ~Table() override;
        
        // juce::Component
        void resized() override;
        
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
        Accessor::Listener mListener;
        std::vector<std::pair<Plugin::Key, Plugin::Description>> mFilteredList;
        juce::TableListBox mPluginTable;
        juce::TextButton mClearButton;
        juce::TextButton mScanButton;
        juce::TextEditor mSearchField;
        juce::String mLookingWord;
        
        JUCE_LEAK_DETECTOR(Table)
    };
}

ANALYSE_FILE_END
