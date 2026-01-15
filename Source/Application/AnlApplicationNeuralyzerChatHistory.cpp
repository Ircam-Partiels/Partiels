#include "AnlApplicationNeuralyzerChatHistory.h"
#include <md4c.h>

ANALYSE_FILE_BEGIN

class Application::Neuralyzer::ChatHistory::TableView
: public juce::Component
, private juce::TableListBoxModel
{
public:
    explicit TableView(TableContent table)
    : mData(std::move(table))
    {
        addAndMakeVisible(mCopy);
        addAndMakeVisible(mExport);
        addAndMakeVisible(mSeparator);
        addAndMakeVisible(mTable);

        mCopy.onClick = [this]()
        {
            juce::SystemClipboard::copyTextToClipboard(mData.toCsv());
            Tooltip::showMessageBox(juce::translate("Table copied to clipboard"), mCopy.getScreenBounds(), 2000);
        };

        mExport.onClick = [this]()
        {
            mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Export to file"), juce::File{}, "*.csv");
            using Flags = juce::FileBrowserComponent::FileChooserFlags;
            auto const fcFlags = Flags::saveMode | Flags::canSelectFiles | Flags::warnAboutOverwriting;
            mFileChooser->launchAsync(fcFlags, [=, this](juce::FileChooser const& fileChooser)
                                      {
                                          auto const results = fileChooser.getResults();
                                          if(results.isEmpty())
                                          {
                                              return;
                                          }
                                          results.getFirst().replaceWithText(mData.toCsv());
                                      });
        };

        mTable.setModel(this);
        mTable.setHeaderHeight(28);
        mTable.setRowHeight(22);
        mTable.setMultipleSelectionEnabled(false);
        static auto constexpr defaultColumnWidth = 80;
        using ColumnFlags = juce::TableHeaderComponent::ColumnPropertyFlags;
        auto& header = mTable.getHeader();
        auto const numColumns = mData.headers.size();
        for(auto column = 0_z; column < numColumns; ++column)
        {
            auto const title = !mData.headers.at(column).isEmpty() ? mData.headers.at(column) : juce::translate("Column INDEX").replace("INDEX", juce::String(column + 1));
            header.addColumn(title, static_cast<int>(column) + 1, defaultColumnWidth, 40, -1, ColumnFlags::visible | ColumnFlags::resizable);
        }

        mTable.updateContent();
        mTable.autoSizeAllColumns();
        auto fullWidth = mTable.getVerticalScrollBar().getWidth();
        for(auto column = 0_z; column < numColumns; ++column)
        {
            fullWidth += getColumnAutoSizeWidth(static_cast<int>(column) + 1);
        }
        fullWidth += 2;
        auto const fullHeight = mTable.getHeaderHeight() + getNumRows() * (mTable.getRowHeight() + 1) + 20 + 4;
        auto const* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
        auto const maxWidth = display != nullptr ? static_cast<int>(display->userBounds.getWidth() / 2.0f) : 0;
        auto const maxHeight = display != nullptr ? static_cast<int>(display->userBounds.getHeight() / 2.0f) : 0;
        setSize(std::min(fullWidth, maxWidth), std::min(fullHeight, maxHeight));
    }

    ~TableView() override = default;

    // juce::Component
    void resized() override
    {
        auto bounds = getLocalBounds().reduced(1);
        auto bottom = bounds.removeFromBottom(20);
        mCopy.setBounds(bottom.removeFromLeft(48));
        bottom.removeFromLeft(1);
        mExport.setBounds(bottom.removeFromLeft(48));
        bounds.removeFromBottom(1);
        mSeparator.setBounds(bounds.removeFromBottom(1));
        mTable.setBounds(bounds);
    }

private:
    // juce::TableListBoxModel
    int getNumRows() override
    {
        return static_cast<int>(mData.rows.size());
    }

    void paintRowBackground(juce::Graphics& g, [[maybe_unused]] int rowNumber, [[maybe_unused]] int width, [[maybe_unused]] int height, bool rowIsSelected) override
    {
        auto const defaultColour = mTable.findColour(juce::ListBox::backgroundColourId);
        auto const textColour = mTable.findColour(juce::ListBox::textColourId);
        g.fillAll(rowIsSelected ? defaultColour.interpolatedWith(textColour, 0.25f) : defaultColour);
    }

    void paintCell(juce::Graphics& g, int row, int columnId, int width, int height, [[maybe_unused]] bool rowIsSelected) override
    {
        auto const columnIndex = static_cast<size_t>(std::max(0, columnId - 1));
        auto const rowIndex = static_cast<size_t>(std::max(0, row));
        if(rowIndex >= mData.rows.size() || columnIndex >= mData.rows.at(rowIndex).size())
        {
            return;
        }
        auto const text = mData.rows.at(rowIndex).at(columnIndex);
        g.setColour(findColour(juce::TextEditor::ColourIds::textColourId));
        g.drawText(text, 4, 0, width - 8, height, juce::Justification::centredLeft, true);
    }

    int getColumnAutoSizeWidth(int columnId) override
    {
        auto const columnIndex = static_cast<size_t>(std::max(0, columnId - 1));
        if(columnIndex >= mData.headers.size())
        {
            return 0;
        }
        juce::Font const font{juce::FontOptions()};
        auto const getColWidth = [&](std::vector<juce::String> const& row)
        {
            return (columnIndex >= row.size() ? 0 : juce::GlyphArrangement::getStringWidthInt(font, row.at(columnIndex))) + 12;
        };
        return std::accumulate(mData.rows.cbegin(), mData.rows.cend(), getColWidth(mData.headers), [&](auto width, auto const& row)
                               {
                                   return std::max(getColWidth(row), width);
                               });
    }

    TableContent mData;
    juce::TableListBox mTable;
    ColouredPanel mSeparator;
    juce::TextButton mCopy{juce::translate("Copy"), juce::translate("Copy to clipboard")};
    juce::TextButton mExport{juce::translate("Export"), juce::translate("Export to file")};
    std::unique_ptr<juce::FileChooser> mFileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TableView)
};

