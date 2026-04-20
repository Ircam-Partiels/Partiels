#include "AnlFormatter.h"
#include <md4c.h>

ANALYSE_FILE_BEGIN

namespace
{
    struct ListState
    {
        bool ordered = false;
        int index = 1;
    };

    struct ParseState
    {
        juce::TextEditor& editor;
        juce::Font const baseFont;
        std::vector<ListState> listStack = {};
        int listItemDepth = 0;
        int headingLevel = 0;
        int emphasisDepth = 0;
        int strongDepth = 0;
        int codeDepth = 0;
        bool atLineStart = true;
    };

    static juce::Font getCurrentFont(ParseState const& state)
    {
        auto font = state.baseFont;
        if(state.headingLevel > 0)
        {
            auto const size = std::max(14.0f, 26.0f - static_cast<float>(state.headingLevel - 1) * 2.0f);
            font = juce::Font(juce::FontOptions(size)).boldened();
        }
        if(state.strongDepth > 0)
        {
            font = font.boldened();
        }
        if(state.emphasisDepth > 0)
        {
            font = font.italicised();
        }
        if(state.codeDepth > 0)
        {
            font = juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), font.getHeight() * 0.84f, juce::Font::FontStyleFlags::plain));
        }
        return font;
    }

    static void insertText(ParseState& state, juce::String const& text)
    {
        if(text.isEmpty())
        {
            return;
        }
        state.editor.setFont(getCurrentFont(state));
        state.editor.insertTextAtCaret(text);
        state.atLineStart = text.endsWithChar('\n');
    }

    static void insertLineBreak(ParseState& state)
    {
        if(!state.atLineStart)
        {
            insertText(state, "\n");
        }
    }

    static juce::String toString(MD_CHAR const* text, MD_SIZE size)
    {
        return juce::String::fromUTF8(text, static_cast<int>(size));
    }

    static int onEnterBlock(MD_BLOCKTYPE type, void* detail, void* userdata)
    {
        auto& state = *static_cast<ParseState*>(userdata);
        switch(type)
        {
            case MD_BLOCK_UL:
            {
                state.listStack.push_back({false, 1});
                insertLineBreak(state);
                break;
            }
            case MD_BLOCK_OL:
            {
                auto const* orderedDetail = static_cast<MD_BLOCK_OL_DETAIL const*>(detail);
                state.listStack.push_back({true, orderedDetail != nullptr ? static_cast<int>(orderedDetail->start) : 1});
                insertLineBreak(state);
                break;
            }
            case MD_BLOCK_LI:
            {
                ++state.listItemDepth;
                insertLineBreak(state);
                auto const nesting = std::max(0, static_cast<int>(state.listStack.size()) - 1);
                insertText(state, juce::String::repeatedString("  ", nesting));
                if(!state.listStack.empty())
                {
                    auto& list = state.listStack.back();
                    insertText(state, list.ordered ? (juce::String(list.index++) + ". ") : juce::String(juce::CharPointer_UTF8("\xe2\x80\xa2 "))); // 1. or •
                }
                auto const* listItemDetail = static_cast<MD_BLOCK_LI_DETAIL const*>(detail);
                if(listItemDetail != nullptr && listItemDetail->is_task)
                {
                    insertText(state, listItemDetail->task_mark == ' ' ? juce::String(juce::CharPointer_UTF8("\xe2\x98\x90 ")) : juce::String(juce::CharPointer_UTF8("\xe2\x98\x91 "))); // ☐ or ☑
                }
                break;
            }
            case MD_BLOCK_H:
            {
                insertLineBreak(state);
                auto const* headingDetail = static_cast<MD_BLOCK_H_DETAIL const*>(detail);
                state.headingLevel = headingDetail != nullptr ? static_cast<int>(headingDetail->level) : 1;
                break;
            }
            case MD_BLOCK_P:
            {
                if(state.listItemDepth == 0)
                {
                    insertLineBreak(state);
                }
                break;
            }
            case MD_BLOCK_CODE:
            {
                insertLineBreak(state);
                ++state.codeDepth;
                break;
            }
            case MD_BLOCK_DOC:
            case MD_BLOCK_QUOTE:
            case MD_BLOCK_HR:
            case MD_BLOCK_HTML:
            case MD_BLOCK_TABLE:
            case MD_BLOCK_THEAD:
            case MD_BLOCK_TBODY:
            case MD_BLOCK_TR:
            case MD_BLOCK_TH:
            case MD_BLOCK_TD:
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
                insertLineBreak(state);
                break;
            }
            case MD_BLOCK_LI:
            {
                state.listItemDepth = std::max(0, state.listItemDepth - 1);
                insertText(state, "\n");
                break;
            }
            case MD_BLOCK_P:
            {
                if(state.listItemDepth == 0)
                {
                    insertText(state, "\n");
                }
                break;
            }
            case MD_BLOCK_H:
            {
                state.headingLevel = 0;
                insertText(state, "\n");
                break;
            }
            case MD_BLOCK_CODE:
            {
                state.codeDepth = std::max(0, state.codeDepth - 1);
                insertText(state, "\n");
                break;
            }
            case MD_BLOCK_DOC:
            case MD_BLOCK_QUOTE:
            case MD_BLOCK_HR:
            case MD_BLOCK_HTML:
            case MD_BLOCK_TABLE:
            case MD_BLOCK_THEAD:
            case MD_BLOCK_TBODY:
            case MD_BLOCK_TR:
            case MD_BLOCK_TH:
            case MD_BLOCK_TD:
                break;
        }
        return 0;
    }

    static int onEnterSpan(MD_SPANTYPE type, void*, void* userdata)
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
            case MD_SPAN_IMG:
            case MD_SPAN_DEL:
            case MD_SPAN_LATEXMATH:
            case MD_SPAN_LATEXMATH_DISPLAY:
            case MD_SPAN_WIKILINK:
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
            case MD_SPAN_IMG:
            case MD_SPAN_DEL:
            case MD_SPAN_LATEXMATH:
            case MD_SPAN_LATEXMATH_DISPLAY:
            case MD_SPAN_WIKILINK:
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
                insertText(state, "\n");
                break;
            }
            case MD_TEXT_NULLCHAR:
            {
                insertText(state, juce::String(juce::CharPointer_UTF8("\xef\xbf\xbd"))); // �
                break;
            }
            case MD_TEXT_NORMAL:
            case MD_TEXT_ENTITY:
            case MD_TEXT_CODE:
            case MD_TEXT_HTML:
            case MD_TEXT_LATEXMATH:
            {
                insertText(state, toString(text, size));
                break;
            }
        }
        return 0;
    }
} // namespace

bool Formatter::addMarkdown(juce::TextEditor& editor, juce::String const& content)
{
    auto state = ParseState{editor, editor.getFont()};

    MD_PARSER parser{};
    parser.abi_version = 0;
    parser.flags = MD_FLAG_PERMISSIVEAUTOLINKS | MD_FLAG_STRIKETHROUGH | MD_FLAG_TASKLISTS | MD_FLAG_HARD_SOFT_BREAKS;
    parser.enter_block = &onEnterBlock;
    parser.leave_block = &onLeaveBlock;
    parser.enter_span = &onEnterSpan;
    parser.leave_span = &onLeaveSpan;
    parser.text = &onText;

    auto const utf8 = content.toUTF8();
    return md_parse(utf8.getAddress(), static_cast<MD_SIZE>(std::strlen(utf8.getAddress())), &parser, &state) == 0;
}

ANALYSE_FILE_END
