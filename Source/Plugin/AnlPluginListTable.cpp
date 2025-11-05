#include "AnlPluginListTable.h"
#include <AnlIconsData.h>
#include <AnlResourceData.h>

ANALYSE_FILE_BEGIN

PluginList::Table::Table(Accessor& accessor, Scanner& scanner)
: mAccessor(accessor)
, mScanner(scanner)
, mSearchIcon(juce::ImageCache::getFromMemory(AnlIconsData::search_png, AnlIconsData::search_pngSize))
, mIrcamIcon(juce::ImageCache::getFromMemory(AnlResourceData::PompidoulogonoirRS_png, AnlResourceData::PompidoulogonoirRS_pngSize))
{
    addAndMakeVisible(mSearchIcon);
    addAndMakeVisible(mSearchField);
    addAndMakeVisible(mIrcamIcon);
    addAndMakeVisible(mSeparator1);
    addAndMakeVisible(mPluginTable);
    addAndMakeVisible(mSeparator2);
    addAndMakeVisible(mDescriptionPanel);

    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acsr, [[maybe_unused]] AttrType attribute)
    {
        updateContent();
    };

    mPluginTable.setRowHeight(22);
    mPluginTable.setMultipleSelectionEnabled(true);
    mPluginTable.setModel(this);

    mIrcamIcon.setTooltip(juce::translate("Show Ircam plugins"));
    mIrcamIcon.onClick = [&]()
    {
        mSearchField.setText("Ircam", true);
    };

    mSearchIcon.setTooltip(juce::translate("Search for a plugin by name, feature, description or maker"));
    mSearchIcon.onClick = [this]()
    {
        mSearchField.grabKeyboardFocus();
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

    mReceiver.onSignal = [&]([[maybe_unused]] Accessor const& acsr, SignalType signal, [[maybe_unused]] juce::var value)
    {
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
    auto headerBar = bounds.removeFromTop(20);
    mSearchIcon.setBounds(headerBar.removeFromLeft(20).reduced(2));
    mIrcamIcon.setBounds(headerBar.removeFromRight(22).reduced(2));
    mSearchField.setBounds(headerBar);
    mSeparator1.setBounds(bounds.removeFromTop(1));
    mDescriptionPanel.setBounds(bounds.removeFromBottom(Plugin::DescriptionPanel::optimalHeight).withTrimmedRight(2));
    mSeparator2.setBounds(bounds.removeFromBottom(1));
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

void PluginList::Table::notifyAddSelectedPlugins()
{
    if(onAddPlugins == nullptr)
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
    MiscWeakAssert(!selection.empty());
    onAddPlugins(std::move(selection));
}

void PluginList::Table::updateContent()
{
    mFilteredList.clear();
    mFilteredWebPlugins.clear();

    auto const searchPattern = mSearchField.getText().removeCharacters(" ");

    // Filter local plugins
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

    // Filter internet plugins that are compatible and NOT installed locally
    for(auto const& plugin : mAccessor.getAttr<AttrType::webReferences>())
    {
        auto const isNotInstalled = std::none_of(mList.cbegin(), mList.cend(), [&](auto const& pair)
                                                 {
                                                     return juce::String(pair.first.identifier).startsWith(plugin.identifier);
                                                 });
        if(isNotInstalled && plugin.isCompatible)
        {
            auto const filterName = (plugin.name + plugin.pluginDescription + plugin.libraryDescription + plugin.maker).removeCharacters(" ");
            if(searchPattern.isEmpty() || filterName.containsIgnoreCase(searchPattern))
            {
                mFilteredWebPlugins.push_back(plugin);
            }
        }
    }

    mPluginTable.updateContent();
    mPluginTable.repaint();
}

int PluginList::Table::getNumRows()
{
    return static_cast<int>(mFilteredList.size() + mFilteredWebPlugins.size());
}

void PluginList::Table::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    const auto defaultColour = mPluginTable.findColour(juce::ListBox::backgroundColourId);
    const auto selectedColour = defaultColour.interpolatedWith(mPluginTable.findColour(juce::ListBox::textColourId), 0.5f);
    g.fillAll(rowIsSelected ? selectedColour : defaultColour);

    auto const info = [&]()
    {
        auto const totalLocalPlugins = static_cast<int>(mFilteredList.size());
        auto const totalPlugins = totalLocalPlugins + static_cast<int>(mFilteredWebPlugins.size());
        if(rowNumber < totalLocalPlugins) // Local Plugins
        {
            auto const index = static_cast<size_t>(rowNumber);
            auto const& description = mFilteredList.at(index).second;
            return std::make_tuple(description.name + " - " + description.output.name, false);
        }
        else if(rowNumber < totalPlugins) // Web Plugins
        {
            static const auto webPluginIndicator = juce::CharPointer_UTF8(" \xf0\x9f\x8c\x90"); // 🌐
            auto const index = static_cast<size_t>(rowNumber - totalLocalPlugins);
            auto const& description = mFilteredWebPlugins.at(index);
            return std::make_tuple(description.name + juce::String(webPluginIndicator), true);
        }
        else [[unlikely]]
        {
            return std::make_tuple(juce::String(), true);
        }
    }();

    const auto defaultTextColour = mPluginTable.findColour(juce::ListBox::textColourId);
    g.setColour(std::get<1>(info) ? defaultTextColour.withAlpha(0.7f) : defaultTextColour);
    g.setFont(juce::Font(juce::FontOptions(static_cast<float>(height) * 0.7f)));
    g.drawText(std::get<0>(info), 4, 0, width - 6, height, juce::Justification::centredLeft, false);
}

void PluginList::Table::returnKeyPressed([[maybe_unused]] int lastRowSelected)
{
    notifyAddSelectedPlugins();
}

void PluginList::Table::deleteKeyPressed([[maybe_unused]] int lastRowSelected)
{
    getLookAndFeel().playAlertSound();
}

void PluginList::Table::selectedRowsChanged(int lastRowSelected)
{
    auto const totalLocalPlugins = static_cast<int>(mFilteredList.size());
    auto const totalPlugins = totalLocalPlugins + static_cast<int>(mFilteredWebPlugins.size());

    if(lastRowSelected < 0 || lastRowSelected >= totalPlugins) [[unlikely]]
    {
        mDescriptionPanel.setDescription({});
    }
    else if(lastRowSelected < totalLocalPlugins) // Local plugin
    {
        mDescriptionPanel.setDescription(mFilteredList.at(static_cast<size_t>(lastRowSelected)).second);
    }
    else // Web Plugin
    {
        mDescriptionPanel.setWebReference(mFilteredWebPlugins.at(static_cast<size_t>(lastRowSelected - totalLocalPlugins)));
    }
}

void PluginList::Table::listBoxItemDoubleClicked([[maybe_unused]] int rowNumber, [[maybe_unused]] juce::MouseEvent const& event)
{
    notifyAddSelectedPlugins();
}

juce::String PluginList::Table::getTooltipForRow(int rowNumber)
{
    auto const totalLocalPlugins = static_cast<int>(mFilteredList.size());
    auto const totalPlugins = totalLocalPlugins + static_cast<int>(mFilteredWebPlugins.size());
    if(rowNumber < 0 || rowNumber >= totalPlugins)
    {
        return "";
    }
    if(rowNumber < totalLocalPlugins)
    {
        return mFilteredList.at(static_cast<size_t>(rowNumber)).second.details;
    }
    return mFilteredWebPlugins.at(static_cast<size_t>(rowNumber - totalLocalPlugins)).pluginDescription;
}

void PluginList::Table::setMultipleSelectionEnabled(bool shouldBeEnabled) noexcept
{
    mPluginTable.setMultipleSelectionEnabled(shouldBeEnabled);
}

ANALYSE_FILE_END
