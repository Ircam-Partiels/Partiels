#include "AnlTrackResultTable.h"
#include "../AnlTrackTools.h"
#include "AnlTrackResultCell.h"

ANALYSE_FILE_BEGIN

Track::Result::Table::WindowContainer::WindowContainer(Result::Table& table)
: FloatingWindowContainer(juce::translate("Results"), table)
, mTable(table)
, mTooltip(&mTable)
{
    mFloatingWindow.setResizable(true, false);
    mBoundsConstrainer.setMinimumWidth(420 + mTable.mTable.getVerticalScrollBar().getWidth());
    mBoundsConstrainer.setMinimumHeight(120);
    mFloatingWindow.setConstrainer(&mBoundsConstrainer);

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
                break;
            case AttrType::name:
            {
                mFloatingWindow.setName(juce::translate("ANLNAME RESULTS").replace("ANLNAME", acsr.getAttr<AttrType::name>().toUpperCase()));
            }
            break;
            case AttrType::file:
            case AttrType::results:
            case AttrType::key:
            case AttrType::description:
            case AttrType::state:
            case AttrType::height:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::unit:
            case AttrType::channelsLayout:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::graphics:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::focused:
            case AttrType::grid:
                break;
        }
    };
    mTable.mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::Result::Table::WindowContainer::~WindowContainer()
{
    mTable.mAccessor.removeListener(mListener);
}

