
#include "AnlPluginListTable.h"
#include "AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

PluginList::Table::FeatureDescription::FeatureDescription(juce::String k, Description const& description, size_t index)
: Description(description)
, key(k)
, feature(index)
{
}

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
    mPluginTable.setRowHeight(26);
    mPluginTable.setMultipleSelectionEnabled(true);
    mPluginTable.setModel(this);
    
    using ColumnFlags = juce::TableHeaderComponent::ColumnPropertyFlags;
    auto& header = mPluginTable.getHeader();
    header.addColumn(juce::translate("Plugin"), ColumnType::Plugin, 200, 100, 700, ColumnFlags::defaultFlags | ColumnFlags::sortable);
    header.addColumn(juce::translate("Feature"), ColumnType::Feature, 200, 100, 700, ColumnFlags::defaultFlags | ColumnFlags::sortable);
    header.addColumn(juce::translate("Description"), ColumnType::Details, 240, 100, 500, ColumnFlags::notSortable);
    header.addColumn(juce::translate("Manufacturer"), ColumnType::Maker, 150, 100, 300);
    header.addColumn(juce::translate("Category"), ColumnType::Category, 80, 100, 200);
    header.addColumn(juce::translate("Api"), ColumnType::Api, 40, 40, 40, ColumnFlags::notResizable | ColumnFlags::notSortable);
    
    addAndMakeVisible(mClearButton);
    mClearButton.setClickingTogglesState(false);
    mClearButton.onClick = [this]()
    {
        mAccessor.setAttr<AttrType::descriptions>(std::map<juce::String, Description>{}, NotificationType::synchronous);
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
        
        mAccessor.setAttr<AttrType::descriptions>(getNewList(), NotificationType::synchronous);
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
    
    mFilteredList.clear();
    auto const searchPattern = mSearchField.getText().removeCharacters(" ");
    for(auto const& description : descriptions)
    {
        auto const pluginName = (description.second.name + description.second.maker).removeCharacters(" ");
        for(size_t feature = 0; feature < description.second.features.size(); ++feature)
        {
            auto const filterName = pluginName + description.second.features[feature].removeCharacters(" ");
            if(searchPattern.isEmpty() || filterName.containsIgnoreCase(searchPattern))
            {
                mFilteredList.emplace_back(description.first, description.second, feature);
            }
        }
    }
        
    auto const isForwards = mAccessor.getAttr<AttrType::sortIsFowards>();
    switch (mAccessor.getAttr<AttrType::sortColumn>())
    {
        case ColumnType::Plugin:
        {
            std::sort(mFilteredList.begin(), mFilteredList.end(), [&](auto const& lhs, auto const& rhs)
            {
                auto const lhsName = lhs.name + (lhs.feature < lhs.features.size() ? lhs.features[lhs.feature] : "");
                auto const rhsName = rhs.name + (rhs.feature < rhs.features.size() ? rhs.features[rhs.feature] : "");
                return isForwards ? (lhsName > rhsName) : (lhsName < rhsName);
            });
        }
            break;
        case ColumnType::Feature:
        {
            std::sort(mFilteredList.begin(), mFilteredList.end(), [&](auto const& lhs, auto const& rhs)
            {
                auto const lhsFeature = lhs.feature < lhs.features.size() ? lhs.features[lhs.feature] : "";
                auto const rhsFeature = rhs.feature < rhs.features.size() ? rhs.features[rhs.feature] : "";
                return isForwards ? (lhsFeature > rhsFeature) : (lhsFeature < rhsFeature);
            });
        }
            break;
        case ColumnType::Maker:
        {
            std::sort(mFilteredList.begin(), mFilteredList.end(), [&](auto const& lhs, auto const& rhs)
            {
                return isForwards ? (lhs.maker > rhs.maker) : (lhs.maker < rhs.maker);
            });
        }
            break;
        case ColumnType::Category:
        {
            std::sort(mFilteredList.begin(), mFilteredList.end(), [&](auto const& lhs, auto const& rhs)
            {
                auto const lhsCategories = std::accumulate(lhs.categories.cbegin(), lhs.categories.cend(), juce::String());
                auto const rhsCategories = std::accumulate(rhs.categories.cbegin(), rhs.categories.cend(), juce::String());
                return isForwards ? (lhsCategories > rhsCategories) : (lhsCategories < rhsCategories);
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
    auto const& description = mFilteredList[index];
    auto const feature = description.feature;
    auto getText = [&]() -> juce::String
    {
        switch (columnId)
        {
            case ColumnType::Plugin:
                return description.name;
            case ColumnType::Feature:
                return static_cast<size_t>(feature) < description.features.size() ? description.features[static_cast<size_t>(feature)] : "";
            case ColumnType::Maker:
                return description.maker;
            case ColumnType::Api:
                return juce::String(description.api);
            case ColumnType::Details:
                return description.details;
            case ColumnType::Category:
                return description.categories.empty() ? "-" : *(description.categories.begin());
        }
        return "";
    };
    
    const auto defaultTextColour = mPluginTable.findColour(juce::ListBox::textColourId);
    g.setColour(columnId == ColumnType::Plugin ? defaultTextColour : defaultTextColour.interpolatedWith(juce::Colours::transparentBlack, 0.3f));
    g.setFont(juce::Font(static_cast<float>(height) * 0.7f, juce::Font::bold));
    g.drawFittedText(getText(), 4, 0, width - 6, height, juce::Justification::centredLeft, 1, 1.f);
}

void PluginList::Table::deleteKeyPressed(int lastRowSelected)
{
    juce::ignoreUnused(lastRowSelected);
    
    auto const selectedRows = mPluginTable.getSelectedRows();
    mPluginTable.deselectAllRows();
    
    auto descriptions = mAccessor.getAttr<AttrType::descriptions>();
    auto const numSelectedRows = selectedRows.size();
    for (int i = 0; i < numSelectedRows; ++i)
    {
        auto const index = selectedRows[i];
        anlStrongAssert(index >= 0 && static_cast<size_t>(index) < mFilteredList.size());
        if(index >= 0 && static_cast<size_t>(index) < mFilteredList.size())
        {
            auto const key = mFilteredList[static_cast<size_t>(index)].key;
            auto const feature = mFilteredList[static_cast<size_t>(index)].feature;
            anlStrongAssert(descriptions.count(key) > 0_z && feature < descriptions.at(key).features.size());
            if(descriptions.count(key) > 0_z && feature < descriptions.at(key).features.size())
            {
                if(descriptions.at(key).features.size() == 1)
                {
                    descriptions.erase(key);
                }
                else
                {
                    descriptions[key].features.erase(descriptions[key].features.begin() + static_cast<long>(feature));
                }
            }
            
        }
    }
    mAccessor.setAttr<AttrType::descriptions>(descriptions, NotificationType::synchronous);
}

void PluginList::Table::returnKeyPressed(int lastRowSelected)
{
    if(lastRowSelected < 0 || lastRowSelected >= static_cast<int>(mFilteredList.size()))
    {
        return;
    }
    auto const index = static_cast<size_t>(lastRowSelected);
    auto const key = mFilteredList[index].key;
    auto const feature = mFilteredList[index].feature;
    anlStrongAssert(feature < mFilteredList[index].features.size());
    if(feature >= mFilteredList[index].features.size())
    {
        return;
    }
    if(onPluginSelected != nullptr)
    {
        onPluginSelected(key, feature);
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
