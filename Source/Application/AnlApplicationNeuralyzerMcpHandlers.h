#pragma once

#include "../Misc/AnlMisc.h"
#include "../Plugin/AnlPluginModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Instance;

    namespace Neuralyzer
    {
        namespace Mcp
        {
            //! @brief The callbacks of the tools that depend on the client of the dispatcher (the tools are not available if the callbacks are null)
            struct DynamicToolMethods
            {
                using read_files_fn_t = std::function<nlohmann::json(std::vector<std::string> const& filePaths)>;
                using search_docs_fn_t = std::function<nlohmann::json(std::string const& query, size_t maxNumResources)>;
                using load_docs_fn_t = std::function<nlohmann::json(std::vector<std::string> const& ids)>;

                read_files_fn_t readFilesFn{nullptr};
                search_docs_fn_t searchDocsFn{nullptr};
                load_docs_fn_t loadDocsFn{nullptr};
            };

            //! @brief Creates the response of a tool that failed (tool execution error)
            //! @param message The error message
            //! @return The response of the tool
            nlohmann::json createToolErrorResponse(std::string const& message);

            // Handlers of the MCP tools: each handler parses the JSON arguments of a tool, calls the
            // corresponding operations (on the message thread if they access the document) and
            // formats the JSON response. The handlers throw an ExecutionError if the arguments are invalid.
            class Handlers
            {
            public:
                explicit Handlers(Instance& instance);
                ~Handlers() = default;

                //! @brief Aborts the pending and the future calls on the message thread
                //! @details This must be called before the message thread waits for the threads that use the handlers
                void abort();

                //! @brief Gets the installed plugins (can be called on any thread)
                //! @return The descriptions of the installed plugins by key
                std::map<Plugin::Key, Plugin::Description> getInstalledPlugins();

                // Document

                //! @brief Handles the tool get_document_state
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getDocumentState(nlohmann::json const& arguments);

                //! @brief Handles the tool get_document_audio_file_layout
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getDocumentAudioFileLayout(nlohmann::json const& arguments);

                //! @brief Handles the tool set_document_audio_file_layout
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json setDocumentAudioFileLayout(nlohmann::json const& arguments);

                //! @brief Handles the tool get_audio_file_info
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getAudioFileInfo(nlohmann::json const& arguments);

                //! @brief Handles the tool get_document_time_zoom
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getDocumentTimeZoom(nlohmann::json const& arguments);

                //! @brief Handles the tool set_document_time_zoom
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json setDocumentTimeZoom(nlohmann::json const& arguments);

                //! @brief Handles the tool get_time_selection
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getTimeSelection(nlohmann::json const& arguments);

                //! @brief Handles the tool get_selected_items
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getSelectedItems(nlohmann::json const& arguments);

                // Groups

                //! @brief Handles the tool get_group_names
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getGroupNames(nlohmann::json const& arguments);

                //! @brief Handles the tool set_group_names
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json setGroupNames(nlohmann::json const& arguments);

                //! @brief Handles the tool get_group_layouts
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getGroupLayouts(nlohmann::json const& arguments);

                //! @brief Handles the tool set_group_layouts
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json setGroupLayouts(nlohmann::json const& arguments);

                //! @brief Handles the tool get_group_vertical_zoom
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getGroupVerticalZoom(nlohmann::json const& arguments);

                //! @brief Handles the tool set_group_vertical_zoom
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json setGroupVerticalZoom(nlohmann::json const& arguments);

                //! @brief Handles the tool create_groups
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json createGroups(nlohmann::json const& arguments);

                //! @brief Handles the tool remove_groups
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json removeGroups(nlohmann::json const& arguments);

                // Tracks

                //! @brief Handles the tool create_tracks_with_plugins
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json createTracksWithPlugins(nlohmann::json const& arguments);

                //! @brief Handles the tool create_tracks_with_files
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json createTracksWithFiles(nlohmann::json const& arguments);

                //! @brief Handles the tool remove_tracks
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json removeTracks(nlohmann::json const& arguments);

                //! @brief Handles the tool get_track_graphics
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getTrackGraphics(nlohmann::json const& arguments);

                //! @brief Handles the tool set_track_graphics
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json setTrackGraphics(nlohmann::json const& arguments);

                //! @brief Handles the tool set_track_parameters
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json setTrackParameters(nlohmann::json const& arguments);

                //! @brief Handles the tool set_track_inputs
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json setTrackInputs(nlohmann::json const& arguments);

                //! @brief Handles the tool set_track_extra_thresholds
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json setTrackExtraThresholds(nlohmann::json const& arguments);

                //! @brief Handles the tool get_track_types
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getTrackTypes(nlohmann::json const& arguments);

                //! @brief Handles the tool get_track_descriptions
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getTrackDescriptions(nlohmann::json const& arguments);

                //! @brief Handles the tool get_track_parameters
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getTrackParameters(nlohmann::json const& arguments);

                //! @brief Handles the tool get_track_vertical_zoom
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getTrackVerticalZoom(nlohmann::json const& arguments);

                //! @brief Handles the tool set_track_vertical_zoom
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json setTrackVerticalZoom(nlohmann::json const& arguments);

                //! @brief Handles the tool get_track_value_range
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getTrackValueRange(nlohmann::json const& arguments);

                //! @brief Handles the tool set_track_value_range
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json setTrackValueRange(nlohmann::json const& arguments);

                //! @brief Handles the tool get_track_inputs
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getTrackInputs(nlohmann::json const& arguments);

                //! @brief Handles the tool get_track_compatible_inputs
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getTrackCompatibleInputs(nlohmann::json const& arguments);

                //! @brief Handles the tool get_track_is_visible_in_group
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getTrackIsVisibleInGroup(nlohmann::json const& arguments);

                //! @brief Handles the tool set_track_is_visible_in_group
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json setTrackIsVisibleInGroup(nlohmann::json const& arguments);

                //! @brief Handles the tool get_track_extra_thresholds
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getTrackExtraThresholds(nlohmann::json const& arguments);

                //! @brief Handles the tool get_track_names
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getTrackNames(nlohmann::json const& arguments);

                //! @brief Handles the tool set_track_names
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json setTrackNames(nlohmann::json const& arguments);

                // Results

                //! @brief Handles the tool get_track_results_raw
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getTrackResultsRaw(nlohmann::json const& arguments);

                //! @brief Handles the tool get_track_results_summary
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getTrackResultsSummary(nlohmann::json const& arguments);

                //! @brief Handles the tool set_track_results_raw
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json setTrackResultsRaw(nlohmann::json const& arguments);

                //! @brief Handles the tool save_track_results
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json saveTrackResults(nlohmann::json const& arguments);

                // Plugins

                //! @brief Handles the tool get_vamp_plugin_descriptions
                //! @param arguments The arguments of the tool
                //! @return The response of the tool
                nlohmann::json getVampPluginDescriptions(nlohmann::json const& arguments);

                // Dynamic tools

                //! @brief Handles the tool search_docs
                //! @param arguments The arguments of the tool
                //! @param searchDocsFn The callback that searches the documents
                //! @return The response of the tool
                nlohmann::json searchDocs(nlohmann::json const& arguments, DynamicToolMethods::search_docs_fn_t const& searchDocsFn);

                //! @brief Handles the tool load_docs
                //! @param arguments The arguments of the tool
                //! @param loadDocsFn The callback that loads the documents
                //! @return The response of the tool
                nlohmann::json loadDocs(nlohmann::json const& arguments, DynamicToolMethods::load_docs_fn_t const& loadDocsFn);

                //! @brief Handles the tool read_files
                //! @param arguments The arguments of the tool
                //! @param readFilesFn The callback that reads the files
                //! @return The response of the tool
                nlohmann::json readFiles(nlohmann::json const& arguments, DynamicToolMethods::read_files_fn_t const& readFilesFn);

            private:
                Instance& mInstance;
                std::atomic<bool> mShouldAbort{false};
            };
        } // namespace Mcp
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
