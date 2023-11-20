#pragma once

#include "AnlPluginDescriptionPanel.h"
#include "AnlPluginListModel.h"
#include "AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    class Table
    : public juce::Component
    , private juce::ListBoxModel
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

        std::function<void(std::set<Plugin::Key> keys)> onAddPlugins = nullptr;
        void setMultipleSelectionEnabled(bool shouldBeEnabled) noexcept;

    private:
        void updateContent();
        void notifyAddSelectedPlugins();

        // juce::TableListBoxModel
        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void returnKeyPressed(int lastRowSelected) override;
        void deleteKeyPressed(int lastRowSelected) override;
        void selectedRowsChanged(int lastRowSelected) override;
        void listBoxItemDoubleClicked(int rowNumber, juce::MouseEvent const& event) override;
        juce::String getTooltipForRow(int rowNumber) override;

        Accessor& mAccessor;
        Scanner& mScanner;
        Accessor::Listener mListener{"PluginList::Table"};
        Accessor::Receiver mReceiver;
        std::map<Plugin::Key, Plugin::Description> mList;
        std::vector<std::pair<Plugin::Key, Plugin::Description>> mFilteredList;
        juce::String mLookingWord;

        Misc::Icon mSearchIcon;
        juce::TextEditor mSearchField;
        Misc::Icon mIrcamIcon;
        ColouredPanel mSeparator1;
        juce::ListBox mPluginTable;
        ColouredPanel mSeparator2;
        Plugin::DescriptionPanel mDescriptionPanel;

        JUCE_LEAK_DETECTOR(Table)
    };
} // namespace PluginList

ANALYSE_FILE_END
