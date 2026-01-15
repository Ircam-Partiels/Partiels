#include "AnlApplicationNeuralyzerTools.h"
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wshadow-field-in-constructor", "-Wimplicit-float-conversion", "-Wunused-function", "-Wzero-as-null-pointer-constant", "-Wfloat-equal", "-Wsign-conversion", "-Wdeprecated-copy-with-dtor", "-Wmissing-noreturn", "-Winconsistent-missing-destructor-override", "-Wshadow", "-Wfloat-conversion", "-Wshorten-64-to-32")
#include <../src/llama-model.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
#include <regex>

ANALYSE_FILE_BEGIN

std::pair<juce::Result, std::vector<common_chat_tool_call>> Application::Neuralyzer::Tools::parse(common_chat_params const& chatParams, std::string assistantResponse)
{
    auto const createResults = [](juce::Result result, std::vector<common_chat_tool_call> tc = {})
    {
        return std::make_pair(std::move(result), std::move(tc));
    };

    if(assistantResponse.empty())
    {
        return createResults(juce::Result::ok());
    }

    // Create parser config from stored chat params
    common_chat_parser_params parserParams(chatParams);
    if(!chatParams.parser.empty())
    {
        parserParams.parser.load(chatParams.parser);
    }

    try
    {
        // Use the appropriate parser based on format type
        auto toolCalls = common_chat_parse(assistantResponse, false, parserParams).tool_calls;

        // Tool calls are now in parsed.tool_calls
        if(toolCalls.empty())
        {
            MiscDebug("Application::Neuralyzer::Tools", "No tool calls found in response");
            return createResults(juce::Result::ok());
        }

        MiscDebug("Application::Neuralyzer::Tools", juce::String("Parsed ") + juce::String(toolCalls.size()) + juce::String(" tool calls"));
        return createResults(juce::Result::ok(), std::move(toolCalls));
    }
    catch(std::exception const& e)
    {
        MiscDebug("Application::Neuralyzer::Tools", "===============\n" + juce::String(juce::CharPointer_UTF8(e.what())) + "\nPARSER:\n" + chatParams.parser + "\nGRAMMAR:\n" + chatParams.grammar + "\n===============\n");
        return createResults(juce::Result::fail(e.what()));
    }
    catch(...)
    {
        MiscDebug("Application::Neuralyzer::Tools", "Unknown error");
        return createResults(juce::Result::fail("Unknown error."));
    }
}

std::pair<juce::Result, std::string> Application::Neuralyzer::Tools::call(Mcp::Dispatcher& dispatcher, std::vector<common_chat_tool_call> const& toolCalls)
{
    std::string message = "Tool execution results:\n\n";
    auto result = juce::Result::ok();
    for(auto const& toolCall : toolCalls)
    {
        MiscDebug("Application::Neuralyzer::Tools", juce::String("Calling tool: ") + toolCall.name + " (ID: " + toolCall.id + ")");

        nlohmann::json request;
        request["method"] = "tools/call";
        request["params"]["name"] = toolCall.name;
        request["params"]["arguments"] = toolCall.arguments.empty() ? nlohmann::json::object() : nlohmann::json::parse(toolCall.arguments);

        message += "[Tool Call ID: " + toolCall.id + "]\n";
        message += "Tool: " + toolCall.name + "\n";

        try
        {
            auto const dispatcherResponse = dispatcher.handle(request);
            if(dispatcherResponse.contains("error"))
            {
                result = juce::Result::fail("One or more tool calls failed");
                message += "Status: ERROR\n";
                if(dispatcherResponse.at("error").is_object())
                {
                    auto const& error = dispatcherResponse.at("error");
                    if(error.contains("message") && error.at("message").is_string())
                    {
                        message += "Message: " + error.at("message").get<std::string>() + "\n";
                    }
                    if(error.contains("code") && error.at("code").is_number())
                    {
                        message += "Code: " + std::to_string(error.at("code").get<int>()) + "\n";
                    }
                }
                else
                {
                    message += "Message: " + dispatcherResponse.at("error").dump() + "\n";
                }
            }
            else
            {
                auto const isError = dispatcherResponse.contains("isError") && dispatcherResponse.at("isError").get<bool>();
                if(isError)
                {
                    result = juce::Result::fail("One or more tool calls failed");
                    message += "Status: FAILED\n";
                    if(dispatcherResponse.contains("content") && dispatcherResponse.at("content").is_array() && !dispatcherResponse.at("content").empty() && dispatcherResponse.at("content").at(0).contains("text"))
                    {
                        message += "Message: " + dispatcherResponse.at("content").at(0)["text"].get<std::string>() + "\n";
                    }
                }
                else
                {
                    message += "Status: SUCCESS\n";
                    if(dispatcherResponse.contains("content") && dispatcherResponse.at("content").is_array() && !dispatcherResponse.at("content").empty() && dispatcherResponse.at("content").at(0).contains("text"))
                    {
                        message += "Result: " + dispatcherResponse["content"][0]["text"].get<std::string>() + "\n";
                    }
                    else
                    {
                        message += "Result: (empty)\n";
                    }
                }
            }
        }
        catch(std::exception const& e)
        {
            result = juce::Result::fail("One or more tool calls failed");
            message += "Status: ERROR\n";
            message += "Message: Exception during execution: " + std::string(e.what()) + "\n";
        }
        catch(...)
        {
            result = juce::Result::fail("One or more tool calls failed");
            message += "Status: ERROR\n";
            message += "Message: Exception during execution: Unknown\n";
        }
        message += "\n";
    }
    return std::make_pair(std::move(result), std::move(message));
}

ANALYSE_FILE_END