struct Application::Neuralyzer::ChatHistory::ParseState
{
    struct ListState
    {
        bool ordered = false;
        int index = 1;
    };

    ChatHistory& editor;
    std::vector<std::pair<TextRange, juce::URL>>& links = editor.mLinks;
    std::vector<std::pair<TextRange, TableContent>>& tables = editor.mTables;
    juce::Font const baseFont = editor.getFont();
    juce::Colour const colour = editor.findColour(juce::TextEditor::ColourIds::textColourId);
    std::vector<ListState> listStack = {};
    int listItemDepth = 0;
    int headingLevel = 0;
    int emphasisDepth = 0;
    int strongDepth = 0;
    int codeDepth = 0;
    bool atLineStart = true;
    bool isInTable = false;
    bool isInTableHead = false;
    bool isInTableCell = false;
    int currentTableRow = -1;
    int currentTableColumn = -1;
    bool isLink = false;

    juce::Font getCurrentFont() const
    {
        auto font = baseFont;
        if(headingLevel > 0)
        {
            auto const size = std::max(14.0f, 22.0f - static_cast<float>(headingLevel - 1) * 2.0f);
            font = juce::Font(juce::FontOptions(size)).boldened();
        }
        if(strongDepth > 0)
        {
            font.setBold(true);
        }
        if(emphasisDepth > 0)
        {
            font.setItalic(true);
        }
        if(isLink > 0)
        {
            font.setUnderline(true);
        }
        if(codeDepth > 0)
        {
            font = juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), font.getHeight() * 0.84f, juce::Font::FontStyleFlags::plain));
        }
        return font;
    }

    void insertText(juce::String const& text)
    {
        if(text.isEmpty())
        {
            return;
        }
        if(isInTable && !tables.empty())
        {
            auto& table = tables.back().second;
            if(!isInTableCell || currentTableColumn < 0)
            {
                return;
            }

            auto const column = static_cast<size_t>(currentTableColumn);
            if(isInTableHead)
            {
                if(column >= table.headers.size())
                {
                    table.headers.resize(column + 1);
                }
                table.headers[column] += text;
            }
            else if(currentTableRow >= 0)
            {
                auto const row = static_cast<size_t>(currentTableRow);
                if(row >= table.rows.size())
                {
                    table.rows.resize(row + 1);
                }
                if(column >= table.rows[row].size())
                {
                    table.rows[row].resize(column + 1);
                }
                table.rows[row][column] += text;
            }
            atLineStart = text.endsWithChar('\n');
        }
        else
        {

            editor.setColour(juce::TextEditor::ColourIds::textColourId, isLink || isInTable ? juce::Colours::blue : colour);
            editor.setFont(getCurrentFont());
            editor.insertTextAtCaret(text);
            atLineStart = text.endsWithChar('\n');
            editor.setColour(juce::TextEditor::ColourIds::textColourId, colour);
        }
    }

    void insertLineBreak()
    {
        if(!atLineStart)
        {
            insertText("\n");
        }
    }

    static int onEnterBlock(MD_BLOCKTYPE type, void* detail, void* userdata)
    {
        auto& state = *static_cast<ParseState*>(userdata);
        switch(type)
        {
            case MD_BLOCK_UL:
            {
                state.listStack.push_back({false, 1});
                state.insertLineBreak();
                break;
            }
            case MD_BLOCK_OL:
            {
                auto const* orderedDetail = static_cast<MD_BLOCK_OL_DETAIL const*>(detail);
                state.listStack.push_back({true, orderedDetail != nullptr ? static_cast<int>(orderedDetail->start) : 1});
                state.insertLineBreak();
                break;
            }
            case MD_BLOCK_LI:
            {
                ++state.listItemDepth;
                state.insertLineBreak();
                auto const nesting = std::max(0, static_cast<int>(state.listStack.size()) - 1);
                state.insertText(juce::String::repeatedString("  ", nesting));
                if(!state.listStack.empty())
                {
                    auto& list = state.listStack.back();
                    state.insertText(list.ordered ? (juce::String(list.index++) + ". ") : juce::String(juce::CharPointer_UTF8("\xe2\x80\xa2 "))); // 1. or •
                }
                auto const* listItemDetail = static_cast<MD_BLOCK_LI_DETAIL const*>(detail);
                if(listItemDetail != nullptr && listItemDetail->is_task)
                {
                    state.insertText(listItemDetail->task_mark == ' ' ? juce::String(juce::CharPointer_UTF8("\xe2\x98\x90 ")) : juce::String(juce::CharPointer_UTF8("\xe2\x98\x91 "))); // ☐ or ☑
                }
                break;
            }
            case MD_BLOCK_H:
            {
                state.insertLineBreak();
                auto const* headingDetail = static_cast<MD_BLOCK_H_DETAIL const*>(detail);
                state.headingLevel = headingDetail != nullptr ? static_cast<int>(headingDetail->level) : 1;
                break;
            }
            case MD_BLOCK_P:
            {
                if(state.listItemDepth == 0)
                {
                    state.insertLineBreak();
                }
                break;
            }
            case MD_BLOCK_CODE:
            {
                state.insertLineBreak();
                ++state.codeDepth;
                break;
            }
            case MD_BLOCK_TABLE:
            {
                if(auto const* tableDetail = static_cast<MD_BLOCK_TABLE_DETAIL const*>(detail))
                {
                    state.isInTable = true;
                    state.isInTableHead = false;
                    state.isInTableCell = false;
                    state.currentTableRow = -1;
                    state.currentTableColumn = -1;
                    state.tables.emplace_back(TextRange{}, TableContent{});
                    auto& table = state.tables.back().second;
                    table.headers.resize(static_cast<size_t>(tableDetail->col_count));
                    table.rows.resize(static_cast<size_t>(tableDetail->body_row_count));
                    for(auto& row : table.rows)
                    {
                        row.resize(static_cast<size_t>(tableDetail->col_count));
                    }
                }
                break;
            }
            case MD_BLOCK_THEAD:
            {
                state.isInTableHead = true;
                state.currentTableRow = -1;
                state.currentTableColumn = -1;
                break;
            }
            case MD_BLOCK_TBODY:
            {
                state.isInTableHead = false;
                state.currentTableRow = -1;
                state.currentTableColumn = -1;
                break;
            }
            case MD_BLOCK_TR:
            {
                if(!state.isInTableHead)
                {
                    ++state.currentTableRow;
                }
                state.currentTableColumn = -1;
                break;
            }
            case MD_BLOCK_TH:
            case MD_BLOCK_TD:
            {
                ++state.currentTableColumn;
                state.isInTableCell = true;
                break;
            }
            case MD_BLOCK_DOC:
            case MD_BLOCK_QUOTE:
            case MD_BLOCK_HR:
            case MD_BLOCK_HTML:
                break;
        }
        return 0;
    }

    static int onLeaveBlock(MD_BLOCKTYPE type, void*, void* userdata)
    {
        auto& state = *static_cast<ParseState*>(userdata);
        switch(type)
        {
            case MD_BLOCK_UL:
            case MD_BLOCK_OL:
            {
                if(!state.listStack.empty())
                {
                    state.listStack.pop_back();
                }
                state.insertLineBreak();
                break;
            }
            case MD_BLOCK_LI:
            {
                state.listItemDepth = std::max(0, state.listItemDepth - 1);
                state.insertText("\n");
                break;
            }
            case MD_BLOCK_P:
            {
                if(state.listItemDepth == 0)
                {
                    state.insertText("\n");
                }
                break;
            }
            case MD_BLOCK_H:
            {
                state.headingLevel = 0;
                state.insertText("\n");
                break;
            }
            case MD_BLOCK_CODE:
            {
                state.codeDepth = std::max(0, state.codeDepth - 1);
                state.insertText("\n");
                break;
            }
            case MD_BLOCK_TABLE:
            {
                if(state.isInTable && !state.tables.empty())
                {
                    state.isInTable = false;
                    state.isInTableHead = false;
                    state.isInTableCell = false;
                    state.currentTableRow = -1;
                    state.currentTableColumn = -1;

                    auto& table = state.tables.back();
                    auto const start = state.editor.getCaretPosition();
                    state.isLink = true;
                    state.insertText(juce::String("Table TINDEX").replace("TINDEX", juce::String(state.tables.size())));
                    state.isLink = false;
                    state.insertText(" ");
                    state.insertText(juce::String(juce::CharPointer_UTF8("\xf0\x9f\x94\x8d "))); // 🔍
                    auto const end = state.editor.getCaretPosition();
                    state.insertLineBreak();
                    table.first = {start, end};
                }
                break;
            }
            case MD_BLOCK_THEAD:
            {
                state.isInTableHead = false;
                break;
            }
            case MD_BLOCK_TBODY:
            {
                state.currentTableRow = -1;
                break;
            }
            case MD_BLOCK_TR:
            {
                state.currentTableColumn = -1;
                break;
            }
            case MD_BLOCK_TH:
            case MD_BLOCK_TD:
            {
                state.isInTableCell = false;
                break;
            }

            case MD_BLOCK_DOC:
            case MD_BLOCK_QUOTE:
            case MD_BLOCK_HR:
            case MD_BLOCK_HTML:
                break;
        }
        return 0;
    }

    static int onEnterSpan(MD_SPANTYPE type, void* detail, void* userdata)
    {
        auto& state = *static_cast<ParseState*>(userdata);
        switch(type)
        {
            case MD_SPAN_EM:
            {
                ++state.emphasisDepth;
                break;
            }
            case MD_SPAN_STRONG:
            {
                ++state.strongDepth;
                break;
            }
            case MD_SPAN_CODE:
            {
                ++state.codeDepth;
                break;
            }
            case MD_SPAN_A:
            {
                if(auto* link = static_cast<MD_SPAN_A_DETAIL const*>(detail))
                {
                    state.isLink = true;
                    auto const start = state.editor.getCaretPosition();
                    auto const url = juce::String::fromUTF8(link->href.text, static_cast<int>(link->href.size));
                    state.links.push_back(std::make_pair(juce::Range<int>{start, start}, juce::URL(url)));
                }
                break;
            }
            case MD_SPAN_WIKILINK:
            case MD_SPAN_IMG:
            case MD_SPAN_DEL:
            case MD_SPAN_LATEXMATH:
            case MD_SPAN_LATEXMATH_DISPLAY:
            case MD_SPAN_U:
                break;
        }
        return 0;
    }

    static int onLeaveSpan(MD_SPANTYPE type, void*, void* userdata)
    {
        auto& state = *static_cast<ParseState*>(userdata);
        switch(type)
        {
            case MD_SPAN_EM:
            {
                state.emphasisDepth = std::max(0, state.emphasisDepth - 1);
                break;
            }
            case MD_SPAN_STRONG:
            {
                state.strongDepth = std::max(0, state.strongDepth - 1);
                break;
            }
            case MD_SPAN_CODE:
            {
                state.codeDepth = std::max(0, state.codeDepth - 1);
                break;
            }
            case MD_SPAN_A:
            {
                if(!state.links.empty() && state.isLink)
                {
                    state.links.back().first.setEnd(state.editor.getCaretPosition());
                    state.isLink = false;
                }
                break;
            }
            case MD_SPAN_WIKILINK:
            case MD_SPAN_IMG:
            case MD_SPAN_DEL:
            case MD_SPAN_LATEXMATH:
            case MD_SPAN_LATEXMATH_DISPLAY:
            case MD_SPAN_U:
                break;
        }
        return 0;
    }

    static int onText(MD_TEXTTYPE type, MD_CHAR const* text, MD_SIZE size, void* userdata)
    {
        auto& state = *static_cast<ParseState*>(userdata);
        switch(type)
        {
            case MD_TEXT_BR:
            case MD_TEXT_SOFTBR:
            {
                state.insertText("\n");
                break;
            }
            case MD_TEXT_NULLCHAR:
            {
                state.insertText(juce::String(juce::CharPointer_UTF8("\xef\xbf\xbd"))); // �
                break;
            }
            case MD_TEXT_NORMAL:
            case MD_TEXT_ENTITY:
            case MD_TEXT_CODE:
            case MD_TEXT_HTML:
            case MD_TEXT_LATEXMATH:
            {
                state.insertText(juce::String::fromUTF8(text, static_cast<int>(size)));
                break;
            }
        }
        return 0;
    }
};

