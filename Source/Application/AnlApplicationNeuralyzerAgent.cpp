#include "AnlApplicationNeuralyzerAgent.h"

ANALYSE_FILE_BEGIN

std::string Application::Neuralyzer::Agent::getErrorInstuction(size_t const maxErrors, size_t const numErrors)
{
    if(numErrors == 0_z)
    {
        return "Based on these results, provide your final answer or call more tools if needed. If necessary, ensure your response is accurate based on the current state of the document.";
    }
    if(maxErrors > 0_z && numErrors < maxErrors - 1_z)
    {
        // The tool call failed so we provide a warning to the model.
        return "Warning: The previous tool call had error. Be careful with further tool calls. Call one tool at a time and split complex tasks into smaller steps. Ensure the request is consistent with the actual state of the document.";
    }
    if(maxErrors > 0_z && numErrors == maxErrors - 1_z)
    {
        // The tool call failed for too many consecutive iterations,
        // we disable tools for the next iteration and provide a warning
        // to the model.
        return "Warning: The previous tool calls had too many errors. Tools will be disabled for the next iteration. Please provide a final answer without calling tools.";
    }
    // The tool call failed for too many consecutive iterations,
    // we abort the entire query.
    return "Error: The previous tool calls had too many errors. The query will be aborted.";
}

std::string Application::Neuralyzer::Agent::callTools(Mcp::Dispatcher& mcpDispatcher, std::vector<common_chat_tool_call> const& toolCalls, size_t const maxErrors, size_t& numErrors)
{
    auto const [callSucceeded, toolResponses] = mcpDispatcher.callTools(toolCalls);
    if(callSucceeded && toolResponses.empty())
    {
        return {};
    }
    std::string newQuery;
    for(auto const& toolCall : toolResponses)
    {
        newQuery += toolCall + "\n\n";
    }
    numErrors = callSucceeded ? 0_z : numErrors + 1_z;
    return newQuery + getErrorInstuction(maxErrors, numErrors);
}

ANALYSE_FILE_END