Track::Result::Table::Table(Director& director, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAcsr)
: mDirector(director)
, mTransportAccessor(transportAcsr)
, mTimeZoomAccessor(timeZoomAccessor)
{
    setSize(420 + mTable.getVerticalScrollBar().getWidth(), 600);
    setWantsKeyboardFocus(true);

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
                break;
            case AttrType::file:
            case AttrType::results:
            case AttrType::description:
            case AttrType::unit:
            {
                mTabbedButtonBar.removeChangeListener(this);
                auto setNumChannels = [this](auto resultPtr)
                {
                    auto const numChannels = resultPtr != nullptr ? static_cast<int>(resultPtr->size()) : 0;
                    auto const channel = getSelectedChannel();
                    mTabbedButtonBar.clearTabs();
                    for(auto i = 0; i < numChannels; ++i)
                    {
                        auto const name = juce::translate("Channel INDEX").replace("INDEX", juce::String(i + 1));
                        mTabbedButtonBar.addTab(name, juce::Colours::transparentBlack, i);
                    }
                    mTabbedButtonBar.setCurrentTabIndex(channel.has_value() ? static_cast<int>(*channel) : 0, false);
                    mTabbedButtonBar.setVisible(numChannels > 1);
                };

                auto const& results = acsr.getAttr<AttrType::results>();
                auto const access = results.getReadAccess();
                if(static_cast<bool>(access))
                {
                    auto& header = mTable.getHeader();
                    switch(Tools::getFrameType(acsr))
                    {
                        case Track::FrameType::label:
                        {
                            header.setColumnName(static_cast<int>(ColumnType::value), "Label");
                            setNumChannels(results.getMarkers());
                        }
                        break;
                        case Track::FrameType::value:
                        {
                            header.setColumnName(static_cast<int>(ColumnType::value), "Value");
                            setNumChannels(results.getPoints());
                        }
                        break;
                        case Track::FrameType::vector:
                        {
                            header.setColumnName(static_cast<int>(ColumnType::value), "Values");
                            setNumChannels(results.getColumns());
                        }
                        break;
                        default:
                        {
                            mTabbedButtonBar.setCurrentTabIndex(-1, false);
                            mTabbedButtonBar.clearTabs();
                        }
                        break;
                    }
                }
                mTable.repaint();
                mTable.updateContent();
                resized();
                mTabbedButtonBar.addChangeListener(this);
                selectionUpdated();
            }
            break;
            case AttrType::key:
            case AttrType::state:
            case AttrType::height:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::channelsLayout:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::graphics:
            case AttrType::warnings:
            case AttrType::processing:
                break;
            case AttrType::focused:
            {
                auto const focused = acsr.getAttr<AttrType::focused>();
                auto const channel = getSelectedChannel();
                if(focused.any() && (!channel.has_value() || !focused.test(*channel)))
                {
                    for(auto index = 0_z; index < focused.size(); ++index)
                    {
                        if(focused.test(index))
                        {
                            mTabbedButtonBar.setCurrentTabIndex(static_cast<int>(index), false);
                            break;
                        }
                    }
                }
            }
            break;
            case AttrType::grid:
                break;
        }
    };

    mTransportListener.onAttrChanged = [this](Transport::Accessor const& acsr, Transport::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Transport::AttrType::looping:
            case Transport::AttrType::loopRange:
            case Transport::AttrType::stopAtLoopEnd:
            case Transport::AttrType::gain:
            case Transport::AttrType::markers:
            case Transport::AttrType::magnetize:
            case Transport::AttrType::autoScroll:
                break;
            case Transport::AttrType::playback:
            case Transport::AttrType::startPlayhead:
            case Transport::AttrType::runningPlayhead:
            {
                auto const playHeadRow = getPlayheadRow();
                if(playHeadRow.has_value())
                {
                    mTable.repaintRow(static_cast<int>(*playHeadRow));
                    mTable.scrollToEnsureRowIsOnscreen(static_cast<int>(*playHeadRow));
                }
                if(mPreviousPlayheadRow.has_value())
                {
                    mTable.repaintRow(static_cast<int>(*mPreviousPlayheadRow));
                }
                mPreviousPlayheadRow = playHeadRow;
            }
            break;
            case Transport::AttrType::selection:
            {
                selectionUpdated();
            }
            break;
        }
    };

    mTimeZoomListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            {
                mTable.repaint();
                mTable.updateContent();
            }
            break;
            case Zoom::AttrType::minimumLength:
            case Zoom::AttrType::visibleRange:
            case Zoom::AttrType::anchor:
                break;
        }
    };

    addAndMakeVisible(mTable);
    addAndMakeVisible(mSeparator);
    addAndMakeVisible(mTabbedButtonBar);
    mTabbedButtonBar.setMinimumTabScaleFactor(1.0f);

    mTable.setHeaderHeight(28);
    mTable.setRowHeight(22);
    mTable.setMultipleSelectionEnabled(true);
    mTable.setModel(this);

    using ColumnFlags = juce::TableHeaderComponent::ColumnPropertyFlags;
    auto& header = mTable.getHeader();
    header.setStretchToFitActive(true);
    header.addColumn(juce::translate("Index"), static_cast<int>(ColumnType::index), 60, 60, 60, ColumnFlags::visible);
    header.addColumn(juce::translate("Time"), static_cast<int>(ColumnType::time), 132, 132, 132, ColumnFlags::visible);
    header.addColumn(juce::translate("Duration"), static_cast<int>(ColumnType::duration), 132, 132, 132, ColumnFlags::visible);
    header.addColumn(juce::translate("Undefined"), static_cast<int>(ColumnType::value), 96, 84, -1, ColumnFlags::visible);
}

Track::Result::Table::~Table()
{
    mTabbedButtonBar.removeChangeListener(this);
    mTimeZoomAccessor.removeListener(mTimeZoomListener);
    mTransportAccessor.removeListener(mTransportListener);
    mAccessor.removeListener(mListener);
}

void Track::Result::Table::resized()
{
    auto bounds = getLocalBounds();
    if(mTabbedButtonBar.isVisible())
    {
        mTabbedButtonBar.setBounds(bounds.removeFromBottom(28));
    }
    mSeparator.setBounds(bounds.removeFromBottom(1));
    mTable.setBounds(bounds);
}

void Track::Result::Table::visibilityChanged()
{
    mTable.updateContent();
    repaint();
    parentHierarchyChanged();
}

void Track::Result::Table::parentHierarchyChanged()
{
    if(!isShowing())
    {
        mTimeZoomAccessor.removeListener(mTimeZoomListener);
        mTransportAccessor.removeListener(mTransportListener);
        mAccessor.removeListener(mListener);
    }
    else
    {
        mAccessor.addListener(mListener, NotificationType::synchronous);
        mTransportAccessor.addListener(mTransportListener, NotificationType::synchronous);
        mTimeZoomAccessor.addListener(mTimeZoomListener, NotificationType::synchronous);
    }
}