juce::String Application::Neuralyzer::ChatHistory::TableContent::toCsv() const
{
    auto const toString = [&](std::vector<juce::String> const& row)
    {
        juce::StringArray array;
        array.ensureStorageAllocated(static_cast<int>(row.size()));
        for(auto const& column : row)
        {
            array.add(column);
        }
        return array.joinIntoString(",");
    };
    juce::StringArray content;
    if(std::any_of(headers.cbegin(), headers.cend(), [](auto const& header)
                   {
                       return header.isNotEmpty();
                   }))
    {
        content.add(toString(headers));
    }
    for(auto const& row : rows)
    {
        content.add(toString(row));
    }
    return content.joinIntoString("\n");
}

Application::Neuralyzer::ChatHistory::ChatHistory()
{
    setReadOnly(true);
    setMultiLine(true);
    setScrollbarsShown(true);
    setCaretVisible(true);
}

void Application::Neuralyzer::ChatHistory::mouseMove(juce::MouseEvent const& event)
{
    auto const textIndex = getTextIndexAt(event.x, event.y);
    auto const containIndex = [&](auto const& pair)
    {
        return pair.first.contains(textIndex);
    };
    if(std::any_of(mLinks.cbegin(), mLinks.cend(), containIndex) || std::any_of(mTables.cbegin(), mTables.cend(), containIndex))
    {
        setMouseCursor(juce::MouseCursor(juce::MouseCursor::StandardCursorType::PointingHandCursor));
    }
    else
    {
        setMouseCursor(juce::MouseCursor(juce::MouseCursor::StandardCursorType::IBeamCursor));
        juce::TextEditor::mouseMove(event);
    }
}

