#pragma once

#include "../Document/AnlDocumentDirector.h"
#include "../Document/AnlDocumentExporter.h"
#include "../Plugin/AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        namespace Mcp
        {
            // Operations performed by the MCP tools, independent of the JSON protocol and of the application instance
            // The operations that access the document must be called on the message thread
            // The operations on several items return a result for each item
            // The groups of the document and the tracks of a group are ordered from the top (index 0) to the bottom (the
            // top track is drawn in front of the other tracks)
            namespace Operations
            {
                //! @brief Starts a document action that is aborted on destruction unless it has been committed
                class ScopedDocumentAction
                {
                public:
                    explicit ScopedDocumentAction(Document::Director& documentDir);
                    ~ScopedDocumentAction();

                    //! @brief Ends the action with a new undoable transaction
                    //! @param actionName The name of the transaction
                    void commit(juce::String const& actionName);

                    //! @brief Ends the action and reverts its changes
                    void abort();

                private:
                    Document::Director& mDocumentDir;
                };

                //! @brief The identifier and the name of a track
                struct TrackState
                {
                    juce::String identifier;
                    juce::String name;
                };

                //! @brief The identifier and the name of a new track, and the other linked tracks of its multitrack output
                struct NewTrackState
                {
                    juce::String identifier;
                    juce::String name;
                    std::vector<TrackState> linkedTracks; //!< The other linked tracks of the multitrack output (empty if the output is not a multitrack output)
                };

                //! @brief The linked tracks of a multitrack output (the tracks of the voices of the same analysis)
                struct LinkedTracks
                {
                    size_t count{0_z};              //!< The number of tracks of the multitrack output
                    size_t minCount{1_z};           //!< The minimum number of tracks
                    size_t maxCount{1_z};           //!< The maximum number of tracks
                    size_t defaultCount{1_z};       //!< The default number of tracks of the plugin
                    std::vector<TrackState> tracks; //!< The linked tracks ordered from the first one (the removed tracks are missing)
                };

                //! @brief The identifier, the name and the tracks of a group
                struct GroupState
                {
                    juce::String identifier;
                    juce::String name;
                    std::vector<TrackState> tracks;
                };

                //! @brief The visible and global ranges of a zoom
                struct ZoomRanges
                {
                    juce::Range<double> visibleRange;
                    juce::Range<double> globalRange;
                };

                //! @brief The properties of a track to modify (the properties without value are not modified)
                struct TrackPropertiesModification
                {
                    std::optional<juce::String> name;
                    std::optional<bool> visibleInGroup;
                    std::optional<std::vector<std::optional<float>>> extraThresholds; //!< The threshold of each extra output (if any)
                    std::optional<juce::Range<double>> verticalZoom;                  //!< The visible range of the vertical zoom (value and vector tracks)
                    std::optional<juce::Range<double>> valueRange;                    //!< The global range of the value zoom used by the color map (vector tracks)
                };

                //! @brief The effective properties of a modified track (only the modified properties have a value)
                struct TrackPropertiesState
                {
                    std::optional<juce::String> name;
                    std::optional<bool> visibleInGroup;
                    std::optional<std::vector<std::optional<float>>> extraThresholds;
                    std::optional<ZoomRanges> verticalZoom;
                    std::optional<ZoomRanges> valueRange;
                };

                //! @brief The properties of a group to modify (the properties without value are not modified)
                struct GroupPropertiesModification
                {
                    std::optional<juce::String> name;
                    std::optional<std::vector<juce::String>> layout; //!< The identifiers of the tracks from the top to the bottom
                    std::optional<juce::Range<double>> verticalZoom; //!< The visible range of the vertical zoom
                };

                //! @brief The effective properties of a modified group (only the modified properties have a value)
                struct GroupPropertiesState
                {
                    std::optional<juce::String> name;
                    std::optional<std::vector<juce::String>> layout;
                    std::optional<ZoomRanges> verticalZoom;
                };

                //! @brief The selected groups and tracks
                struct SelectedItems
                {
                    std::set<juce::String> groups;
                    std::set<juce::String> tracks;
                };

                //! @brief The placement of a track in a group or of a group in the document
                //! @details The items placed with the same placement (the same group, the same placement and the same
                //! anchor) within a single action are placed in the order of their requests from the top to the bottom:
                //! the first item is placed as requested and each following item is placed just below the previous one
                enum class Placement
                {
                    top,    //!< At the top of the group or of the document
                    bottom, //!< At the bottom of the group or of the document
                    above,  //!< Just above the anchor
                    below   //!< Just below the anchor
                };

                //! @brief The name and the placement of a new group
                struct GroupCreation
                {
                    juce::String name;
                    Placement placement{Placement::bottom}; //!< The placement in the document
                    juce::String anchorIdentifier;          //!< The anchor group of the above and below placements
                };

                //! @brief The graphics properties of a track
                //! @details When getting the properties, only the properties supported by the type of the track are defined.
                //! When setting the properties, only the defined properties are modified.
                struct TrackGraphics
                {
                    std::optional<Track::ColourMap> colourMap;                                 //!< Vector tracks
                    std::optional<juce::Colour> backgroundColour;                              //!< Label and value tracks
                    std::optional<juce::Colour> foregroundColour;                              //!< Label and value tracks
                    std::optional<juce::Colour> durationColour;                                //!< Label tracks
                    std::optional<juce::Colour> textColour;                                    //!< Label and value tracks
                    std::optional<juce::Colour> shadowColour;                                  //!< Label and value tracks
                    std::optional<juce::String> fontName;                                      //!< Label and value tracks
                    std::optional<juce::String> fontStyle;                                     //!< Label and value tracks
                    std::optional<float> fontSize;                                             //!< Label and value tracks
                    std::optional<float> lineWidth;                                            //!< Label and value tracks
                    std::optional<juce::String> unit;                                          //!< Value and vector tracks (empty for no unit)
                    std::optional<float> labelLayoutPosition;                                  //!< Label tracks
                    std::optional<Track::LabelLayout::Justification> labelLayoutJustification; //!< Label tracks
                    std::optional<Track::MarkerStyle> markerStyle;                             //!< Label tracks
                };

                //! @brief A gradient of colors sampled from a color map and distributed over tracks
                struct TrackColourGradient
                {
                    Track::ColourMap colourMap{Track::ColourMap::Viridis};                //!< The sampled color map
                    std::vector<std::optional<juce::Colour> TrackGraphics::*> properties; //!< The color properties modified by the gradient
                };

                //! @brief The plugin, the group, the name and the parameter values of a new track
                //! @details The new track is inserted at the top of the group, except a vector track that is inserted at the bottom of the group
                struct PluginTrackCreation
                {
                    Plugin::Key key;                         //!< The key of the plugin
                    juce::String groupIdentifier;            //!< The identifier of the group (the top group if empty)
                    std::optional<juce::String> name;        //!< The name of the track (the name of the plugin if not defined)
                    std::map<std::string, float> parameters; //!< The parameter values by parameter identifier
                    std::optional<size_t> linkedTrackCount;  //!< The number of linked tracks if the output is a multitrack output (the default number of the plugin if not defined)
                };

                //! @brief The file and the group of a new track
                //! @details The new track is inserted at the top of the group
                struct FileTrackCreation
                {
                    juce::File file;              //!< The file of the results
                    juce::String groupIdentifier; //!< The identifier of the group (the top group if empty)
                };

                //! @brief The track, the group and the placement of a track move
                struct TrackMove
                {
                    juce::String identifier;             //!< The identifier of the moved track
                    juce::String groupIdentifier;        //!< The identifier of the destination group (the group of the anchor track with the above and below placements, the group of the track if empty)
                    Placement placement{Placement::top}; //!< The placement in the destination group
                    juce::String anchorIdentifier;       //!< The anchor track of the above and below placements
                };

                //! @brief The tracks that can be connected to an input of a track
                struct CompatibleInput
                {
                    juce::String inputIdentifier;                    //!< The identifier of the input
                    std::vector<juce::String> compatibleIdentifiers; //!< The identifiers of the compatible tracks
                };

                //! @brief The selection of the results of tracks
                struct ResultsSelection
                {
                    std::vector<juce::Range<double>> timeRanges; //!< The time ranges in seconds (all the results if empty)
                    std::set<size_t> channels;                   //!< The channel indices (all the channels if empty)
                    bool applyExtraThresholds{true};             //!< Whether the results are filtered with the extra thresholds of the tracks
                };

                //! @brief The statistics of a set of values (undefined if the set is empty)
                struct Statistics
                {
                    std::optional<float> min;
                    std::optional<float> max;
                    std::optional<double> median;
                    std::optional<double> mean;
                    std::optional<double> variance;
                    std::optional<double> skewness;
                };

                //! @brief The statistics of the values of an extra output
                struct ExtraValuesSummary
                {
                    size_t index{0_z};       //!< The index of the extra output
                    juce::String identifier; //!< The identifier of the extra output
                    juce::String name;       //!< The name of the extra output
                    juce::String unit;       //!< The unit of the extra output
                    Statistics statistics;   //!< The statistics of the extra values
                };

                //! @brief The summary of the results of a channel in a time range
                struct ResultsRangeSummary
                {
                    std::optional<juce::Range<double>> timeRange; //!< The time range (all the results if undefined)
                    size_t count{0_z};                            //!< The number of results (including the breaks)
                    std::optional<size_t> breakCount;             //!< The number of points without value that break the lines (value tracks only)
                    std::optional<Statistics> values;             //!< The statistics of the values (value and vector tracks only)
                    std::vector<ExtraValuesSummary> extraValues;  //!< The statistics of the extra values
                };

                //! @brief The summary of the results of a channel
                struct ResultsChannelSummary
                {
                    size_t channel{0_z};                     //!< The index of the channel
                    std::vector<ResultsRangeSummary> ranges; //!< The summary of each time range
                };

                //! @brief The summary of the results of a track
                struct ResultsSummary
                {
                    Track::FrameType type{Track::FrameType::label}; //!< The type of the track
                    std::vector<ResultsChannelSummary> channels;    //!< The summary of each selected channel
                };

                //! @brief A copy of the results of a track with the information required to read them
                //! @details The results share their data with the track, the snapshot can be used on any thread
                struct ResultsSnapshot
                {
                    Track::FrameType type{Track::FrameType::label};    //!< The type of the track
                    Track::Results results;                            //!< The results of the track
                    std::vector<std::optional<float>> extraThresholds; //!< The extra thresholds of the track
                    std::vector<Plugin::OutputExtra> extraOutputs;     //!< The extra outputs of the track
                };

                //! @brief The new results of a track
                struct ResultsModification
                {
                    std::string results;                          //!< The results in the CSV format
                    std::optional<juce::Range<double>> timeRange; //!< The time range to replace (all the results if undefined)
                };

                //! @brief The information of an audio file
                struct AudioFileInfo
                {
                    juce::String formatName;
                    double sampleRate{0.0};
                    unsigned int bitsPerSample{0u};
                    bool usesFloatingPointData{false};
                    juce::int64 lengthInSamples{0};
                    unsigned int numChannels{0u};
                    juce::StringPairArray metadata;
                };

                // Document

                //! @brief Gets the groups of the document with their tracks
                //! @param documentAcsr The document accessor
                //! @return The groups and their tracks from the top to the bottom
                std::vector<GroupState> getDocumentState(Document::Accessor const& documentAcsr);

                //! @brief Gets the audio file layout of the document
                //! @param documentAcsr The document accessor
                //! @return The audio files and their channel assignments
                std::vector<AudioFileLayout> getDocumentAudioFileLayout(Document::Accessor const& documentAcsr);

                //! @brief Sets the audio file layout of the document
                //! @param documentAcsr The document accessor
                //! @param layouts The audio files and their channel assignments
                void setDocumentAudioFileLayout(Document::Accessor& documentAcsr, std::vector<AudioFileLayout> const& layouts);

                //! @brief Gets the information of audio files
                //! @param formatManager The format manager used to read the files
                //! @param files The audio files
                //! @return For each file, the result of the operation and the information of the audio file
                std::map<juce::File, std::tuple<juce::Result, AudioFileInfo>> getAudioFileInfo(juce::AudioFormatManager& formatManager, std::vector<juce::File> const& files);

                //! @brief Gets the visible and global ranges of the time zoom of the document
                //! @param documentAcsr The document accessor
                //! @return The ranges of the time zoom
                ZoomRanges getDocumentTimeZoom(Document::Accessor const& documentAcsr);

                //! @brief Sets the visible range of the time zoom of the document
                //! @param documentAcsr The document accessor
                //! @param visibleRange The new visible range
                //! @return The result of the operation and the effective ranges of the time zoom (the visible range can be clipped to the global range)
                std::tuple<juce::Result, ZoomRanges> setDocumentTimeZoom(Document::Accessor& documentAcsr, juce::Range<double> const& visibleRange);

                //! @brief Gets the time selection of the document
                //! @param documentAcsr The document accessor
                //! @return The time selection
                juce::Range<double> getTimeSelection(Document::Accessor const& documentAcsr);

                //! @brief Gets the selected groups and tracks of the document
                //! @param documentAcsr The document accessor
                //! @return The identifiers of the selected groups and tracks
                SelectedItems getSelectedItems(Document::Accessor const& documentAcsr);

                // Groups

                //! @brief Gets the names of groups
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the groups
                //! @return For each group identifier, the result of the operation and the name of the group
                std::map<juce::String, std::tuple<juce::Result, juce::String>> getGroupNames(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers);

                //! @brief Gets the layouts of groups
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the groups
                //! @return For each group identifier, the result of the operation and the identifiers of the tracks from the top to the bottom
                std::map<juce::String, std::tuple<juce::Result, std::vector<juce::String>>> getGroupLayouts(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers);

                //! @brief Gets the vertical zooms of groups
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the groups
                //! @return For each group identifier, the result of the operation and the ranges of the vertical zoom
                std::map<juce::String, std::tuple<juce::Result, ZoomRanges>> getGroupVerticalZoom(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers);

                //! @brief Modifies the properties of groups
                //! @details The names and the layouts are modified within a single undoable action (aborted if no group has
                //! been modified), then the vertical zooms are modified (the vertical zoom of a group is the vertical zoom of
                //! its top-most visible value or vector track). A layout must contain exactly the tracks of the group. A group
                //! is not modified if one of its properties is invalid.
                //! @param documentDir The document director
                //! @param modifications The identifiers of the groups and their properties to modify
                //! @return For each group identifier, the result of the operation and the effective values of the modified properties
                std::map<juce::String, std::tuple<juce::Result, GroupPropertiesState>> setGroupProperties(Document::Director& documentDir, std::map<juce::String, GroupPropertiesModification> const& modifications);

                //! @brief Creates groups within a single undoable action
                //! @details The groups are created in order (see Placement). The action is aborted if no group has been created
                //! @param documentDir The document director
                //! @param groups The names and the placements of the new groups
                //! @return For each new group in the same order, the result of the operation and the state of the group
                std::vector<std::tuple<juce::Result, GroupState>> createGroups(Document::Director& documentDir, std::vector<GroupCreation> const& groups);

                //! @brief Removes groups and their tracks within a single undoable action
                //! @details The groups that don't exist are ignored, the action is aborted if no group has been removed
                //! @param documentDir The document director
                //! @param identifiers The identifiers of the groups
                //! @return For each group identifier, the result of the operation
                std::map<juce::String, juce::Result> removeGroups(Document::Director& documentDir, std::vector<juce::String> const& identifiers);

                // Tracks

                //! @brief Gets the names of tracks
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the tracks
                //! @return For each track identifier, the result of the operation and the name of the track
                std::map<juce::String, std::tuple<juce::Result, juce::String>> getTrackNames(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers);

                //! @brief Modifies the properties of tracks
                //! @details The names, the visibilities in the groups and the extra thresholds are modified within a single
                //! undoable action (aborted if no track has been modified), then the vertical zooms (the tracks with a linked
                //! zoom propagate the change to the other linked tracks of their group) and the value ranges are modified.
                //! A track is not modified if one of its properties is invalid or not supported by its type.
                //! @param documentDir The document director
                //! @param modifications The identifiers of the tracks and their properties to modify
                //! @return For each track identifier, the result of the operation and the effective values of the modified properties
                std::map<juce::String, std::tuple<juce::Result, TrackPropertiesState>> setTrackProperties(Document::Director& documentDir, std::map<juce::String, TrackPropertiesModification> const& modifications);

                //! @brief Gets the graphics properties of tracks
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the tracks
                //! @return For each track identifier, the result of the operation and the graphics properties supported by the track
                std::map<juce::String, std::tuple<juce::Result, TrackGraphics>> getTrackGraphics(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers);

                //! @brief Sets the graphics properties of tracks within a single undoable action
                //! @details The default properties are applied to all the tracks and ignored if a track doesn't support them,
                //! the properties of a track override the default properties and a track is not modified if it doesn't support
                //! one of its properties. The action is aborted if no track has been modified.
                //! The colors of the gradient are distributed over the tracks in their order: they replace the default
                //! properties, are ignored if a track doesn't support them, are overridden by the properties of a track and
                //! keep the alpha of the current colors of the track.
                //! @param documentDir The document director
                //! @param defaultGraphics The default graphics properties to modify
                //! @param graphics The identifiers of the tracks and their graphics properties to modify (ordered for the gradient)
                //! @param gradient The optional gradient of colors distributed over the tracks
                //! @return For each track identifier, the result of the operation
                std::map<juce::String, juce::Result> setTrackGraphics(Document::Director& documentDir, TrackGraphics const& defaultGraphics, std::vector<std::pair<juce::String, TrackGraphics>> const& graphics, std::optional<TrackColourGradient> const& gradient = {});

                //! @brief Gets the colors of a color map discretized by a number of items
                //! @details The color of an item is sampled at the center of its interval in the color map (so the colors
                //! never reach the ends of the color map that are often too dark or too light)
                //! @param colourMap The color map
                //! @param count The number of items
                //! @return The opaque colors of the items
                std::vector<juce::Colour> getColourMapColours(Track::ColourMap colourMap, size_t count);

                //! @brief Sets the plugin parameter values of tracks within a single undoable action
                //! @details The values are clamped to the ranges of the parameters. The default values are applied to all
                //! the tracks and ignored if a track doesn't have the parameter, the values of a track override the default
                //! values and a track is not modified if it doesn't have one of the parameters. The alias parameters are
                //! applied after the other parameters and modify the parameters they represent. The action is aborted if no
                //! track has been modified
                //! @param documentDir The document director
                //! @param defaultValues The default values by parameter identifier
                //! @param values The identifiers of the tracks and their values by parameter identifier
                //! @return For each track identifier, the result of the operation and the effective values of the requested
                //! parameters and of the other parameters modified (by the alias parameters)
                std::map<juce::String, std::tuple<juce::Result, std::map<std::string, float>>> setTrackParameters(Document::Director& documentDir, std::map<std::string, float> const& defaultValues, std::map<juce::String, std::map<std::string, float>> const& values);

                //! @brief Connects the inputs of tracks to other tracks within a single undoable action
                //! @details A track is not modified if one of its inputs or one of the input tracks is invalid.
                //! The action is aborted if no track has been modified
                //! @param documentDir The document director
                //! @param inputs The identifiers of the tracks and the identifiers of the input tracks by input identifier (an empty identifier disconnects the input)
                //! @return For each track identifier, the result of the operation
                std::map<juce::String, juce::Result> setTrackInputs(Document::Director& documentDir, std::map<juce::String, std::map<juce::String, juce::String>> const& inputs);

                //! @brief Creates tracks with plugins within a single undoable action
                //! @details The tracks are created in order, each one at the top of its group (or at the bottom for a vector track,
                //! see PluginTrackCreation). The default parameter values are applied to all the tracks and
                //! ignored if a track doesn't have the parameter, the parameter values of a track override the default values
                //! and a track is not created if it doesn't have one of its parameters. The action is aborted if no track has been created
                //! @param documentDir The document director
                //! @param presetListAcsr The presets applied to the new tracks according to their plugins
                //! @param globalGraphicPreset The graphic preset applied to the new tracks without graphic preset
                //! @param defaultParameters The default parameter values by parameter identifier
                //! @param tracks The plugins, the groups, the names, the parameter values and the numbers of linked tracks of the new tracks
                //! If the output of a plugin is a multitrack output, the linked tracks are created (from the bottom to the top of
                //! the block of linked tracks), the parameters are shared by the linked tracks and the name is suffixed with the number of the track
                //! @return For each new track in the same order, the result of the operation and the state of the track
                std::vector<std::tuple<juce::Result, NewTrackState>> createTracksWithPlugins(Document::Director& documentDir, Track::PresetList::Accessor const& presetListAcsr, Track::GraphicsSettings const& globalGraphicPreset, std::map<std::string, float> const& defaultParameters, std::vector<PluginTrackCreation> const& tracks);

                //! @brief Creates tracks with result files within a single undoable action
                //! @details The tracks are created in order, each one at the top of its group. The action is aborted if no track has been created
                //! @param documentDir The document director
                //! @param globalGraphicPreset The graphic preset applied to the new tracks
                //! @param tracks The files and the groups of the new tracks
                //! @return For each new track in the same order, the result of the operation and the state of the track
                std::vector<std::tuple<juce::Result, NewTrackState>> createTracksWithFiles(Document::Director& documentDir, Track::GraphicsSettings const& globalGraphicPreset, std::vector<FileTrackCreation> const& tracks);

                //! @brief Removes tracks within a single undoable action
                //! @details The tracks that don't exist are ignored, the action is aborted if no track has been removed
                //! @param documentDir The document director
                //! @param identifiers The identifiers of the tracks
                //! @return For each track identifier, the result of the operation
                std::map<juce::String, juce::Result> removeTracks(Document::Director& documentDir, std::vector<juce::String> const& identifiers);

                //! @brief Moves tracks within a single undoable action
                //! @details The tracks are moved in order (only the requested tracks are moved, not their linked tracks).
                //! The invalid moves are ignored, the action is aborted if no track has been moved
                //! @param documentDir The document director
                //! @param moves The tracks, the destination groups and the placements of the moves
                //! @return For each track identifier, the result of the operation
                std::map<juce::String, juce::Result> moveTracks(Document::Director& documentDir, std::vector<TrackMove> const& moves);

                // Plugins

                //! @brief Gets the descriptions of installed plugins
                //! @param scanner The scanner of the installed plugins
                //! @param keys The keys of the plugins
                //! @return For each plugin key in the same order, the result of the operation and the description of the plugin
                std::vector<std::tuple<juce::Result, Plugin::Description>> getVampPluginDescriptions(PluginList::Scanner& scanner, std::vector<Plugin::Key> const& keys);

                // Results

                //! @brief Checks if the results of existing tracks are not yet available (the analyses might be running)
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the tracks
                //! @return True if the results of at least one existing track are not available
                bool hasTrackResultsPending(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers);

                //! @brief Gets snapshots of the available results of tracks
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the tracks
                //! @return For each track identifier, the result of the operation and the snapshot of the results
                std::map<juce::String, std::tuple<juce::Result, ResultsSnapshot>> getTrackResultsSnapshots(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers);

                //! @brief Counts the selected results of a snapshot (can be called on any thread)
                //! @param snapshot The snapshot of the results
                //! @param selection The selection of the results
                //! @return The result of the operation and the number of selected results
                std::tuple<juce::Result, size_t> countResults(ResultsSnapshot const& snapshot, ResultsSelection const& selection);

                //! @brief Summarizes the selected results of a snapshot (can be called on any thread)
                //! @param snapshot The snapshot of the results
                //! @param selection The selection of the results
                //! @return The result of the operation and the summary of the results
                std::tuple<juce::Result, ResultsSummary> summarizeResults(ResultsSnapshot const& snapshot, ResultsSelection const& selection);

                //! @brief Gets the selected results of tracks in the CSV format
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the tracks
                //! @param selection The selection of the results
                //! @param headerType The type of the CSV header
                //! @param separator The column separator
                //! @return For each track identifier, the result of the operation and the results in the CSV format
                std::map<juce::String, std::tuple<juce::Result, std::string>> getTrackResultsRaw(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers, ResultsSelection const& selection, Track::Exporter::CsvHeaderType headerType, char separator);

                //! @brief Replaces the results of tracks within a single undoable transaction
                //! @details As with the edition of the results in the UI, the results of the time range (that excludes its end) are erased
                //! and the new results are inserted at their own times (that must be within the time range). The tracks that don't exist,
                //! whose analysis is running or with invalid results are ignored.
                //! @param documentDir The document director
                //! @param modifications The identifiers of the tracks and their new results
                //! @param separator The column separator of the CSV results
                //! @return For each track identifier, the result of the operation
                std::map<juce::String, juce::Result> setTrackResultsRaw(Document::Director& documentDir, std::map<juce::String, ResultsModification> const& modifications, char separator);

                //! @brief Erases the results of tracks in time ranges within a single undoable transaction
                //! @details As with the deletion of the frames in the UI, the results are erased in each time range that excludes its end
                //! (an empty time range erases the result at its start). The tracks that don't exist, whose analysis is running or with
                //! invalid channels are ignored.
                //! @param documentDir The document director
                //! @param identifiers The identifiers of the tracks
                //! @param timeRanges The time ranges in seconds
                //! @param channels The channel indices (all the channels if empty)
                //! @return For each track identifier, the result of the operation and the number of erased results
                std::map<juce::String, std::tuple<juce::Result, size_t>> eraseTrackResults(Document::Director& documentDir, std::vector<juce::String> const& identifiers, std::vector<juce::Range<double>> const& timeRanges, std::set<size_t> const& channels);

                //! @brief Gets the items to export and checks that they can be exported with the format
                //! @details With the image formats, the groups are exported as images with all their tracks. With the other formats,
                //! the results are exported per track, so the groups are replaced by their tracks. The tracks that are not
                //! compatible with the format or whose analysis is running are reported as errors.
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the tracks and the groups
                //! @param options The export options
                //! @return For each item to export (track or group), the result of the check
                std::map<juce::String, juce::Result> getExportItems(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers, Document::Exporter::Options const& options);

                //! @brief Exports the results of tracks and groups to a directory
                //! @details As for the export of the application, the export can be performed on a background thread. The items
                //! should be checked with getExportItems() beforehand.
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the tracks and the groups to export
                //! @param options The export options
                //! @param directory The existing directory where the files are created
                //! @param shouldAbort The flag to abort the export
                //! @return For each item, the result of the operation and the exported files
                std::map<juce::String, std::tuple<juce::Result, std::vector<juce::File>>> saveTrackResults(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers, Document::Exporter::Options const& options, juce::File const& directory, std::atomic<bool> const& shouldAbort);

                //! @brief Computes the statistics of a set of values
                //! @param values The values
                //! @return The minimum, maximum, median, mean, variance and skewness of the values
                Statistics computeStatistics(std::vector<float> const& values);

                //! @brief Gets the frame types of tracks
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the tracks
                //! @return For each track identifier, the result of the operation and the frame type of the track
                std::map<juce::String, std::tuple<juce::Result, Track::FrameType>> getTrackTypes(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers);

                //! @brief Gets the plugin descriptions of tracks
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the tracks
                //! @return For each track identifier, the result of the operation and the plugin description of the track
                std::map<juce::String, std::tuple<juce::Result, Plugin::Description>> getTrackDescriptions(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers);

                //! @brief Gets the plugin parameter values of tracks
                //! @details The values of the alias parameters are retrieved from the values of the parameters they represent.
                //! @param documentDir The document director
                //! @param identifiers The identifiers of the tracks
                //! @return For each track identifier, the result of the operation and the values of the plugin parameters of the track
                std::map<juce::String, std::tuple<juce::Result, std::map<std::string, float>>> getTrackParameters(Document::Director& documentDir, std::vector<juce::String> const& identifiers);

                //! @brief Gets the linked tracks of the multitrack outputs of tracks
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the tracks
                //! @return For each track identifier, the result of the operation and the linked tracks of its multitrack output
                std::map<juce::String, std::tuple<juce::Result, LinkedTracks>> getLinkedTracks(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers);

                //! @brief Sets the numbers of linked tracks of the multitrack outputs of tracks within a single undoable action
                //! @details The numbers are clamped to the ranges of the plugins. The new tracks are added above the last
                //! existing track of the output and the tracks that no longer exist are removed (potentially the track itself).
                //! The action is aborted if no track has been modified
                //! @param documentDir The document director
                //! @param counts The identifiers of the tracks and their numbers of linked tracks
                //! @return For each track identifier, the result of the operation and the new linked tracks of its multitrack output
                std::map<juce::String, std::tuple<juce::Result, LinkedTracks>> setLinkedTrackCount(Document::Director& documentDir, std::map<juce::String, size_t> const& counts);

                //! @brief Gets the vertical zooms of tracks
                //! @details The vertical zoom is the range of values for value tracks and the range of bin indices for vector tracks
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the tracks
                //! @return For each track identifier, the result of the operation and the ranges of the vertical zoom
                std::map<juce::String, std::tuple<juce::Result, ZoomRanges>> getTrackVerticalZoom(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers);

                //! @brief Gets the value ranges (used by the color map) of vector tracks
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the tracks
                //! @return For each track identifier, the result of the operation and the ranges of the value zoom
                std::map<juce::String, std::tuple<juce::Result, ZoomRanges>> getTrackValueRange(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers);

                //! @brief Gets the input tracks of tracks
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the tracks
                //! @return For each track identifier, the result of the operation and the identifiers of the input tracks by input identifier
                std::map<juce::String, std::tuple<juce::Result, std::map<juce::String, juce::String>>> getTrackInputs(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers);

                //! @brief Gets the tracks that can be used as inputs of tracks
                //! @param documentDir The document director
                //! @param inputs The identifiers of the tracks and the identifiers of their inputs
                //! @return For each track identifier, the result of the operation and the compatible tracks of each input
                std::map<juce::String, std::tuple<juce::Result, std::vector<CompatibleInput>>> getTrackCompatibleInputs(Document::Director& documentDir, std::map<juce::String, std::set<juce::String>> const& inputs);

                //! @brief Gets the visibility of tracks in their group
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the tracks
                //! @return For each track identifier, the result of the operation and the visibility of the track in its group
                std::map<juce::String, std::tuple<juce::Result, bool>> getTrackIsVisibleInGroup(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers);

                //! @brief Gets the extra thresholds of tracks
                //! @param documentAcsr The document accessor
                //! @param identifiers The identifiers of the tracks
                //! @return For each track identifier, the result of the operation and the threshold of each extra output (if any)
                std::map<juce::String, std::tuple<juce::Result, std::vector<std::optional<float>>>> getTrackExtraThresholds(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers);
            } // namespace Operations
        } // namespace Mcp
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
