
#include "AnlPluginListTable.h"
#include "AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

PluginList::Table::Table(Accessor& accessor)
: mAccessor(accessor)
, mClearButton(juce::translate("Clear"))
, mScanButton(juce::translate("Scan"))
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        updateContent();
    };
    
    addAndMakeVisible(mPluginTable);
    mPluginTable.setHeaderHeight(28);
    mPluginTable.setRowHeight(22);
    mPluginTable.setMultipleSelectionEnabled(true);
    mPluginTable.setModel(this);
    
    using ColumnFlags = juce::TableHeaderComponent::ColumnPropertyFlags;
    auto& header = mPluginTable.getHeader();
    header.addColumn(juce::translate("Name"), ColumnType::Name, 170, 100, 700, ColumnFlags::defaultFlags | ColumnFlags::sortable);
    header.addColumn(juce::translate("Feature"), ColumnType::Feature, 200, 100, 700, ColumnFlags::defaultFlags | ColumnFlags::sortable);
    header.addColumn(juce::translate("Description"), ColumnType::Details, 200, 100, 500, ColumnFlags::notSortable);
    header.addColumn(juce::translate("Maker"), ColumnType::Maker, 120, 100, 300);
    header.addColumn(juce::translate("Category"), ColumnType::Category, 60, 100, 200);
    header.addColumn(juce::translate("Version"), ColumnType::Version, 40, 40, 40, ColumnFlags::notResizable | ColumnFlags::notSortable);
    
    addAndMakeVisible(mClearButton);
    mClearButton.setClickingTogglesState(false);
    mClearButton.onClick = [this]()
    {
        mAccessor.setAttr<AttrType::keys>(decltype(mAccessor.getAttr<AttrType::keys>()){}, NotificationType::synchronous);
    };
    
    addAndMakeVisible(mScanButton);
    mScanButton.setClickingTogglesState(false);
    mScanButton.onClick = [this]()
    {
        auto getNewList = []() -> decltype(Scanner::getPluginKeys(48000.0, AlertType::window))
        {
            try
            {
                return Scanner::getPluginKeys(48000.0, AlertType::window);
            }
            catch(std::runtime_error& e)
            {
                using AlertIconType = juce::AlertWindow::AlertIconType;
                juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, juce::translate("Scan failed!"), juce::translate(e.what()));
            }
            return {};
        };
        
        mAccessor.setAttr<AttrType::keys>(getNewList(), NotificationType::synchronous);
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
    auto const& keys = mAccessor.getAttr<AttrType::keys>();
    
    mFilteredList.clear();
    auto const searchPattern = mSearchField.getText().removeCharacters(" ");
    for(auto const& key : keys)
    {
        auto const description = Scanner::getPluginDescription(key, 48000.0, AlertType::window);
        if(description.name.isNotEmpty())
        {
            auto const filterName = (description.name + description.output.name + description.maker).removeCharacters(" ");
            if(searchPattern.isEmpty() || filterName.containsIgnoreCase(searchPattern))
            {
                mFilteredList.push_back({key, description});
            }
        }
    }
        
    auto const isForwards = mAccessor.getAttr<AttrType::sortIsFowards>();
    switch(mAccessor.getAttr<AttrType::sortColumn>())
    {
        case ColumnType::Name:
        {
            std::sort(mFilteredList.begin(), mFilteredList.end(), [&](auto const& lhs, auto const& rhs)
            {
                return isForwards ? (lhs.second.name + lhs.second.output.name > rhs.second.name + rhs.second.output.name) : (lhs.second.name + lhs.second.output.name < rhs.second.name + rhs.second.output.name);
            });
        }
            break;
        case ColumnType::Feature:
        {
            std::sort(mFilteredList.begin(), mFilteredList.end(), [&](auto const& lhs, auto const& rhs)
            {
                return isForwards ? (lhs.second.output.name > rhs.second.output.name) : (lhs.second.output.name < rhs.second.output.name);
            });
        }
            break;
        case ColumnType::Maker:
        {
            std::sort(mFilteredList.begin(), mFilteredList.end(), [&](auto const& lhs, auto const& rhs)
            {
                return isForwards ? (lhs.second.maker > rhs.second.maker) : (lhs.second.maker < rhs.second.maker);
            });
        }
            break;
        case ColumnType::Category:
        {
            std::sort(mFilteredList.begin(), mFilteredList.end(), [&](auto const& lhs, auto const& rhs)
            {
                return isForwards ? (lhs.second.category > rhs.second.category) : (lhs.second.category < rhs.second.category);
            });
        }
            break;
        case ColumnType::Version:
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
    auto const& description = mFilteredList[index].second;
    auto getText = [&]() -> juce::String
    {
        switch(columnId)
        {
            case ColumnType::Name:
                return description.name;
            case ColumnType::Feature:
                return description.output.name;
            case ColumnType::Maker:
                return description.maker;
            case ColumnType::Version:
                return juce::String(description.version);
            case ColumnType::Details:
                return description.details;
            case ColumnType::Category:
                return description.category.isEmpty() ? "-" : description.category;
        }
        return "";
    };
    
    const auto defaultTextColour = mPluginTable.findColour(juce::ListBox::textColourId);
    g.setColour(columnId == ColumnType::Name || columnId == ColumnType::Feature ? defaultTextColour : defaultTextColour.interpolatedWith(juce::Colours::transparentBlack, 0.3f));
    g.setFont(juce::Font(static_cast<float>(height) * 0.7f, juce::Font::bold));
    g.drawFittedText(getText(), 4, 0, width - 6, height, juce::Justification::centredLeft, 1, 1.f);
}

void PluginList::Table::deleteKeyPressed(int lastRowSelected)
{
    juce::ignoreUnused(lastRowSelected);
    auto const selectedRows = mPluginTable.getSelectedRows();
    mPluginTable.deselectAllRows();
    
    auto keys = mAccessor.getAttr<AttrType::keys>();
    for(int i = 0; i < selectedRows.size(); ++i)
    {
        auto const index = selectedRows[i];
        anlWeakAssert(index >= 0 && static_cast<size_t>(index) < mFilteredList.size());
        if(index >= 0 && static_cast<size_t>(index) < mFilteredList.size())
        {
            auto const key = mFilteredList[static_cast<size_t>(index)].first;
            keys.erase(key);
        }
    }
    
    mAccessor.setAttr<AttrType::keys>(keys, NotificationType::synchronous);
}

void PluginList::Table::returnKeyPressed(int lastRowSelected)
{
    if(lastRowSelected < 0 || lastRowSelected >= static_cast<int>(mFilteredList.size()))
    {
        return;
    }
    if(onPluginSelected != nullptr)
    {
        onPluginSelected(mFilteredList[static_cast<size_t>(lastRowSelected)].first, mFilteredList[static_cast<size_t>(lastRowSelected)].second);
    }
}

void PluginList::Table::cellDoubleClicked(int rowNumber, int columnId, juce::MouseEvent const& e)
{
    juce::ignoreUnused(columnId, e);
    returnKeyPressed(rowNumber);
}

void PluginList::Table::sortOrderChanged(int newSortColumnId, bool isForwards)
{
    mAccessor.setAttr<AttrType::sortColumn>(static_cast<ColumnType>(newSortColumnId), NotificationType::synchronous);
    mAccessor.setAttr<AttrType::sortIsFowards>(isForwards, NotificationType::synchronous);
}

ANALYSE_FILE_END
