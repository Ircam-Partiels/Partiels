#include "AnlPluginListTable.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

PluginList::Table::WindowContainer::WindowContainer(Table& table)
: FloatingWindowContainer(juce::translate("Add Plugins..."), table)
{
    mFloatingWindow.setResizable(false, false);
    mBoundsConstrainer.setSizeLimits(820, 600, 820, 600);
    mFloatingWindow.setConstrainer(&mBoundsConstrainer);
}

PluginList::Table::Table(Accessor& accessor, Scanner& scanner)
: mAccessor(accessor)
, mScanner(scanner)
, mSearchIcon(juce::ImageCache::getFromMemory(AnlIconsData::search_png, AnlIconsData::search_pngSize))
, mSettingsIcon(juce::ImageCache::getFromMemory(AnlIconsData::settings_png, AnlIconsData::settings_pngSize))
{
    addAndMakeVisible(mSeparator1);
    addAndMakeVisible(mSearchIcon);
    addAndMakeVisible(mSearchField);
    addAndMakeVisible(mSettingsIcon);
    addAndMakeVisible(mSeparator2);
    addAndMakeVisible(mPluginTable);

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::useEnvVariable:
            case AttrType::quarantineMode:
            case AttrType::searchPath:
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
    };

    mPluginTable.setHeaderHeight(28);
    mPluginTable.setRowHeight(22);
    mPluginTable.setMultipleSelectionEnabled(true);
    mPluginTable.setModel(this);

    using ColumnFlags = juce::TableHeaderComponent::ColumnPropertyFlags;
    auto& header = mPluginTable.getHeader();
    auto constexpr defaultColumnFlags = ColumnFlags::visible | ColumnFlags::sortable;
    header.addColumn(juce::translate("Name"), ColumnType::name, 170, 100, 700, defaultColumnFlags);
    header.addColumn(juce::translate("Feature"), ColumnType::feature, 200, 100, 700, defaultColumnFlags);
    header.addColumn(juce::translate("Description"), ColumnType::details, 200, 100, 500, ColumnFlags::visible);
    header.addColumn(juce::translate("Maker"), ColumnType::maker, 120, 100, 300, defaultColumnFlags);
    header.addColumn(juce::translate("Category"), ColumnType::category, 60, 60, 200, defaultColumnFlags);
    header.addColumn(juce::translate("Version"), ColumnType::version, 44, 44, 44, ColumnFlags::visible);

    mSearchIcon.setTooltip(juce::translate("Search for a plugin by name, feature, description or maker"));
    mSearchIcon.onClick = [this]()
    {
        mSearchField.grabKeyboardFocus();
    };
    mSettingsIcon.setTooltip(juce::translate("Shows the plugin settings panel"));
    mSettingsIcon.onClick = [this]()
    {
        if(onSettingButtonClicked)
        {
            onSettingButtonClicked();
        }
    };
    mSearchField.setMultiLine(false);
    mSearchField.setPopupMenuEnabled(false);
    mSearchField.setCaretVisible(true);
    mSearchField.setJustification(juce::Justification::centredLeft);
    mSearchField.setBorder({});
    mSearchField.setIndents(0, 0);
    mSearchField.onTextChange = [this]()
    {
        updateContent();
    };
    mSearchField.setEscapeAndReturnKeysConsumed(false);
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

    mReceiver.onSignal = [&](Accessor const& acsr, SignalType signal, juce::var value)
    {
        juce::ignoreUnused(acsr, value);
        switch(signal)
        {
            case SignalType::rescan:
            {
                auto scanPlugins = [this]() -> decltype(mScanner.getPlugins(48000.0))
                {
                    auto showWarning = [](juce::String const& message)
                    {
                        auto const options = juce::MessageBoxOptions()
                                                 .withIconType(juce::AlertWindow::WarningIcon)
                                                 .withTitle(juce::translate("Plugins scan failed!"))
                                                 .withMessage(juce::translate("Partiels failed to scan the plugins due to: MESSAGE.").replace("MESSAGE", message))
                                                 .withButton(juce::translate("Ok"));
                        juce::AlertWindow::showAsync(options, nullptr);
                    };
                    try
                    {
                        return mScanner.getPlugins(48000.0);
                    }
                    catch(std::exception& e)
                    {
                        showWarning(juce::translate(e.what()));
                    }
                    catch(...)
                    {
                        showWarning(juce::translate("Unknwon reason"));
                    }
                    return {};
                };
                auto const results = scanPlugins();
                mList = std::get<0>(results);
                if(!std::get<1>(results).isEmpty())
                {
                    auto const options = juce::MessageBoxOptions()
                                             .withIconType(juce::AlertWindow::WarningIcon)
                                             .withTitle(juce::translate("Plugins scan has encountered errors!"))
                                             .withMessage(juce::translate("The following plugins failed to be scanned:\n.") + std::get<1>(results).joinIntoString("\n"))
                                             .withButton(juce::translate("Ok"));
                    juce::AlertWindow::showAsync(options, nullptr);
                }
                updateContent();
            }
            break;
        }
    };
    mReceiver.onSignal(mAccessor, SignalType::rescan, {});
    mAccessor.addReceiver(mReceiver);

    setWantsKeyboardFocus(true);
    setSize(820, 600);
}

PluginList::Table::~Table()
{
    mAccessor.removeReceiver(mReceiver);
    mAccessor.removeListener(mListener);
}

