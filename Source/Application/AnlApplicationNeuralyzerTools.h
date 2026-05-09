#pragma once

#include "AnlApplicationNeuralyzerMcp.h"
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wshadow-field-in-constructor", "-Wimplicit-float-conversion", "-Wunused-function", "-Wzero-as-null-pointer-constant", "-Wfloat-equal", "-Wsign-conversion", "-Wdeprecated-copy-with-dtor", "-Wmissing-noreturn", "-Winconsistent-missing-destructor-override")
#include <chat.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
#include <llama-cpp.h>

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        namespace Tools
        {
            std::pair<juce::Result, std::vector<common_chat_tool_call>> parse(common_chat_params const& chatParams, std::string assistantResponse);
            std::pair<juce::Result, std::string> call(Mcp::Dispatcher& dispatcher, std::vector<common_chat_tool_call> const& toolCalls);
        } // namespace Tools
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
