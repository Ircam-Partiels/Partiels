#include "AnlApplicationNeuralyzerAst.h"
#include <md4c.h>
extern "C"
{
#include <entity.h>
}

ANALYSE_FILE_BEGIN

namespace
{
    struct ParseState
    {
        Application::Neuralyzer::Ast::Document document;

        std::vector<juce::String> currentHierarchy;
        std::vector<std::optional<unsigned>> listIndices; // The next item number of ordered lists, nothing for unordered lists
        bool includeImages = false;
        bool inHeading = false;
        bool inTable = false;
        bool inTableHead = false;
        bool inWrittenLink = false;
        bool inWrittenImage = false;
        bool rowHasHeaderCells = false;
        size_t currentRowCellCount = 0;
    };

    juce::String toString(MD_ATTRIBUTE const& attribute)
    {
        return juce::String::fromUTF8(attribute.text, static_cast<int>(attribute.size));
    }

    // The text of the headings is used to create the identifiers of the resources, so it only receives the text
    juce::String& getContent(ParseState& state)
    {
        return state.inHeading ? state.currentHierarchy.back() : state.document.sections.back().content;
    }

    void ensureNewLine(juce::String& content)
    {
        if(content.isNotEmpty() && !content.endsWithChar('\n'))
        {
            content << "\n";
        }
    }

    juce::String decodeEntity(juce::String const& entity)
    {
        if(entity.length() > 3 && entity[1] == '#')
        {
            auto const isHexadecimal = entity[2] == 'x' || entity[2] == 'X';
            auto const digits = entity.substring(isHexadecimal ? 3 : 2, entity.length() - 1);
            auto const codepoint = isHexadecimal ? digits.getHexValue64() : digits.getLargeIntValue();
            auto const isValid = codepoint > 0 && codepoint <= 0x10FFFF && (codepoint < 0xD800 || codepoint > 0xDFFF);
            return juce::String::charToString(isValid ? static_cast<juce::juce_wchar>(codepoint) : static_cast<juce::juce_wchar>(0xFFFD));
        }
        auto const utf8 = entity.toUTF8();
        if(auto const* definition = entity_lookup(utf8.getAddress(), utf8.sizeInBytes() - 1_z))
        {
            juce::String result = juce::String::charToString(static_cast<juce::juce_wchar>(definition->codepoints[0]));
            if(definition->codepoints[1] != 0u)
            {
                result << juce::String::charToString(static_cast<juce::juce_wchar>(definition->codepoints[1]));
            }
            return result;
        }
        return entity;
    }