int Track::Result::Table::getNumRows()
{
    auto const& results = mAccessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return 0;
    }

    auto const channel = getSelectedChannel();
    if(!channel.has_value())
    {
        return 0;
    }
    auto getNumColumns = [&](auto resultPtr)
    {
        auto constexpr maxSize = std::numeric_limits<int>::max();
        auto const size = *channel < resultPtr->size() ? resultPtr->at(*channel).size() : 0_z;
        return size < maxSize ? static_cast<int>(size) : maxSize;
    };
    switch(Tools::getFrameType(mAccessor))
    {
        case Track::FrameType::label:
        {
            return getNumColumns(results.getMarkers());
        }
        case Track::FrameType::value:
        {
            return getNumColumns(results.getPoints());
        }
        case Track::FrameType::vector:
        {
            return getNumColumns(results.getColumns());
        }
    };
    return 0;
}

void Track::Result::Table::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    juce::ignoreUnused(width, height);
    auto const defaultColour = mTable.findColour(juce::ListBox::backgroundColourId);
    auto const textColour = mTable.findColour(juce::ListBox::textColourId);
    g.fillAll(rowIsSelected ? defaultColour.interpolatedWith(textColour, 0.25f) : defaultColour);
    auto const playHeadRow = getPlayheadRow();
    if(playHeadRow.has_value() && *playHeadRow == static_cast<size_t>(rowNumber))
    {
        g.setColour(defaultColour.interpolatedWith(textColour, 0.75f));
        g.drawRect(0, 0, width, height, 1);
    }
}

juce::Component* Track::Result::Table::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, juce::Component* existingComponentToUpdate)
{
    std::unique_ptr<juce::Component> component(existingComponentToUpdate);
    if(rowNumber < 0 || columnId == static_cast<int>(ColumnType::index) || !isRowSelected)
    {
        return nullptr;
    }
    auto const channel = getSelectedChannel();
    if(!channel.has_value())
    {
        return nullptr;
    }

    auto const& results = mAccessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return nullptr;
    }

    if(auto* previousCell = dynamic_cast<CellBase*>(existingComponentToUpdate))
    {
        if(previousCell->updateAndValidate(*channel))
        {
            return component.release();
        }
    }

    auto const frameIndex = static_cast<size_t>(rowNumber);
    if(columnId == static_cast<int>(ColumnType::time))
    {
        auto cell = std::make_unique<CellTime>(mDirector, mTimeZoomAccessor, *channel, frameIndex);
        if(cell != nullptr && cell->updateAndValidate(*channel))
        {
            return cell.release();
        }
    }
    else if(columnId == static_cast<int>(ColumnType::duration))
    {
        auto cell = std::make_unique<CellDuration>(mDirector, mTimeZoomAccessor, *channel, frameIndex);
        if(cell != nullptr && cell->updateAndValidate(*channel))
        {
            return cell.release();
        }
    }
    else if(columnId == static_cast<int>(ColumnType::value))
    {
        auto cell = std::make_unique<CellValue>(mDirector, mTimeZoomAccessor, *channel, frameIndex);
        if(cell != nullptr && cell->updateAndValidate(*channel))
        {
            return cell.release();
        }
    }

    return nullptr;
}

