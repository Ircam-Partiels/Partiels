#pragma once

#include "AnlBase.h"

ANALYSE_FILE_BEGIN

namespace Formatter
{
    //! @brief Parses the markdown content and adds it to the text editor
    //! @details The methods only support font size (titles), font style, and lists (numbered and bullet)
    bool addMarkdown(juce::TextEditor& editor, juce::String const& content);
} // namespace Formatter

ANALYSE_FILE_END