    int astEnterBlock(MD_BLOCKTYPE type, void* detail, void* userdata)
    {
        auto& state = *static_cast<ParseState*>(userdata);
        // Start a new section for each heading.
        if(type == MD_BLOCK_H)
        {
            state.document.sections.emplace_back();
        }

        auto& section = state.document.sections.back();
        switch(type)
        {
            case MD_BLOCK_H:
            {
                state.inHeading = true;
                auto const* headingDetail = static_cast<MD_BLOCK_H_DETAIL const*>(detail);
                state.currentHierarchy.resize(headingDetail != nullptr ? static_cast<size_t>(headingDetail->level) : 1_z);
                state.currentHierarchy.back().clear();
                break;
            }
            case MD_BLOCK_TABLE:
            {
                state.inTable = true;
                ensureNewLine(section.content);
                break;
            }
            case MD_BLOCK_CODE:
            {
                ensureNewLine(section.content);
                section.content << "```";
                if(auto const* codeDetail = static_cast<MD_BLOCK_CODE_DETAIL const*>(detail))
                {
                    section.content << toString(codeDetail->lang);
                }
                section.content << "\n";
                break;
            }
            case MD_BLOCK_UL:
            {
                state.listIndices.push_back(std::nullopt);
                ensureNewLine(section.content);
                break;
            }
            case MD_BLOCK_OL:
            {
                auto const* listDetail = static_cast<MD_BLOCK_OL_DETAIL const*>(detail);
                state.listIndices.push_back(listDetail != nullptr ? listDetail->start : 1u);
                ensureNewLine(section.content);
                break;
            }
            case MD_BLOCK_LI:
            {
                if(state.listIndices.empty())
                {
                    break;
                }
                auto const numList = static_cast<int>(state.listIndices.size());
                ensureNewLine(section.content);
                section.content << juce::String::repeatedString("  ", numList - 1);
                auto& listIndex = state.listIndices.back();
                if(!listIndex.has_value())
                {
                    section.content << "- ";
                }
                else
                {
                    section.content << juce::String(listIndex.value()) << ". ";
                    ++listIndex.value();
                }
                auto const* itemDetail = static_cast<MD_BLOCK_LI_DETAIL const*>(detail);
                if(itemDetail != nullptr && itemDetail->is_task != 0)
                {
                    section.content << (itemDetail->task_mark == ' ' ? "[ ] " : "[x] ");
                }
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
            case MD_BLOCK_FOOTNOTE_DEF_SECTION:
            case MD_BLOCK_FOOTNOTE_DEF:
            case MD_BLOCK_ADMONITION:
            case MD_BLOCK_BLANK:
                break;
        }
        return 0;
    }

    int astLeaveBlock(MD_BLOCKTYPE type, void*, void* userdata)
    {
        auto& state = *static_cast<ParseState*>(userdata);
        auto& section = state.document.sections.back();
        switch(type)
        {
            case MD_BLOCK_H:
            {
                // The empty headings (skipped levels) are ignored
                state.inHeading = false;
                section.hierarchy.clear();
                std::copy_if(state.currentHierarchy.cbegin(), state.currentHierarchy.cend(), std::back_inserter(section.hierarchy), [](auto const& heading)
                             {
                                 return heading.isNotEmpty();
                             });
                break;
            }
            case MD_BLOCK_UL:
            case MD_BLOCK_OL:
            {
                state.listIndices.pop_back();
                break;
            }
            case MD_BLOCK_LI:
            case MD_BLOCK_P:
            {
                section.content << "\n";
                break;
            }
            case MD_BLOCK_CODE:
            {
                ensureNewLine(section.content);
                section.content << "```\n";
                break;
            }
            case MD_BLOCK_DOC:
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
            case MD_BLOCK_FOOTNOTE_DEF_SECTION:
            case MD_BLOCK_FOOTNOTE_DEF:
            case MD_BLOCK_ADMONITION:
            case MD_BLOCK_BLANK:
                break;
        }
        return 0;
    }

    int astEnterSpan(MD_SPANTYPE type, void* detail, void* userdata)
    {
        auto& state = *static_cast<ParseState*>(userdata);
        auto& content = getContent(state);
        switch(type)
        {
            case MD_SPAN_A:
            {
                // The internal links (anchors) and the autolinks (that already display the URL) only keep their text
                auto const* link = static_cast<MD_SPAN_A_DETAIL const*>(detail);
                state.inWrittenLink = !state.inHeading && link != nullptr && link->is_autolink == 0 && !toString(link->href).startsWithChar('#');
                if(state.inWrittenLink)
                {
                    content << "[";
                }
                break;
            }
            case MD_SPAN_IMG:
            {
                state.inWrittenImage = state.includeImages && !state.inHeading && detail != nullptr;
                if(state.inWrittenImage)
                {
                    content << "![";
                }
                break;
            }
            case MD_SPAN_CODE:
            {
                content << "`";
                break;
            }
            case MD_SPAN_DEL:
            {
                content << "~~";
                break;
            }
            case MD_SPAN_EM:
            case MD_SPAN_STRONG:
            case MD_SPAN_LATEXMATH:
            case MD_SPAN_LATEXMATH_DISPLAY:
            case MD_SPAN_WIKILINK:
            case MD_SPAN_U:
            case MD_SPAN_SPOILER:
            case MD_SPAN_SUPERSCRIPT:
            case MD_SPAN_SUBSCRIPT:
            case MD_SPAN_FOOTNOTE_REF:
            case MD_SPAN_MARK:
            case MD_SPAN_INS:
                break;
        }
        return 0;
    }

    int astLeaveSpan(MD_SPANTYPE type, void* detail, void* userdata)
    {
        auto& state = *static_cast<ParseState*>(userdata);
        auto& content = getContent(state);
        auto const writeDestination = [&](MD_ATTRIBUTE const& destination, MD_ATTRIBUTE const& title)
        {
            content << "](" << toString(destination);
            if(title.size > 0)
            {
                content << " \"" << toString(title) << "\"";
            }
            content << ")";
        };
        switch(type)
        {
            case MD_SPAN_A:
            {
                if(std::exchange(state.inWrittenLink, false))
                {
                    auto const& link = *static_cast<MD_SPAN_A_DETAIL const*>(detail);
                    writeDestination(link.href, link.title);
                }
                break;
            }
            case MD_SPAN_IMG:
            {
                if(std::exchange(state.inWrittenImage, false))
                {
                    auto const& image = *static_cast<MD_SPAN_IMG_DETAIL const*>(detail);
                    writeDestination(image.src, image.title);
                }
                break;
            }
            case MD_SPAN_CODE:
            {
                content << "`";
                break;
            }
            case MD_SPAN_DEL:
            {
                content << "~~";
                break;
            }
            case MD_SPAN_EM:
            case MD_SPAN_STRONG:
            case MD_SPAN_LATEXMATH:
            case MD_SPAN_LATEXMATH_DISPLAY:
            case MD_SPAN_WIKILINK:
            case MD_SPAN_U:
            case MD_SPAN_SPOILER:
            case MD_SPAN_SUPERSCRIPT:
            case MD_SPAN_SUBSCRIPT:
            case MD_SPAN_FOOTNOTE_REF:
            case MD_SPAN_MARK:
            case MD_SPAN_INS:
                break;
        }
        return 0;
    }

    int astText(MD_TEXTTYPE type, MD_CHAR const* text, MD_SIZE size, void* userdata)
    {
        auto& state = *static_cast<ParseState*>(userdata);
        auto& content = getContent(state);
        // The pipes of the cells are escaped so they are not interpreted as cell separators (the code spans already
        // contain the escaped pipes of the source)
        auto const escape = [&](juce::String const& value)
        {
            return state.inTable ? value.replace("|", "\\|") : value;
        };
        switch(type)
        {
            case MD_TEXT_BR:
            case MD_TEXT_SOFTBR:
            case MD_TEXT_NULLCHAR:
            {
                content << " ";
                break;
            }
            case MD_TEXT_HTML:
                break;
            case MD_TEXT_ENTITY:
            {
                content << escape(decodeEntity(juce::String::fromUTF8(text, static_cast<int>(size))));
                break;
            }
            case MD_TEXT_NORMAL:
            case MD_TEXT_LATEXMATH:
            {
                content << escape(juce::String::fromUTF8(text, static_cast<int>(size)));
                break;
            }
            case MD_TEXT_CODE:
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
    // The content before the first heading is stored in a section named after the document
    state.document.sections.push_back({{}, {name}});

    MD_PARSER parser{};
    parser.abi_version = 0;
    parser.flags = MD_FLAG_PERMISSIVEAUTOLINKS | MD_FLAG_STRIKETHROUGH | MD_FLAG_TASKLISTS | MD_FLAG_TABLES;
    parser.enter_block = &astEnterBlock;
    parser.leave_block = &astLeaveBlock;
    parser.enter_span = &astEnterSpan;
    parser.leave_span = &astLeaveSpan;
    parser.text = &astText;

    auto const trimmedContent = content.trim();
    auto const utf8 = trimmedContent.toUTF8();
    // The size must exclude the null terminator, otherwise it is parsed as a character of the last line
    [[maybe_unused]] auto const result = md_parse(utf8.getAddress(), static_cast<MD_SIZE>(utf8.sizeInBytes() - 1_z), &parser, &state);
    MiscWeakAssert(result == 0);

    if(state.document.sections.front().content.trim().isEmpty())
    {
        state.document.sections.erase(state.document.sections.begin());
    }
    return std::move(state.document);
}

class AstTest
: public juce::UnitTest
{
public:
    AstTest()
    : juce::UnitTest("Neuralyzer Ast", "Application")
    {
    }

    ~AstTest() override = default;

    void runTest() override
    {
        namespace Ast = Application::Neuralyzer::Ast;

        beginTest("Null Terminator");
        {
            auto const paragraph = Ast::parseMarkdown("# Title\nLast line", "Document");
            expect(paragraph.sections.size() == 1_z);
            expectEquals(paragraph.sections.front().content, juce::String("Last line\n"));

            auto const code = Ast::parseMarkdown("# Title\nText\n\n```\nint a;\n```", "Document");
            expect(code.sections.size() == 1_z);
            auto const& content = code.sections.front().content;
            // The closing fence must not be parsed as a line of code
            expectEquals(content.fromFirstOccurrenceOf("int a;", false, false).trim(), juce::String("```"));
        }

        beginTest("Ordered List Start");
        {
            auto const document = Ast::parseMarkdown("# Title\n1. First\n\n- Item\n\n3. Third\n4. Fourth\n\nText\n\n0. Zero", "Document");
            expect(document.sections.size() == 1_z);
            auto const& content = document.sections.front().content;
            expect(content.contains("1. First"), content);
            expect(content.contains("- Item"), content);
            expect(content.contains("3. Third"), content);
            expect(content.contains("4. Fourth"), content);
            expect(content.contains("0. Zero"), content);
        }

        beginTest("Heading Spans");
        {
            auto const document = Ast::parseMarkdown("# Use [the link](#anchor) and ![the image](image.png) here\nText", "Document", true);
            expect(document.sections.size() == 1_z);
            auto const& section = document.sections.front();
            expect(section.hierarchy.size() == 1_z);
            expectEquals(section.hierarchy.front(), juce::String("Use the link and the image here"));
        }

        beginTest("Links And Images");
        {
            auto const text = juce::String("# Title\nSee [the site](https://www.ircam.fr \"IRCAM\"), [the manual](https://partiels.ircam.fr), [the section](#section), https://ircam.fr and ![the logo](logo.png)");
            auto const withImages = Ast::parseMarkdown(text, "Document", true);
            expect(withImages.sections.size() == 1_z);
            expectEquals(withImages.sections.front().content, juce::String("See [the site](https://www.ircam.fr \"IRCAM\"), [the manual](https://partiels.ircam.fr), the section, https://ircam.fr and ![the logo](logo.png)\n"));
            auto const withoutImages = Ast::parseMarkdown(text, "Document", false);
            expect(withoutImages.sections.size() == 1_z);
            expect(withoutImages.sections.front().content.endsWith(" and the logo\n"), withoutImages.sections.front().content);
        }

        beginTest("Code Blocks");
        {
            auto const document = Ast::parseMarkdown("# Title\n- Item\n  ```json\n  {}\n  ```\n\nText", "Document");
            expect(document.sections.size() == 1_z);
            expectEquals(document.sections.front().content, juce::String("- Item\n```json\n{}\n```\n\nText\n"));
        }

        beginTest("Extensions");
        {
            auto const document = Ast::parseMarkdown("# Title\n- [ ] To do\n- [x] Done\n\n~~Removed~~ &amp; &lt;kept&gt; &#233; &#x1F600; &unknown;", "Document");
            expect(document.sections.size() == 1_z);
            expectEquals(document.sections.front().content, juce::String("- [ ] To do\n- [x] Done\n~~Removed~~ & <kept> ") + juce::String::charToString(0xE9) + " " + juce::String::charToString(0x1F600) + " &unknown;\n");
        }

        beginTest("Tables");
        {
            auto const document = Ast::parseMarkdown("# Title\n| Key | Value |\n| - | - |\n| a \\| b | `c \\| d` |", "Document");
            expect(document.sections.size() == 1_z);
            expectEquals(document.sections.front().content, juce::String("| Key | Value |\n| --- | --- |\n| a \\| b | `c \\| d` |\n\n"));
        }

        beginTest("Hierarchy");
        {
            auto const document = Ast::parseMarkdown("Introduction\n# Title\n### Subsection\nText\n## Section\nText", "Document");
            expect(document.sections.size() == 4_z);
            if(document.sections.size() == 4_z)
            {
                expect(document.sections.at(0).hierarchy == std::vector<juce::String>{"Document"});
                expectEquals(document.sections.at(0).content, juce::String("Introduction\n"));
                expect(document.sections.at(1).hierarchy == std::vector<juce::String>{"Title"});
                expect(document.sections.at(2).hierarchy == std::vector<juce::String>{"Title", "Subsection"});
                expect(document.sections.at(3).hierarchy == std::vector<juce::String>{"Title", "Section"});
            }
            expect(Ast::parseMarkdown("<p>HTML</p>\n\n# Title\nText", "Document").sections.size() == 1_z);
            expect(Ast::parseMarkdown("Text without heading", "Document").sections.size() == 1_z);
        }
    }
};

static AstTest astTest;

ANALYSE_FILE_END
