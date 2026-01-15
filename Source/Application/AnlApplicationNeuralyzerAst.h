#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        namespace Ast
        {
            //! @brief A part of a Markdown document that starts with a heading
            struct Section
            {
                juce::String content;                //!< The content of the section (without its heading) formatted as Markdown
                std::vector<juce::String> hierarchy; //!< The non-empty headings from the top level to the heading of the section
            };

            //! @brief A Markdown document split into sections
            struct Document
            {
                juce::String document;         //!< The name of the document
                std::vector<Section> sections; //!< The sections in the order of the document
            };

            //! @brief Parses a Markdown text and splits it into sections at each heading
            //! @details The content before the first heading is stored in a section whose hierarchy is the name of the
            //! document. The headings only keep the text of their links and images, the internal links (anchors) and the
            //! autolinks only keep their text, and the HTML is ignored.
            //! @param content The Markdown text
            //! @param name The name of the document
            //! @param includeImages Whether the images are kept as Markdown images (otherwise only their alternative text is kept)
            //! @return The parsed document
            Document parseMarkdown(juce::String const& content, juce::String const& name, bool includeImages = false);
        } // namespace Ast
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
