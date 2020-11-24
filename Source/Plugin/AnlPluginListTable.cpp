
#include "AnlPluginListTable.h"
#include "AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

PluginList::Table::Table(Accessor& accessor)
: mAccessor(accessor)
, mClearButton(juce::translate("Clear"))
, mScanButton(juce::translate("Scan"))
{
    mListener.onChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        updateContent();
    };
    
    addAndMakeVisible(mPluginTable);
    mPluginTable.setHeaderHeight(28);
    mPluginTable.setRowHeight(26);
    mPluginTable.setMultipleSelectionEnabled(true);
    mPluginTable.setModel(this);
    
    using ColumnFlags = juce::TableHeaderComponent::ColumnPropertyFlags;
    auto& header = mPluginTable.getHeader();
    header.addColumn(juce::translate("Name"), ColumnType::Name, 200, 100, 700, ColumnFlags::defaultFlags | ColumnFlags::sortedForwards);
    header.addColumn(juce::translate("Manufacturer"), ColumnType::Maker, 150, 100, 300);
    header.addColumn(juce::translate("Api"), ColumnType::Api, 40, 40, 40, ColumnFlags::notResizable | ColumnFlags::notSortable);
    header.addColumn(juce::translate("Category"), ColumnType::Category, 80, 100, 200);
    header.addColumn(juce::translate("Description"), ColumnType::Details, 240, 100, 500, ColumnFlags::notSortable);
    
    addAndMakeVisible(mClearButton);
    mClearButton.setClickingTogglesState(false);
    mClearButton.onClick = [this]()
    {
        mAccessor.setValue<AttrType::descriptions>(std::map<juce::String, Description>{}, NotificationType::synchronous);
    };
    
    addAndMakeVisible(mScanButton);
    mScanButton.setClickingTogglesState(false);
    mScanButton.onClick = [this]()
    {
        auto getNewList = []() -> decltype(Scanner::getPluginDescriptions())
        {
            using AlertIconType = juce::AlertWindow::AlertIconType;
            try
            {
                return Scanner::getPluginDescriptions();
            }
            catch(std::runtime_error& e)
            {
                juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, juce::translate("Scan failed!"), juce::translate(e.what()));
            }
            return {};
        };
        
        mAccessor.setValue<AttrType::descriptions>(getNewList(), NotificationType::synchronous);
    };
    
    addAndMakeVisible(mSearchField);
    mSearchField.setMultiLine(false);
    mSearchField.setPopupMenuEnabled(false);
    mSearchField.setCaretVisible(true);
    mSearchField.setJustification(juce::Justification::centredLeft);
    mSearchField.setTextToShowWhenEmpty(juce::translate("Filter..."), juce::Colours::white.withAlpha(0.6f));
    mSearchField.setColour(juce::CaretComponent::caretColourId, juce::Colours::white.withAlpha(0.8f));
    mSearchField.setColour(juce::TextEditor::textColourId, juce::Colours::white.withAlpha(0.8f));
    mSearchField.setColour(juce::TextEditor::backgroundColourId, juce::Colours::black.withAlpha(0.4799997f));
    mSearchField.onTextChange = [this]()
    {
        updateContent();
    };
    mSearchField.setEscapeAndReturnKeysConsumed(true);
    mSearchField.onEscapeKey = [this]()
    {
        mSearchField.setText("");
        moveKeyboardFocusToSibling(false);
        updateContent();
    };
    mSearchField.onReturnKey = [this]()
    {
        moveKeyboardFocusToSibling(false);
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    setSize(800, 600);
}

PluginList::Table::~Table()
{
    mAccessor.removeListener(mListener);
}

void PluginList::Table::resized()
{
    auto bounds = getLocalBounds();
    auto bottom = bounds.removeFromBottom(30);
    bottom.removeFromLeft(4);
    mClearButton.setBounds(bottom.removeFromLeft(100).withSizeKeepingCentre(100, 21));
    bottom.removeFromLeft(4);
    mScanButton.setBounds(bottom.removeFromLeft(100).withSizeKeepingCentre(100, 21));
    bottom.removeFromLeft(4);
    mSearchField.setBounds(bottom.removeFromLeft(200).withSizeKeepingCentre(200, 21));
    mPluginTable.setBounds(bounds);
}

