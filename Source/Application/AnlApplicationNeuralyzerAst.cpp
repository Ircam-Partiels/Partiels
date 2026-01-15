#include "AnlApplicationNeuralyzerAst.h"
#include <md4c.h>

ANALYSE_FILE_BEGIN

namespace
{
    struct ParseState
    {
        Application::Neuralyzer::Ast::Document document;

        std::vector<juce::String> currentHierarchy;
        juce::String listIndent;
        bool includeImages = false;
        bool inHeading = false;
        bool inListItem = false;
        bool inCodeBlock = false;
        bool inTable = false;
        bool inTableHead = false;
        bool rowHasHeaderCells = false;
        size_t currentRowCellCount = 0;
    };

    static int astEnterBlock(MD_BLOCKTYPE type, void* detail, void* userdata)
    {
        auto& state = *static_cast<ParseState*>(userdata);
        // Start a new section for each heading.
        if(type == MD_BLOCK_H)
        {
            state.document.sections.emplace_back();
        }
        if(state.document.sections.empty())
        {
            return 0;
        }

        auto& section = state.document.sections.back();
        switch(type)
        {
            case MD_BLOCK_H:
            {
                state.inHeading = true;
                if(auto const* headingDetail = static_cast<MD_BLOCK_H_DETAIL const*>(detail))
                {
                    state.currentHierarchy.resize(static_cast<size_t>(headingDetail->level));
                }
                state.currentHierarchy.back().clear();
                break;
            }
            case MD_BLOCK_TABLE:
            {
                state.inTable = true;
                break;
            }
            case MD_BLOCK_CODE:
            {
                state.inCodeBlock = true;
                section.content << "```\n";
                break;
            }
            case MD_BLOCK_UL:
            case MD_BLOCK_OL:
            {
                if(state.inListItem || state.listIndent.isNotEmpty())
                {
                    state.listIndent << "  ";
                }
                break;
            }
            case MD_BLOCK_LI:
            {
                state.inListItem = true;
                section.content << state.listIndent + "- ";
                break;
            }
            case MD_BLOCK_P:
            case MD_BLOCK_DOC:
            case MD_BLOCK_QUOTE:
            case MD_BLOCK_HR:
            case MD_BLOCK_HTML:
                break;
            case MD_BLOCK_THEAD:
            {
                state.inTableHead = true;
                break;
            }
            case MD_BLOCK_TBODY:
                break;
            case MD_BLOCK_TR:
            {
                state.currentRowCellCount = 0;
                state.rowHasHeaderCells = false;
                section.content << "| ";
                break;
            }
            case MD_BLOCK_TH:
            {
                if(state.currentRowCellCount > 0)
                {
                    section.content << " | ";
                }
                state.rowHasHeaderCells = true;
                ++state.currentRowCellCount;
                break;
            }
            case MD_BLOCK_TD:
            {
                if(state.currentRowCellCount > 0)
                {
                    section.content << " | ";
                }
                ++state.currentRowCellCount;
                break;
            }
        }
        return 0;
    }

    static int astLeaveBlock(MD_BLOCKTYPE type, void*, void* userdata)
    {
        auto& state = *static_cast<ParseState*>(userdata);
        if(state.document.sections.empty() || state.currentHierarchy.empty())
        {
            return 0;
        }

        auto& section = state.document.sections.back();
        switch(type)
        {
            case MD_BLOCK_DOC:
            case MD_BLOCK_H:
            {
                state.inHeading = false;
                section.hierarchy = state.currentHierarchy;
                break;
            }
            case MD_BLOCK_UL:
            case MD_BLOCK_OL:
            {
                state.listIndent = state.listIndent.dropLastCharacters(2);
                break;
            }
            case MD_BLOCK_LI:
            {
                state.inListItem = false;
                section.content << "\n";
                break;
            }
            case MD_BLOCK_P:
            {
                section.content << (state.inTable ? " " : "\n");
                break;
            }
            case MD_BLOCK_CODE:
            {
                state.inCodeBlock = false;
                section.content << "\n```\n\n";
                break;
            }
            case MD_BLOCK_QUOTE:
            case MD_BLOCK_HR:
            case MD_BLOCK_HTML:
                break;
            case MD_BLOCK_TABLE:
            {
                state.inTable = false;
                section.content << "\n";
                break;
            }
            case MD_BLOCK_THEAD:
            {
                state.inTableHead = false;
                break;
            }
            case MD_BLOCK_TBODY:
                break;
            case MD_BLOCK_TR:
            {
                section.content << " |\n";
                if(state.inTableHead && state.rowHasHeaderCells)
                {
                    auto& content = section.content;
                    content << "|";
                    for(size_t i = 0; i < state.currentRowCellCount; ++i)
                    {
                        content << " --- |";
                    }
                    content << "\n";
                }
                break;
            }
            case MD_BLOCK_TH:
            case MD_BLOCK_TD:
                break;
        }
        return 0;
    }