void PluginList::Table::resized()
{
    auto bounds = getLocalBounds();
    mSeparator1.setBounds(bounds.removeFromTop(1));
    auto headerBar = bounds.removeFromTop(20);
    mSearchIcon.setBounds(headerBar.removeFromLeft(20).reduced(2));
    mSettingsIcon.setBounds(headerBar.removeFromRight(20).reduced(2));
    mSearchField.setBounds(headerBar.removeFromLeft(202).reduced(1));
    mSeparator2.setBounds(bounds.removeFromTop(1));
    mPluginTable.setBounds(bounds);
}

void PluginList::Table::colourChanged()
{
    auto const text = mSearchField.getText();
    mSearchField.clear();
    mSearchField.setText(text);
    mSearchField.setTextToShowWhenEmpty(juce::translate("Search..."), getLookAndFeel().findColour(juce::TableListBox::ColourIds::textColourId));
}

void PluginList::Table::parentHierarchyChanged()
{
    colourChanged();
}

void PluginList::Table::visibilityChanged()
{
    colourChanged();
}

bool PluginList::Table::keyPressed(juce::KeyPress const& key)
{
    if(key == juce::KeyPress('f', juce::ModifierKeys::commandModifier, 0))
    {
        mSearchField.grabKeyboardFocus();
        return true;
    }
    return false;
}

void PluginList::Table::updateContent()
{
    mFilteredList.clear();
    auto const searchPattern = mSearchField.getText().removeCharacters(" ");
    for(auto const& plugin : mList)
    {
        auto const& description = plugin.second;
        if(description.name.isNotEmpty())
        {
            auto const filterName = (description.name + description.output.name + description.maker + description.details).removeCharacters(" ");
            if(searchPattern.isEmpty() || filterName.containsIgnoreCase(searchPattern))
            {
                mFilteredList.push_back(plugin);
            }
        }
    }

    auto const isForwards = mAccessor.getAttr<AttrType::sortIsFowards>();
    switch(mAccessor.getAttr<AttrType::sortColumn>())
    {
        case ColumnType::name:
        {
            std::sort(mFilteredList.begin(), mFilteredList.end(), [&](auto const& lhs, auto const& rhs)
                      {
                          return isForwards ? (lhs.second.name + lhs.second.output.name > rhs.second.name + rhs.second.output.name) : (lhs.second.name + lhs.second.output.name < rhs.second.name + rhs.second.output.name);
                      });
        }
        break;
        case ColumnType::feature:
        {
            std::sort(mFilteredList.begin(), mFilteredList.end(), [&](auto const& lhs, auto const& rhs)
                      {
                          return isForwards ? (lhs.second.output.name > rhs.second.output.name) : (lhs.second.output.name < rhs.second.output.name);
                      });
        }
        break;
        case ColumnType::maker:
        {
            std::sort(mFilteredList.begin(), mFilteredList.end(), [&](auto const& lhs, auto const& rhs)
                      {
                          return isForwards ? (lhs.second.maker > rhs.second.maker) : (lhs.second.maker < rhs.second.maker);
                      });
        }
        break;
        case ColumnType::category:
        {
            std::sort(mFilteredList.begin(), mFilteredList.end(), [&](auto const& lhs, auto const& rhs)
                      {
                          return isForwards ? (lhs.second.category > rhs.second.category) : (lhs.second.category < rhs.second.category);
                      });
        }
        break;
        case ColumnType::version:
        case ColumnType::details:
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
            case ColumnType::name:
                return description.name;
            case ColumnType::feature:
                return description.output.name;
            case ColumnType::maker:
                return description.maker;
            case ColumnType::version:
                return juce::String(description.version);
            case ColumnType::details:
            {
                auto const position = description.details.indexOf("\n");
                return description.details.substring(0, position > 0 ? position : description.details.length());
            }
            case ColumnType::category:
                return description.category.isEmpty() ? "-" : description.category;
        }
        return "";
    };

    const auto defaultTextColour = mPluginTable.findColour(juce::ListBox::textColourId);
    g.setColour(columnId == ColumnType::name || columnId == ColumnType::feature ? defaultTextColour : defaultTextColour.interpolatedWith(juce::Colours::transparentBlack, 0.3f));
    g.setFont(juce::Font(static_cast<float>(height) * 0.7f));
    g.drawText(getText(), 4, 0, width - 6, height, juce::Justification::centredLeft, false);
}

void PluginList::Table::returnKeyPressed(int lastRowSelected)
{
    if(onPluginSelected == nullptr)
    {
        return;
    }
    if(lastRowSelected < 0 || lastRowSelected >= static_cast<int>(mFilteredList.size()))
    {
        return;
    }
    auto const indices = mPluginTable.getSelectedRows();
    std::set<Plugin::Key> selection;
    for(int i = 0; i < indices.size(); ++i)
    {
        auto const index = indices[i];
        MiscWeakAssert(index >= 0 && static_cast<size_t>(index) < mFilteredList.size());
        if(index >= 0 && static_cast<size_t>(index) < mFilteredList.size())
        {
            selection.insert(mFilteredList[static_cast<size_t>(index)].first);
        }
    }
    onPluginSelected(std::move(selection));
}

void PluginList::Table::deleteKeyPressed(int lastRowSelected)
{
    juce::ignoreUnused(lastRowSelected);
    getLookAndFeel().playAlertSound();
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

void PluginList::Table::setMultipleSelectionEnabled(bool shouldBeEnabled) noexcept
{
    mPluginTable.setMultipleSelectionEnabled(shouldBeEnabled);
}

ANALYSE_FILE_END