void Track::Result::Table::paintCell(juce::Graphics& g, int row, int columnId, int width, int height, bool rowIsSelected)
{
    juce::ignoreUnused(rowIsSelected);
    auto const& results = mAccessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access) || row < 0)
    {
        return;
    }

    auto const channel = getSelectedChannel();
    if(!channel.has_value())
    {
        return;
    }

    auto const frameIndex = static_cast<size_t>(row);
    auto isValid = [&](auto const& resultPtr)
    {
        return *channel < resultPtr->size() && frameIndex < resultPtr->at(*channel).size();
    };

    auto drawText = [&](juce::String const& text)
    {
        g.setColour(mTable.findColour(juce::ListBox::textColourId));
        g.setFont(juce::Font(static_cast<float>(height) * 0.7f));
        juce::Rectangle<int> const bounds(4, 0, width - 14, height);
        auto constexpr justification = juce::Justification::centredLeft;
        g.drawFittedText(text, bounds, justification, 1, 1.0f);
    };

    if(auto markers = results.getMarkers())
    {
        if(!isValid(markers))
        {
            return;
        }
        switch(columnId)
        {
            case static_cast<int>(ColumnType::index):
            {
                drawText(juce::String(frameIndex + 1_z));
            }
            break;
            case static_cast<int>(ColumnType::time):
            {
                drawText(Format::secondsToString(std::get<0_z>(markers->at(*channel).at(frameIndex))));
            }
            break;
            case static_cast<int>(ColumnType::duration):
            {
                drawText(Format::secondsToString(std::get<1_z>(markers->at(*channel).at(frameIndex))));
            }
            break;
            case static_cast<int>(ColumnType::value):
            {
                auto const text = std::get<2_z>(markers->at(*channel).at(frameIndex));
                drawText(text.empty() ? " - " : text);
            }
            break;
        }
    }
    if(auto points = results.getPoints())
    {
        if(!isValid(points))
        {
            return;
        }
        switch(columnId)
        {
            case static_cast<int>(ColumnType::index):
            {
                drawText(juce::String(frameIndex + 1_z));
            }
            break;
            case static_cast<int>(ColumnType::time):
            {
                drawText(Format::secondsToString(std::get<0_z>(points->at(*channel).at(frameIndex))));
            }
            break;
            case static_cast<int>(ColumnType::duration):
            {
                drawText(Format::secondsToString(std::get<1_z>(points->at(*channel).at(frameIndex))));
            }
            break;
            case static_cast<int>(ColumnType::value):
            {
                auto const& value = std::get<2_z>(points->at(*channel).at(frameIndex));
                if(value.has_value())
                {
                    drawText(Format::valueToString(*value, 4) + Tools::getUnit(mAccessor));
                }
                else
                {
                    drawText(" - ");
                }
            }
            break;
        }
    }
    if(auto columns = results.getColumns())
    {
        if(!isValid(columns))
        {
            return;
        }
        switch(columnId)
        {
            case static_cast<int>(ColumnType::index):
            {
                drawText(juce::String(frameIndex + 1_z));
            }
            break;
            case static_cast<int>(ColumnType::time):
            {
                drawText(Format::secondsToString(std::get<0_z>(columns->at(*channel).at(frameIndex))));
            }
            break;
            case static_cast<int>(ColumnType::duration):
            {
                drawText(Format::secondsToString(std::get<1_z>(columns->at(*channel).at(frameIndex))));
            }
            break;
            case static_cast<int>(ColumnType::value):
            {
                auto const& values = std::get<2_z>(columns->at(*channel).at(frameIndex));
                juce::StringArray textArray;
                textArray.ensureStorageAllocated(static_cast<int>(values.size()));
                for(auto const& value : values)
                {
                    textArray.add(Format::valueToString(value, 4));
                }
                drawText(textArray.joinIntoString(", "));
            }
            break;
        }
    }
}

void Track::Result::Table::deleteKeyPressed(int lastRowSelected)
{
    juce::ignoreUnused(lastRowSelected);
    if(!deleteSelection())
    {
        getLookAndFeel().playAlertSound();
    }
}

void Track::Result::Table::selectedRowsChanged(int lastRowSelected)
{
    if(lastRowSelected < 0)
    {
        return;
    }
    auto const channel = getSelectedChannel();
    if(!channel.has_value())
    {
        return;
    }

    // Force continuous selection
    auto const selectedRows = mTable.getSelectedRows();
    if(selectedRows.getNumRanges() > 1)
    {
        juce::SparseSet<int> set;
        set.addRange(selectedRows.getTotalRange());
        mTable.setSelectedRows(set, juce::NotificationType::dontSendNotification);
    }

    auto const range = mTable.getSelectedRows().getTotalRange();
    auto const selectionStart = Modifier::getTime(mAccessor, *channel, static_cast<size_t>(range.getStart()));
    if(selectionStart.has_value())
    {
        auto const selectionEnd = Modifier::getTime(mAccessor, *channel, static_cast<size_t>(range.getEnd()));
        auto const globalRange = mTimeZoomAccessor.getAttr<Zoom::AttrType::globalRange>();
        juce::Range<double> const selection{*selectionStart, selectionEnd.has_value() ? *selectionEnd : globalRange.getEnd()};
        mTransportAccessor.setAttr<Transport::AttrType::selection>(selection, NotificationType::synchronous);
    }

    auto const time = Modifier::getTime(mAccessor, *channel, static_cast<size_t>(lastRowSelected));
    if(time.has_value())
    {
        mTransportAccessor.setAttr<Transport::AttrType::startPlayhead>(*time, NotificationType::synchronous);
    }
}