    static int astEnterSpan(MD_SPANTYPE type, void* detail, void* userdata)
    {
        auto& state = *static_cast<ParseState*>(userdata);
        if(state.document.sections.empty() || state.currentHierarchy.empty())
        {
            return 0;
        }

        auto& section = state.document.sections.back();
        auto& content = state.inHeading ? state.currentHierarchy.back() : section.content;
        if(type == MD_SPAN_CODE)
        {
            content << "`";
        }
        else if(type == MD_SPAN_IMG && state.includeImages)
        {
            if(auto const* imageDetail = static_cast<MD_SPAN_IMG_DETAIL const*>(detail))
            {
                content << "[Image: " + juce::String::fromUTF8(imageDetail->src.text, static_cast<int>(imageDetail->src.size)) + "] ";
            }
        }
        return 0;
    }

    static int astLeaveSpan(MD_SPANTYPE type, void*, void* userdata)
    {
        auto& state = *static_cast<ParseState*>(userdata);
        if(state.document.sections.empty() || state.currentHierarchy.empty())
        {
            return 0;
        }

        auto& section = state.document.sections.back();
        auto& content = state.inHeading ? state.currentHierarchy.back() : section.content;
        if(type == MD_SPAN_CODE)
        {
            content << "`";
        }
        return 0;
    }

    static int astText(MD_TEXTTYPE type, MD_CHAR const* text, MD_SIZE size, void* userdata)
    {
        auto& state = *static_cast<ParseState*>(userdata);
        if(state.document.sections.empty())
        {
            return 0;
        }

        auto& section = state.document.sections.back();
        auto& content = state.inHeading ? state.currentHierarchy.back() : section.content;
        switch(type)
        {
            case MD_TEXT_BR:
            case MD_TEXT_SOFTBR:
            {
                content << (state.inCodeBlock ? "\n" : " ");
                break;
            }
            case MD_TEXT_NULLCHAR:
            {
                content << " ";
                break;
            }
            case MD_TEXT_HTML:
                break;
            case MD_TEXT_NORMAL:
            case MD_TEXT_ENTITY:
            case MD_TEXT_CODE:
            case MD_TEXT_LATEXMATH:
            {
                content << juce::String::fromUTF8(text, static_cast<int>(size));
                break;
            }
        }
        return 0;
    }
} // namespace

Application::Neuralyzer::Ast::Document Application::Neuralyzer::Ast::parseMarkdown(juce::String const& content, juce::String const& name, bool includeImages)
{
    ParseState state;
    state.document.document = name;
    state.includeImages = includeImages;

    MD_PARSER parser{};
    parser.abi_version = 0;
    parser.flags = MD_FLAG_PERMISSIVEAUTOLINKS | MD_FLAG_STRIKETHROUGH | MD_FLAG_TASKLISTS | MD_FLAG_HARD_SOFT_BREAKS | MD_FLAG_TABLES;
    parser.enter_block = &astEnterBlock;
    parser.leave_block = &astLeaveBlock;
    parser.enter_span = &astEnterSpan;
    parser.leave_span = &astLeaveSpan;
    parser.text = &astText;

    auto const timmedContent = content.trim();
    auto const utf8 = timmedContent.toUTF8();
    md_parse(utf8.getAddress(), static_cast<MD_SIZE>(utf8.sizeInBytes()), &parser, &state);
    return std::move(state.document);
}

ANALYSE_FILE_END
