#pragma once

#include "AnlTrackResultModifier.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    namespace Result
    {
        class Table
        : public juce::Component
        , private juce::TableListBoxModel
        , private juce::ChangeListener
        {
        public:
            Table(Director& director, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAcsr);
            ~Table() override;

            // juce::Component
            void resized() override;
            void visibilityChanged() override;
            void parentHierarchyChanged() override;
            bool keyPressed(juce::KeyPress const& key) override;

            class WindowContainer
            : public FloatingWindowContainer
            {
            public:
                WindowContainer(Table& table);
                ~WindowContainer() override;

            private:
                Table& mTable;
                Accessor::Listener mListener{typeid(*this).name()};
                juce::TooltipWindow mTooltip;
            };

        private:
            // juce::TableListBoxModel
            int getNumRows() override;
            void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
            void paintCell(juce::Graphics& g, int row, int columnId, int width, int height, bool rowIsSelected) override;
            juce::Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
            void deleteKeyPressed(int lastRowSelected) override;
            void selectedRowsChanged(int lastRowSelected) override;
            juce::String getCellTooltip(int rowNumber, int columnId) override;

            // juce::ChangeListener
            void changeListenerCallback(juce::ChangeBroadcaster* source) override;

            std::optional<size_t> getSelectedChannel() const;
            std::optional<size_t> getPlayheadRow() const;
            bool deleteSelection();
            bool copySelection();
            bool cutSelection();
            bool pasteSelection();
            bool duplicateSelection();
            void selectionUpdated();

            // clang-format off
            enum class ColumnType
            {
                  index = 1
                , time
                , duration
                , value
                , extra
            };
            // clang-format on

            class Cell;
            class TimeCell;
            class DurationCell;
            class ValueCell;

            Director& mDirector;
            Accessor& mAccessor{mDirector.getAccessor()};
            Accessor::Listener mListener{typeid(*this).name()};
            Transport::Accessor& mTransportAccessor;
            Transport::Accessor::Listener mTransportListener{typeid(*this).name()};
            Zoom::Accessor& mTimeZoomAccessor;
            Zoom::Accessor::Listener mTimeZoomListener{typeid(*this).name()};

            juce::TabbedButtonBar mTabbedButtonBar{juce::TabbedButtonBar::Orientation::TabsAtBottom};
            ColouredPanel mSeparator;
            juce::TableListBox mTable;
            ChannelData mChannelData;
            std::optional<size_t> mPreviousPlayheadRow;

            JUCE_LEAK_DETECTOR(Table)
        };
    } // namespace Result
} // namespace Track

ANALYSE_FILE_END