juce::String Track::Result::Table::getCellTooltip(int rowNumber, int columnId)
{
    juce::ignoreUnused(columnId);
    if(rowNumber >= getNumRows())
    {
        return "";
    }
    return "";
}

void Track::Result::Table::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    MiscWeakAssert(source == &mTabbedButtonBar);
    if(source != &mTabbedButtonBar)
    {
        return;
    }

    auto const channel = getSelectedChannel();
    if(channel.has_value())
    {
        FocusInfo info;
        info.set(*channel);
        mAccessor.setAttr<AttrType::focused>(info, NotificationType::synchronous);
    }

    mTable.updateContent();
    mTable.repaint();
}

bool Track::Result::Table::keyPressed(juce::KeyPress const& key)
{
    if(key.isKeyCode(juce::KeyPress::deleteKey) || key.isKeyCode(juce::KeyPress::backspaceKey))
    {
        if(!deleteSelection())
        {
            getLookAndFeel().playAlertSound();
        }
        return true;
    }
    if(key == juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0) || key == juce::KeyPress(juce::KeyPress::insertKey, juce::ModifierKeys::ctrlModifier, 0))
    {
        if(!copySelection())
        {
            getLookAndFeel().playAlertSound();
        }
        return true;
    }
    if(key == juce::KeyPress('x', juce::ModifierKeys::commandModifier, 0) || key == juce::KeyPress(juce::KeyPress::deleteKey, juce::ModifierKeys::shiftModifier, 0))
    {
        if(!cutSelection())
        {
            getLookAndFeel().playAlertSound();
        }
        return true;
    }
    if(key == juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0) || key == juce::KeyPress(juce::KeyPress::insertKey, juce::ModifierKeys::shiftModifier, 0))
    {
        if(!pasteSelection())
        {
            getLookAndFeel().playAlertSound();
        }
        return true;
    }
    if(key == juce::KeyPress('d', juce::ModifierKeys::commandModifier, 0))
    {
        if(!duplicateSelection())
        {
            getLookAndFeel().playAlertSound();
        }
        return true;
    }
    return false;
}

std::optional<size_t> Track::Result::Table::getSelectedChannel() const
{
    auto const index = mTabbedButtonBar.getCurrentTabIndex();
    if(index >= 0)
    {
        return static_cast<size_t>(index);
    }
    return {};
}

std::optional<size_t> Track::Result::Table::getPlayheadRow() const
{
    auto const channel = getSelectedChannel();
    if(!channel.has_value())
    {
        return {};
    }
    auto const isPlaying = mTransportAccessor.getAttr<Transport::AttrType::playback>();
    auto const time = isPlaying ? mTransportAccessor.getAttr<Transport::AttrType::runningPlayhead>() : mTransportAccessor.getAttr<Transport::AttrType::startPlayhead>();
    return Modifier::getIndex(mAccessor, *channel, time);
}

bool Track::Result::Table::deleteSelection()
{
    if(Tools::getFrameType(mAccessor) == Track::FrameType::vector)
    {
        return false;
    }

    auto const channel = getSelectedChannel();
    if(!channel.has_value())
    {
        return false;
    }

    auto& undoManager = mDirector.getUndoManager();
    auto const playhead = mTransportAccessor.getAttr<Transport::AttrType::startPlayhead>();
    auto const selection = mTransportAccessor.getAttr<Transport::AttrType::selection>();
    undoManager.beginNewTransaction(juce::translate("Erase Frame(s)"));
    if(undoManager.perform(std::make_unique<Modifier::ActionErase>(mDirector.getSafeAccessorFn(), *channel, selection).release()))
    {
        undoManager.perform(std::make_unique<Result::Modifier::FocusRestorer>(mDirector.getSafeAccessorFn()).release());
        undoManager.perform(std::make_unique<Transport::Action::Restorer>(mDirector.getSafeTransportZoomAccessorFn(), playhead, selection).release());
        return true;
    }
    return false;
}