void PluginList::Table::updateContent()
{
    auto const& descriptions = mAccessor.getAttr<AttrType::descriptions>();
    mFilteredList.resize(descriptions.size());
    auto const searchPattern = mSearchField.getText().removeCharacters(" ");
    auto it = std::copy_if(descriptions.cbegin(), descriptions.cend(), mFilteredList.begin(), [&](auto const& pair)
    {
        auto const filterName = (pair.second.name + pair.second.maker).removeCharacters(" ");
        return searchPattern.isEmpty() || filterName.containsIgnoreCase(searchPattern);
    });
    mFilteredList.resize(static_cast<size_t>(std::distance(mFilteredList.begin(), it)));
        
    auto const isForwards = mAccessor.getAttr<AttrType::sortIsFowards>();
    switch (mAccessor.getAttr<AttrType::sortColumn>())
    {
        case ColumnType::Name:
        {
            std::sort(mFilteredList.begin(), mFilteredList.end(), [&](auto const& lhs, auto const& rhs)
            {
                return (lhs.second.name > rhs.second.name) == isForwards;
            });
        }
            break;
        case ColumnType::Maker:
        {
            std::sort(mFilteredList.begin(), mFilteredList.end(), [&](auto const& lhs, auto const& rhs)
            {
                return (lhs.second.maker > rhs.second.maker) == isForwards;
            });
        }
            break;
        case ColumnType::Category:
        {
            std::sort(mFilteredList.begin(), mFilteredList.end(), [&](auto const& lhs, auto const& rhs)
            {
                return (lhs.second.name > rhs.second.name) == isForwards;
            });
        }
            break;
        case ColumnType::Api:
        case ColumnType::Details:
            break;
    }
    
    mPluginTable.getHeader().reSortTable();
    mPluginTable.updateContent();
    mPluginTable.repaint();
}

int PluginList::Table::getNumRows()
{
    return static_cast<int>(mFilteredList.size());
}

void PluginList::Table::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    juce::ignoreUnused(rowNumber, width, height);
    const auto defaultColour = mPluginTable.findColour(juce::ListBox::backgroundColourId);
    const auto selectedColour = defaultColour.interpolatedWith(mPluginTable.findColour(juce::ListBox::textColourId), 0.5f);
    g.fillAll(rowIsSelected ? selectedColour : defaultColour);
}

void PluginList::Table::paintCell(juce::Graphics& g, int row, int columnId, int width, int height, bool rowIsSelected)
{
    juce::ignoreUnused(rowIsSelected);
    if(row >= static_cast<int>(mFilteredList.size()))
    {
        return;
    }
    auto const index = static_cast<size_t>(row);
    auto getText = [&]() -> juce::String
    {
        switch (columnId)
        {
            case ColumnType::Name:
                return mFilteredList[index].second.name;
            case ColumnType::Maker:
                return mFilteredList[index].second.maker;
            case ColumnType::Api:
                return juce::String(mFilteredList[index].second.api);
            case ColumnType::Details:
                return mFilteredList[index].second.details;
            case ColumnType::Category:
                return mFilteredList[index].second.categories.empty() ? "-" : *(mFilteredList[index].second.categories.begin());
        }
        return "";
    };
    
    const auto defaultTextColour = mPluginTable.findColour(juce::ListBox::textColourId);
    g.setColour(columnId == ColumnType::Name ? defaultTextColour : defaultTextColour.interpolatedWith(juce::Colours::transparentBlack, 0.3f));
    g.setFont(juce::Font(height * 0.7f, juce::Font::bold));
    g.drawFittedText(getText(), 4, 0, width - 6, height, juce::Justification::centredLeft, 1, 1.f);
}

void PluginList::Table::deleteKeyPressed(int lastRowSelected)
{
    juce::ignoreUnused(lastRowSelected);
    
    auto const selectedRows = mPluginTable.getSelectedRows();
    mPluginTable.deselectAllRows();
    
    auto description = mAccessor.getAttr<AttrType::descriptions>();
    auto const numSelectedRows = selectedRows.size();
    for (int i = 0; i < numSelectedRows; ++i)
    {
        auto const index = selectedRows[i];
        if(index < static_cast<int>(mFilteredList.size()))
        {
            auto const entry = mFilteredList[static_cast<size_t>(index)].first;
            description.erase(entry);
        }
    }
    mAccessor.setValue<AttrType::descriptions>(description, NotificationType::synchronous);
}

void PluginList::Table::returnKeyPressed(int lastRowSelected)
{
    if(onPluginSelected && lastRowSelected < static_cast<int>(mFilteredList.size()))
    {
        onPluginSelected(mFilteredList[static_cast<size_t>(lastRowSelected)].first);
    }
}

void PluginList::Table::cellDoubleClicked(int rowNumber, int columnId, juce::MouseEvent const& e)
{
    juce::ignoreUnused(columnId, e);
    returnKeyPressed(rowNumber);
}

void PluginList::Table::sortOrderChanged(int newSortColumnId, bool isForwards)
{
    mAccessor.setValue<AttrType::sortColumn>(static_cast<ColumnType>(newSortColumnId), NotificationType::synchronous);
    mAccessor.setValue<AttrType::sortIsFowards>(isForwards, NotificationType::synchronous);
}

ANALYSE_FILE_END