void Application::Neuralyzer::ChatHistory::mouseDown(juce::MouseEvent const& event)
{
    auto const textIndex = getTextIndexAt(event.x, event.y);
    auto const containIndex = [&](auto const& pair)
    {
        return pair.first.contains(textIndex);
    };
    auto const linkIt = std::find_if(mLinks.cbegin(), mLinks.cend(), containIndex);
    auto const tableIt = std::find_if(mTables.cbegin(), mTables.cend(), containIndex);
    if(linkIt != mLinks.cend())
    {
        if(linkIt->second.isLocalFile())
        {
            linkIt->second.getLocalFile().startAsProcess();
        }
        else
        {
            linkIt->second.launchInDefaultBrowser();
        }
    }
    else if(tableIt != mTables.cend())
    {
        auto const bounds = localAreaToGlobal(getTextBounds(tableIt->first).getBounds());
        Tooltip::showCallOutBox(std::make_unique<TableView>(tableIt->second), bounds);
    }
    juce::TextEditor::mouseDown(event);
}

void Application::Neuralyzer::ChatHistory::addPopupMenuItems(juce::PopupMenu& menu, [[maybe_unused]] juce::MouseEvent const* mouseClickEvent)
{
    menu.addItem(juce::StandardApplicationCommandIDs::copy, juce::translate("Copy"), !getHighlightedRegion().isEmpty());
    menu.addItem(juce::StandardApplicationCommandIDs::selectAll, juce::translate("Select All"));
    if(onShowPopupMenu != nullptr)
    {
        onShowPopupMenu(menu);
    }
}