bool Track::Result::Table::copySelection()
{
    if(Tools::getFrameType(mAccessor) == Track::FrameType::vector)
    {
        return false;
    }

    auto const channel = getSelectedChannel();
    if(!channel.has_value())
    {
        return false;
    }

    auto const selection = mTransportAccessor.getAttr<Transport::AttrType::selection>();
    auto const indices = Modifier::getIndices(mAccessor, *channel, selection);
    if(indices.empty())
    {
        return false;
    }
    mCopiedData = Modifier::copyFrames(mAccessor, *channel, indices);
    mCopiedSelection = selection;
    return true;
}

bool Track::Result::Table::cutSelection()
{
    if(copySelection())
    {
        return deleteSelection();
    }
    return false;
}

bool Track::Result::Table::pasteSelection()
{
    if(Tools::getFrameType(mAccessor) == Track::FrameType::vector)
    {
        return false;
    }

    auto const channel = getSelectedChannel();
    if(!channel.has_value())
    {
        return false;
    }

    auto const indices = Result::Modifier::getIndices(mCopiedData);
    if(indices.empty())
    {
        return true;
    }

    auto& undoManager = mDirector.getUndoManager();
    auto const playhead = mTransportAccessor.getAttr<Transport::AttrType::startPlayhead>();
    auto const selection = mTransportAccessor.getAttr<Transport::AttrType::selection>();
    undoManager.beginNewTransaction(juce::translate("Paste Frame(s)"));
    if(undoManager.perform(std::make_unique<Result::Modifier::ActionPaste>(mDirector.getSafeAccessorFn(), *channel, mCopiedSelection, mCopiedData, playhead).release()))
    {
        undoManager.perform(std::make_unique<Result::Modifier::FocusRestorer>(mDirector.getSafeAccessorFn()).release());
        undoManager.perform(std::make_unique<Transport::Action::Restorer>(mDirector.getSafeTransportZoomAccessorFn(), playhead, selection).release());
        return true;
    }
    return true;
}

bool Track::Result::Table::duplicateSelection()
{
    if(Tools::getFrameType(mAccessor) == Track::FrameType::vector)
    {
        return false;
    }

    auto const channel = getSelectedChannel();
    if(!channel.has_value())
    {
        return false;
    }

    if(!copySelection())
    {
        return false;
    }
    auto const indices = Result::Modifier::getIndices(mCopiedData);
    if(indices.empty())
    {
        return true;
    }

    auto& undoManager = mDirector.getUndoManager();
    mTransportAccessor.setAttr<Transport::AttrType::startPlayhead>(mCopiedSelection.getEnd(), NotificationType::synchronous);
    auto const playhead = mTransportAccessor.getAttr<Transport::AttrType::startPlayhead>();
    auto const selection = mTransportAccessor.getAttr<Transport::AttrType::selection>();
    undoManager.beginNewTransaction(juce::translate("Duplicate Frame(s)"));
    if(undoManager.perform(std::make_unique<Result::Modifier::ActionPaste>(mDirector.getSafeAccessorFn(), *channel, mCopiedSelection, mCopiedData, mCopiedSelection.getEnd()).release()))
    {
        undoManager.perform(std::make_unique<Result::Modifier::FocusRestorer>(mDirector.getSafeAccessorFn()).release());
        undoManager.perform(std::make_unique<Transport::Action::Restorer>(mDirector.getSafeTransportZoomAccessorFn(), playhead, selection.movedToStartAt(playhead)).release());
        return true;
    }
    return false;
}

void Track::Result::Table::selectionUpdated()
{
    auto const channel = getSelectedChannel();
    if(!channel.has_value())
    {
        return;
    }
    auto const selection = mTransportAccessor.getAttr<Transport::AttrType::selection>();
    auto const indices = Modifier::getIndices(mAccessor, *channel, selection);
    juce::SparseSet<int> set;
    if(!indices.empty())
    {
        set.addRange({static_cast<int>(*indices.cbegin()), static_cast<int>(*indices.crbegin()) + 1});
    }
    mTable.setSelectedRows(set, juce::NotificationType::dontSendNotification);
    mTable.repaint();
}

ANALYSE_FILE_END
