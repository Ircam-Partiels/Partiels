#pragma once

#include "AnlApplicationNeuralyzerMcpHandlers.h"
#include "AnlApplicationNeuralyzerModel.h"
JUCE_BEGIN_IGNORE_WARNINGS_MSVC(4100 4245 4267 4305 4457 4505)
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wshadow-field-in-constructor", "-Wimplicit-float-conversion", "-Wunused-function", "-Wzero-as-null-pointer-constant", "-Wfloat-equal", "-Wsign-conversion", "-Wdeprecated-copy-with-dtor", "-Wmissing-noreturn", "-Winconsistent-missing-destructor-override")
#include <chat.h>
#include <llama-cpp.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
JUCE_END_IGNORE_WARNINGS_MSVC

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        namespace Mcp
        {
            static auto constexpr jsonrpc = "2.0";

            //! @brief The error codes of the JSON-RPC protocol
            namespace ErrorCode
            {
                static auto constexpr parseError = -32700;
                static auto constexpr invalidRequest = -32600;
                static auto constexpr methodNotFound = -32601;
                static auto constexpr invalidParams = -32602;
                static auto constexpr internalError = -32603;
                static auto constexpr resourceNotFound = -32002;
            } // namespace ErrorCode

            //! @brief An error of the protocol (invalid request, unknown method, unknown tool...)
            class ProtocolError
            : public std::runtime_error
            {
            public:
                ProtocolError(int code, std::string const& message);
                ~ProtocolError() override = default;

                //! @brief Gets the JSON-RPC error code
                int getCode() const noexcept;

            private:
                int mCode;
            };

            //! @brief Creates a JSON-RPC response with a result
            //! @param id The identifier of the request
            //! @param result The result
            //! @return The response
            nlohmann::json createResultResponse(nlohmann::json const& id, nlohmann::json const& result);

            //! @brief Creates a JSON-RPC response with an error
            //! @param id The identifier of the request (null if unknown)
            //! @param code The error code
            //! @param message The error message
            //! @return The response
            nlohmann::json createErrorResponse(nlohmann::json const& id, int code, std::string const& message);

            class ExecutionError
            : public std::runtime_error
            {
                using std::runtime_error::runtime_error;
            };

            class Connection;

            class Dispatcher
            {
            public:
                explicit Dispatcher(Instance& instance);
                ~Dispatcher() = default;

                using DynamicToolMethods = Mcp::DynamicToolMethods;

                std::string getUuid();
                std::string getInstructions();
                std::vector<std::pair<std::string, std::string>> getResources();
                std::vector<common_chat_tool> getToolList(DynamicToolMethods const& methods);
                common_chat_msg callTool(DynamicToolMethods const& methods, common_chat_tool_call const& toolCalls);

                //! @brief Aborts the pending and the future calls of the tools on the message thread
                //! @details This must be called before the message thread waits for the threads that use the dispatcher (when quitting)
                void abort();

                //! @brief Gets the names of the available tools
                //! @param methods The callbacks of the dynamic tools
                //! @return The names of the registered tools and of the dynamic tools with a callback
                std::set<std::string> getToolNames(DynamicToolMethods const& methods);

            private:
                // Returns the result of the method or throws a ProtocolError
                nlohmann::json callMethod(nlohmann::json const& params, DynamicToolMethods const& methods = DynamicToolMethods());
                friend class Connection;
                friend class DispatcherUnitTest;

                using tool_fn_t = std::function<nlohmann::json(nlohmann::json const& arguments)>;
                std::map<std::string, tool_fn_t> getDynamicTools(DynamicToolMethods const& methods);

                Handlers mHandlers;
                std::map<std::string, tool_fn_t> const mTools;
            };

        } // namespace Mcp
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