void Application::Neuralyzer::ChatHistory::setHistory(Agent::History const& history)
{
    auto const messageColour = getLookAndFeel().findColour(juce::TextEditor::ColourIds::textColourId);
    auto const userName = juce::String(juce::CharPointer_UTF8("\xf0\x9f\x91\xa4 ")) + juce::translate("User"); // 👤
    auto const userColour = messageColour;
    auto const assistantName = juce::String(juce::CharPointer_UTF8("\xf0\x9f\xa4\x96 ")) + juce::translate("Assistant"); // 🤖
    auto const assistantColour = messageColour;
    auto const errorName = juce::String(juce::CharPointer_UTF8("\xe2\x9a\xa0\xef\xb8\x8f ")) + juce::translate("Error"); // ⚠️
    auto const errorColour = juce::Colours::red;
    auto const font = juce::Font(juce::FontOptions(14.0f));

    mTables.clear();
    mLinks.clear();
    auto const addText = [&](auto const& role, auto const& request, auto const& colour)
    {
        setColour(juce::TextEditor::ColourIds::textColourId, colour);
        setFont(font.boldened());
        insertTextAtCaret(role + ":\n");
        setColour(juce::TextEditor::ColourIds::textColourId, messageColour);
        setFont(font);
        auto state = ParseState{*this};

        MD_PARSER parser{};
        parser.abi_version = 0;
        parser.flags = MD_FLAG_PERMISSIVEAUTOLINKS | MD_FLAG_TABLES | MD_FLAG_STRIKETHROUGH | MD_FLAG_TASKLISTS | MD_FLAG_HARD_SOFT_BREAKS;
        parser.enter_block = &ParseState::onEnterBlock;
        parser.leave_block = &ParseState::onLeaveBlock;
        parser.enter_span = &ParseState::onEnterSpan;
        parser.leave_span = &ParseState::onLeaveSpan;
        parser.text = &ParseState::onText;

        auto const utf8 = request.toUTF8();
        [[maybe_unused]] auto const result = md_parse(utf8.getAddress(), static_cast<MD_SIZE>(std::strlen(utf8.getAddress())), &parser, &state);
        MiscWeakAssert(result == 0);

        insertTextAtCaret("\n");
    };

    clear();
    for(auto const& [role, request] : history)
    {
        switch(role)
        {
            case Agent::MessageType::user:
            {
                addText(userName, request, userColour);
                break;
            }
            case Agent::MessageType::assistant:
            {
                addText(assistantName, request, assistantColour);
                break;
            }
            case Agent::MessageType::warning:
            {
                addText(errorName, request, errorColour);
                break;
            }
            case Agent::MessageType::system:
            case Agent::MessageType::tool:
            case Agent::MessageType::media:
                break;
        }
    }
    scrollDown();
}

ANALYSE_FILE_END
