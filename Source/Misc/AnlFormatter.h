#pragma once

#include "AnlBase.h"

ANALYSE_FILE_BEGIN

namespace Formatter
{
    //! @brief Parses the markdown content and adds it to the text editor
    //! @details The methods only support font size (titles), font style, and lists (numbered and bullet)
    bool addMarkdown(juce::TextEditor& editor, juce::String const& content);

    //! @brief Converts a string into a slug usable as a stable resource identifier
    //! @details The value is trimmed, lowercased, non-alphanumeric characters are collapsed into dashes,
    //! and leading or trailing dashes are removed
    juce::String slugify(juce::String const& value);
} // namespace Formatter

ANALYSE_FILE_END
