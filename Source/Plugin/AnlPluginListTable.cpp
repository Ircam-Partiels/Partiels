#include "AnlPluginListTable.h"

ANALYSE_FILE_BEGIN

PluginList::Table::Table(Accessor& accessor, Scanner& scanner)
: FloatingWindowContainer("Add Plugin...", *this)
, mAccessor(accessor)
, mScanner(scanner)
, mClearButton(juce::translate("Clear"))
, mScanButton(juce::translate("Scan"))
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        mIsBeingUpdated = true;
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::keys:
                break;
            case AttrType::sortColumn:
            case AttrType::sortIsFowards:
            {
                auto const columnId = static_cast<int>(acsr.getAttr<AttrType::sortColumn>());
                auto const isForward = acsr.getAttr<AttrType::sortIsFowards>();
                mPluginTable.getHeader().setSortColumnId(columnId, isForward);
            }
            break;
        }
        updateContent();
        mIsBeingUpdated = false;
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
    header.addColumn(juce::translate("Category"), ColumnType::Category, 60, 60, 200);
    header.addColumn(juce::translate("Version"), ColumnType::Version, 44, 44, 44, ColumnFlags::notResizable | ColumnFlags::notSortable);

    addAndMakeVisible(mSeparator);

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
        auto scanPlugins = [this]() -> decltype(mScanner.getKeys(48000.0))
        {
            try
            {
                return mScanner.getKeys(48000.0);
            }
            catch(std::exception& e)
            {
                AlertWindow::showMessage(AlertWindow::MessageType::warning,
                                         "Plugins scan failed!", e.what());
            }
            catch(...)
            {
                AlertWindow::showMessage(AlertWindow::MessageType::warning,
                                         "Plugins scan failed!", "");
            }
            return {};
        };
        auto const results = scanPlugins();
        mAccessor.setAttr<AttrType::keys>(std::get<0>(results), NotificationType::synchronous);
        if(!std::get<1>(results).isEmpty())
        {
            AlertWindow::showMessage(AlertWindow::MessageType::warning,
                                     "Plugins scan has encountered errors!",
                                     "The following plugins failed to be scanned:\n" + std::get<1>(results).joinIntoString("\n"));
        }
    };

    addAndMakeVisible(mSearchField);
    mSearchField.setMultiLine(false);
    mSearchField.setPopupMenuEnabled(false);
    mSearchField.setCaretVisible(true);
    mSearchField.setJustification(juce::Justification::centredLeft);
    lookAndFeelChanged();
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
    setSize(820, 600);
}

PluginList::Table::~Table()
{
    mAccessor.removeListener(mListener);
}

void PluginList::Table::resized()
{
    auto bounds = getLocalBounds();
    auto bottom = bounds.removeFromBottom(31);
    mSeparator.setBounds(bottom.removeFromTop(1));
    bottom.removeFromLeft(4);
    mClearButton.setBounds(bottom.removeFromLeft(100).withSizeKeepingCentre(100, 21));
    bottom.removeFromLeft(4);
    mScanButton.setBounds(bottom.removeFromLeft(100).withSizeKeepingCentre(100, 21));
    bottom.removeFromLeft(4);
    mSearchField.setBounds(bottom.removeFromLeft(200).withSizeKeepingCentre(200, 21));
    mPluginTable.setBounds(bounds);
}

void PluginList::Table::lookAndFeelChanged()
{
    mSearchField.setTextToShowWhenEmpty(juce::translate("Filter..."), getLookAndFeel().findColour(juce::TableListBox::ColourIds::textColourId));
}

void PluginList::Table::parentHierarchyChanged()
{
    lookAndFeelChanged();
}

void PluginList::Table::visibilityChanged()
{
    lookAndFeelChanged();
}

void PluginList::Table::showAt(juce::Point<int> const& pt)
{
    FloatingWindowContainer::showAt(pt);
    mFloatingWindow.runModalLoop();
}

void PluginList::Table::updateContent()
{
    auto const& keys = mAccessor.getAttr<AttrType::keys>();

    mFilteredList.clear();
    juce::StringArray errors;
    auto const searchPattern = mSearchField.getText().removeCharacters(" ");
    for(auto const& key : keys)
    {
        try
        {
            if(mBlacklist.count(key) == 0)
            {
                auto const description = mScanner.getDescription(key, 48000.0);
                if(description.name.isNotEmpty())
                {
                    auto const filterName = (description.name + description.output.name + description.maker).removeCharacters(" ");
                    if(searchPattern.isEmpty() || filterName.containsIgnoreCase(searchPattern))
                    {
                        mFilteredList.push_back({key, description});
                    }
                }
            }
        }
        catch(std::exception& e)
        {
            mBlacklist.insert(key);
            errors.add(key.identifier + " - " + key.feature + ": " + e.what());
        }
        catch(...)
        {
            mBlacklist.insert(key);
            errors.add(key.identifier + " - " + key.feature + ": unknown error");
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

    if(!errors.isEmpty())
    {
        AlertWindow::showMessage(AlertWindow::MessageType::warning,
                                 "Plugins listing has encountered errors!",
                                 "The following plugins failed to be scanned:\n" + errors.joinIntoString("\n"));
    }
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
            {
                auto const position = description.details.indexOf("\n");
                return description.details.substring(0, position > 0 ? position : description.details.length());
            }
            case ColumnType::Category:
                return description.category.isEmpty() ? "-" : description.category;
        }
        return "";
    };

    const auto defaultTextColour = mPluginTable.findColour(juce::ListBox::textColourId);
    g.setColour(columnId == ColumnType::Name || columnId == ColumnType::Feature ? defaultTextColour : defaultTextColour.interpolatedWith(juce::Colours::transparentBlack, 0.3f));
    g.setFont(juce::Font(static_cast<float>(height) * 0.7f, juce::Font::bold));
    g.drawText(getText(), 4, 0, width - 6, height, juce::Justification::centredLeft, false);
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
    if(!mIsBeingUpdated)
    {
        mAccessor.setAttr<AttrType::sortColumn>(static_cast<ColumnType>(newSortColumnId), NotificationType::synchronous);
        mAccessor.setAttr<AttrType::sortIsFowards>(isForwards, NotificationType::synchronous);
    }
}

juce::String PluginList::Table::getCellTooltip(int rowNumber, int columnId)
{
    juce::ignoreUnused(columnId);
    if(rowNumber >= static_cast<int>(mFilteredList.size()))
    {
        return "";
    }
    auto const index = static_cast<size_t>(rowNumber);
    auto const& description = mFilteredList[index].second;
    return description.details;
}

ANALYSE_FILE_END
