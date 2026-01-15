#include "AnlApplicationNeuralyzerMcpOperations.h"
#include "../Document/AnlDocumentSelection.h"
#include "../Document/AnlDocumentTools.h"
#include "../Group/AnlGroupTools.h"
#include "../Plugin/AnlPluginTools.h"
#include "../Track/AnlTrackExporter.h"
#include "../Track/AnlTrackLoader.h"
#include "../Track/AnlTrackTools.h"
#include "../Track/Result/AnlTrackResultModifier.h"
#include "AnlApplicationTools.h"

ANALYSE_FILE_BEGIN

Application::Neuralyzer::Mcp::Operations::ScopedDocumentAction::ScopedDocumentAction(Document::Director& documentDir)
: mDocumentDir(documentDir)
{
    MiscWeakAssert(!mDocumentDir.isPerformingAction());
    mDocumentDir.startAction();
}

Application::Neuralyzer::Mcp::Operations::ScopedDocumentAction::~ScopedDocumentAction()
{
    if(mDocumentDir.isPerformingAction())
    {
        mDocumentDir.endAction(ActionState::abort);
    }
}

void Application::Neuralyzer::Mcp::Operations::ScopedDocumentAction::commit(juce::String const& actionName)
{
    MiscWeakAssert(mDocumentDir.isPerformingAction());
    mDocumentDir.endAction(ActionState::newTransaction, actionName);
}

void Application::Neuralyzer::Mcp::Operations::ScopedDocumentAction::abort()
{
    MiscWeakAssert(mDocumentDir.isPerformingAction());
    mDocumentDir.endAction(ActionState::abort);
}

namespace Application::Neuralyzer::Mcp::Operations
{
    static juce::Result createTrackNotFoundError(juce::String const& identifier)
    {
        return juce::Result::fail(juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier));
    }

    static juce::Result createGroupNotFoundError(juce::String const& identifier)
    {
        return juce::Result::fail(juce::String(R"(The group "GROUPID" doesn't exist.)").replace("GROUPID", identifier));
    }

    static bool isRelativePlacement(Placement placement)
    {
        return placement == Placement::above || placement == Placement::below;
    }

    // Gets the index in a layout (from the top) of an item placed at one end of the layout or relative to an anchor
    // (the layout must not contain the placed item)
    static std::tuple<juce::Result, size_t> getPlacementIndex(std::vector<juce::String> const& layout, Placement placement, juce::String const& anchorIdentifier)
    {
        if(!isRelativePlacement(placement))
        {
            return {juce::Result::ok(), placement == Placement::top ? 0_z : layout.size()};
        }
        auto const it = std::find(layout.cbegin(), layout.cend(), anchorIdentifier);
        if(it == layout.cend())
        {
            return {juce::Result::fail(juce::String(R"(The anchor "ANCHORID" cannot be found.)").replace("ANCHORID", anchorIdentifier)), 0_z};
        }
        auto const index = static_cast<size_t>(std::distance(layout.cbegin(), it));
        return {juce::Result::ok(), placement == Placement::above ? index : index + 1_z};
    }

    // Places the items with the same placement (the same scope, the same placement and the same anchor) in the order
    // of their requests from the top to the bottom: the first item is placed as requested and each following item is
    // placed just below the previous one
    class PlacementChain
    {
    public:
        // Gets the effective placement and anchor of an item
        std::tuple<Placement, juce::String> get(juce::String const& scope, Placement placement, juce::String const& anchorIdentifier) const
        {
            auto const it = mLastIdentifiers.find({scope, placement, anchorIdentifier});
            if(it == mLastIdentifiers.cend())
            {
                return {placement, anchorIdentifier};
            }
            return {Placement::below, it->second};
        }

        // Registers the lowest item placed with a placement
        void set(juce::String const& scope, Placement placement, juce::String const& anchorIdentifier, juce::String const& lowestIdentifier)
        {
            mLastIdentifiers[{scope, placement, anchorIdentifier}] = lowestIdentifier;
        }

    private:
        std::map<std::tuple<juce::String, Placement, juce::String>, juce::String> mLastIdentifiers;
    };

    // Gets the group of a track placed in a group: the group of the anchor track with the above and below placements,
    // otherwise the requested group or the default group if the requested group is empty
    static std::tuple<juce::Result, juce::String> getTrackPlacementGroup(Document::Accessor const& documentAcsr, juce::String const& groupIdentifier, Placement placement, juce::String const& anchorIdentifier, juce::String const& defaultGroupIdentifier)
    {
        if(!isRelativePlacement(placement))
        {
            auto const identifier = groupIdentifier.isEmpty() ? defaultGroupIdentifier : groupIdentifier;
            if(!Document::Tools::hasGroupAcsr(documentAcsr, identifier))
            {
                return {createGroupNotFoundError(identifier), {}};
            }
            return {juce::Result::ok(), identifier};
        }
        if(!Document::Tools::hasTrackAcsr(documentAcsr, anchorIdentifier) || !Document::Tools::isTrackInGroup(documentAcsr, anchorIdentifier))
        {
            return {juce::Result::fail(juce::String(R"(The anchor track "TRACKID" doesn't exist.)").replace("TRACKID", anchorIdentifier)), {}};
        }
        auto const anchorGroupIdentifier = Document::Tools::getGroupAcsrForTrack(documentAcsr, anchorIdentifier).getAttr<Group::AttrType::identifier>();
        if(groupIdentifier.isNotEmpty() && groupIdentifier != anchorGroupIdentifier)
        {
            return {juce::Result::fail(juce::String(R"(The anchor track "TRACKID" is not in the group "GROUPID" (the group is not necessary with the 'above' and 'below' placements).)").replace("TRACKID", anchorIdentifier).replace("GROUPID", groupIdentifier)), {}};
        }
        return {juce::Result::ok(), anchorGroupIdentifier};
    }

    static ZoomRanges getZoomRanges(Zoom::Accessor const& zoomAcsr)
    {
        return {zoomAcsr.getAttr<Zoom::AttrType::visibleRange>(), zoomAcsr.getAttr<Zoom::AttrType::globalRange>()};
    }

    // Applies the function to the accessor of each track (or returns an error if the track doesn't exist)
    // The function returns the result of the operation and the value of the track
    template <typename Function>
    static auto getTracksValues(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers, Function&& function)
    {
        using result_t = decltype(function(std::declval<Track::Accessor const&>()));
        std::map<juce::String, result_t> values;
        for(auto const& identifier : identifiers)
        {
            if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
            {
                values.insert_or_assign(identifier, function(Document::Tools::getTrackAcsr(documentAcsr, identifier)));
            }
            else
            {
                values.insert_or_assign(identifier, result_t{createTrackNotFoundError(identifier), {}});
            }
        }
        return values;
    }

    // Applies the function to the accessor of each group (or returns an error if the group doesn't exist)
    // The function returns the result of the operation and the value of the group
    template <typename Function>
    static auto getGroupsValues(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers, Function&& function)
    {
        using result_t = decltype(function(std::declval<Group::Accessor const&>()));
        std::map<juce::String, result_t> values;
        for(auto const& identifier : identifiers)
        {
            if(Document::Tools::hasGroupAcsr(documentAcsr, identifier))
            {
                values.insert_or_assign(identifier, function(Document::Tools::getGroupAcsr(documentAcsr, identifier)));
            }
            else
            {
                values.insert_or_assign(identifier, result_t{createGroupNotFoundError(identifier), {}});
            }
        }
        return values;
    }

    // Sets the visible range of a zoom and returns the effective ranges
    static std::tuple<juce::Result, ZoomRanges> setVisibleRange(Zoom::Accessor& zoomAcsr, juce::Range<double> const& visibleRange)
    {
        if(visibleRange.isEmpty())
        {
            return {juce::Result::fail("The 'start' value must be less than the 'end' value."), getZoomRanges(zoomAcsr)};
        }
        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange, NotificationType::synchronous);
        return {juce::Result::ok(), getZoomRanges(zoomAcsr)};
    }

    // Gets the vertical zoom accessor of a track (const or not) or the reason why it is not available
    template <typename TrackAccessor>
    static auto getTrackVerticalZoomAcsr(TrackAccessor& trackAcsr)
    {
        using zoom_ref_t = decltype(Track::Tools::getVerticalZoomAccessor(trackAcsr, false));
        auto const frameType = Track::Tools::getFrameType(trackAcsr);
        if(!frameType.has_value())
        {
            return std::make_tuple(juce::Result::fail("The track type is unknown. The analysis might be running."), zoom_ref_t{});
        }
        auto const zoomAcsr = Track::Tools::getVerticalZoomAccessor(trackAcsr, false);
        if(!zoomAcsr.has_value())
        {
            return std::make_tuple(juce::Result::fail("The vertical zoom is only available for value and vector tracks."), zoom_ref_t{});
        }
        return std::make_tuple(juce::Result::ok(), zoomAcsr);
    }

    // Gets the value zoom accessor of a vector track (const or not) or the reason why it is not available
    template <typename TrackAccessor>
    static auto getTrackValueZoomAcsr(TrackAccessor& trackAcsr)
    {
        auto& valueZoomAcsr = trackAcsr.template getAcsr<Track::AcsrType::valueZoom>();
        using zoom_ref_t = optional_ref<std::remove_reference_t<decltype(valueZoomAcsr)>>;
        auto const frameType = Track::Tools::getFrameType(trackAcsr);
        if(!frameType.has_value())
        {
            return std::make_tuple(juce::Result::fail("The track type is unknown. The analysis might be running."), zoom_ref_t{});
        }
        if(frameType.value() != Track::FrameType::vector)
        {
            return std::make_tuple(juce::Result::fail("The value range is only available for vector tracks."), zoom_ref_t{});
        }
        return std::make_tuple(juce::Result::ok(), zoom_ref_t{std::ref(valueZoomAcsr)});
    }

    // Gets the vertical zoom accessor of a group or the reason why it is not available
    static std::tuple<juce::Result, optional_ref<Zoom::Accessor>> getGroupVerticalZoomAcsr(Document::Accessor const& documentAcsr, juce::String const& identifier)
    {
        if(!Document::Tools::hasGroupAcsr(documentAcsr, identifier))
        {
            return {createGroupNotFoundError(identifier), std::nullopt};
        }
        // The vertical zoom of a group is the vertical zoom of its top-most visible value or vector track
        auto const trackAcsr = Group::Tools::getVerticalZoomTrackAcsr(Document::Tools::getGroupAcsr(documentAcsr, identifier));
        if(!trackAcsr.has_value())
        {
            return {juce::Result::fail("The group has no visible value or vector track with a vertical zoom."), std::nullopt};
        }
        return getTrackVerticalZoomAcsr(trackAcsr.value().get());
    }

    static juce::Result const& getResult(juce::Result const& result)
    {
        return result;
    }

    template <typename T>
    static juce::Result const& getResult(std::tuple<juce::Result, T> const& result)
    {
        return std::get<0>(result);
    }

    // Applies the function to each item (an identifier or a pair of an identifier and a value passed as two arguments) within a single
    // undoable action that is committed if the function succeeded at least once
    // The function returns the result of the operation (and optionally a value in a tuple)
    template <typename Container, typename Function>
    static auto performDocumentAction(Document::Director& documentDir, Container const& items, juce::String const& actionName, Function&& function)
    {
        auto const perform = [&](auto const& item)
        {
            if constexpr(std::is_same_v<std::decay_t<decltype(item)>, juce::String>)
            {
                return std::make_pair(item, function(item));
            }
            else
            {
                return std::make_pair(item.first, function(item.first, item.second));
            }
        };
        using result_t = typename decltype(perform(*std::cbegin(items)))::second_type;

        ScopedDocumentAction actionGuard(documentDir);
        std::map<juce::String, result_t> results;
        auto hasChanged = false;
        for(auto const& item : items)
        {
            auto [identifier, result] = perform(item);
            hasChanged = hasChanged || getResult(result).wasOk();
            results.insert_or_assign(identifier, std::move(result));
        }
        if(hasChanged)
        {
            actionGuard.commit(actionName);
        }
        else
        {
            actionGuard.abort();
        }
        return results;
    }

    // Applies the function to each item (an identifier or a pair of an identifier and a value passed as two arguments) within a
    // new transaction of the undo manager, in which the function performs the undoable actions that modify the results (as in the UI)
    // The function returns the result of the operation (and optionally a value in a tuple)
    template <typename Container, typename Function>
    static auto performResultsAction(Document::Director& documentDir, Container const& items, juce::String const& actionName, Function&& function)
    {
        auto const perform = [&](auto const& item)
        {
            if constexpr(std::is_same_v<std::decay_t<decltype(item)>, juce::String>)
            {
                return std::make_pair(item, function(item));
            }
            else
            {
                return std::make_pair(item.first, function(item.first, item.second));
            }
        };
        using result_t = typename decltype(perform(*std::cbegin(items)))::second_type;

        MiscWeakAssert(!documentDir.isPerformingAction());
        documentDir.getUndoManager().beginNewTransaction(actionName);
        std::map<juce::String, result_t> results;
        for(auto const& item : items)
        {
            auto [identifier, result] = perform(item);
            results.insert_or_assign(identifier, std::move(result));
        }
        return results;
    }

    // Sets the global range of the value zoom of a vector track and the zoom value mode that follows the range (so the
    // range is kept when the plugin or the results change)
    static ZoomRanges setValueRange(Track::Accessor& trackAcsr, Zoom::Accessor& valueZoomAcsr, juce::Range<double> const& globalRange)
    {
        auto const pluginRange = Track::Tools::getValueRange(trackAcsr.getAttr<Track::AttrType::description>());
        auto const resultsRange = Track::Tools::getResultRange(trackAcsr);
        if(pluginRange.has_value() && globalRange == pluginRange.value())
        {
            trackAcsr.setAttr<Track::AttrType::zoomValueMode>(Track::ZoomValueMode::plugin, NotificationType::synchronous);
        }
        else if(resultsRange.has_value() && globalRange == resultsRange.value())
        {
            trackAcsr.setAttr<Track::AttrType::zoomValueMode>(Track::ZoomValueMode::results, NotificationType::synchronous);
        }
        else
        {
            trackAcsr.setAttr<Track::AttrType::zoomValueMode>(Track::ZoomValueMode::custom, NotificationType::synchronous);
        }
        valueZoomAcsr.setAttr<Zoom::AttrType::globalRange>(globalRange, NotificationType::synchronous);
        return getZoomRanges(valueZoomAcsr);
    }

    // Checks that a track exists and supports the properties to modify
    static juce::Result checkTrackProperties(Document::Accessor& documentAcsr, juce::String const& identifier, TrackPropertiesModification const& modification)
    {
        if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
        {
            return createTrackNotFoundError(identifier);
        }
        auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
        if(modification.verticalZoom.has_value())
        {
            auto const result = std::get<0_z>(getTrackVerticalZoomAcsr(trackAcsr));
            if(result.failed())
            {
                return result;
            }
            if(modification.verticalZoom.value().isEmpty())
            {
                return juce::Result::fail("The 'start' value of the vertical zoom must be less than the 'end' value.");
            }
        }
        if(modification.valueRange.has_value())
        {
            auto const result = std::get<0_z>(getTrackValueZoomAcsr(trackAcsr));
            if(result.failed())
            {
                return result;
            }
            if(modification.valueRange.value().isEmpty())
            {
                return juce::Result::fail("The 'start' value of the value range must be less than the 'end' value.");
            }
        }
        return juce::Result::ok();
    }

    // Checks that a group exists and that the properties to modify are valid
    static juce::Result checkGroupProperties(Document::Accessor const& documentAcsr, juce::String const& identifier, GroupPropertiesModification const& modification)
    {
        if(!Document::Tools::hasGroupAcsr(documentAcsr, identifier))
        {
            return createGroupNotFoundError(identifier);
        }
        if(modification.layout.has_value())
        {
            // The group layout is ordered from the top to the bottom
            auto const& layout = modification.layout.value();
            auto const currentLayout = Document::Tools::getGroupAcsr(documentAcsr, identifier).getAttr<Group::AttrType::layout>();
            if(std::set<juce::String>(layout.cbegin(), layout.cend()) != std::set<juce::String>(currentLayout.cbegin(), currentLayout.cend()) || layout.size() != currentLayout.size())
            {
                return juce::Result::fail("The layout must contain exactly the tracks of the group (only the order can be changed).");
            }
        }
        if(modification.verticalZoom.has_value())
        {
            auto const result = std::get<0_z>(getGroupVerticalZoomAcsr(documentAcsr, identifier));
            if(result.failed())
            {
                return result;
            }
            if(modification.verticalZoom.value().isEmpty())
            {
                return juce::Result::fail("The 'start' value of the vertical zoom must be less than the 'end' value.");
            }
        }
        return juce::Result::ok();
    }
} // namespace Application::Neuralyzer::Mcp::Operations

std::vector<Application::Neuralyzer::Mcp::Operations::GroupState> Application::Neuralyzer::Mcp::Operations::getDocumentState(Document::Accessor const& documentAcsr)
{
    std::vector<GroupState> groups;
    for(auto const& groupIdentifier : documentAcsr.getAttr<Document::AttrType::layout>())
    {
        auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupIdentifier);
        GroupState group;
        group.identifier = groupIdentifier;
        group.name = groupAcsr.getAttr<Group::AttrType::name>();
        // The group layout is ordered from the top to the bottom
        for(auto const& trackIdentifier : groupAcsr.getAttr<Group::AttrType::layout>())
        {
            auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, trackIdentifier);
            group.tracks.push_back({trackIdentifier, trackAcsr.getAttr<Track::AttrType::name>()});
        }
        groups.push_back(std::move(group));
    }
    return groups;
}

std::vector<AudioFileLayout> Application::Neuralyzer::Mcp::Operations::getDocumentAudioFileLayout(Document::Accessor const& documentAcsr)
{
    return documentAcsr.getAttr<Document::AttrType::reader>();
}

void Application::Neuralyzer::Mcp::Operations::setDocumentAudioFileLayout(Document::Accessor& documentAcsr, std::vector<AudioFileLayout> const& layouts)
{
    documentAcsr.setAttr<Document::AttrType::reader>(layouts, NotificationType::synchronous);
}

std::map<juce::File, std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::AudioFileInfo>> Application::Neuralyzer::Mcp::Operations::getAudioFileInfo(juce::AudioFormatManager& formatManager, std::vector<juce::File> const& files)
{
    std::map<juce::File, std::tuple<juce::Result, AudioFileInfo>> infos;
    for(auto const& file : files)
    {
        if(!file.existsAsFile() || !file.hasReadAccess())
        {
            infos.insert_or_assign(file, std::make_tuple(juce::Result::fail("The file is not readable."), AudioFileInfo{}));
            continue;
        }
        auto const reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(file));
        if(reader == nullptr)
        {
            infos.insert_or_assign(file, std::make_tuple(juce::Result::fail("Failed to create audio reader for file."), AudioFileInfo{}));
            continue;
        }
        AudioFileInfo info;
        info.formatName = reader->getFormatName();
        info.sampleRate = reader->sampleRate;
        info.bitsPerSample = reader->bitsPerSample;
        info.usesFloatingPointData = reader->usesFloatingPointData;
        info.lengthInSamples = reader->lengthInSamples;
        info.numChannels = reader->numChannels;
        info.metadata = reader->metadataValues;
        infos.insert_or_assign(file, std::make_tuple(juce::Result::ok(), std::move(info)));
    }
    return infos;
}

Application::Neuralyzer::Mcp::Operations::ZoomRanges Application::Neuralyzer::Mcp::Operations::getDocumentTimeZoom(Document::Accessor const& documentAcsr)
{
    auto const& timeZoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
    return {timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>(), timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>()};
}

std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::ZoomRanges> Application::Neuralyzer::Mcp::Operations::setDocumentTimeZoom(Document::Accessor& documentAcsr, juce::Range<double> const& visibleRange)
{
    return setVisibleRange(documentAcsr.getAcsr<Document::AcsrType::timeZoom>(), visibleRange);
}

juce::Range<double> Application::Neuralyzer::Mcp::Operations::getTimeSelection(Document::Accessor const& documentAcsr)
{
    return documentAcsr.getAcsr<Document::AcsrType::transport>().getAttr<Transport::AttrType::selection>();
}

Application::Neuralyzer::Mcp::Operations::SelectedItems Application::Neuralyzer::Mcp::Operations::getSelectedItems(Document::Accessor const& documentAcsr)
{
    return {Document::Selection::getGroups(documentAcsr), Document::Selection::getTracks(documentAcsr)};
}

std::map<juce::String, std::tuple<juce::Result, juce::String>> Application::Neuralyzer::Mcp::Operations::getGroupNames(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    return getGroupsValues(documentAcsr, identifiers, [](Group::Accessor const& groupAcsr)
                           {
                               return std::make_tuple(juce::Result::ok(), groupAcsr.getAttr<Group::AttrType::name>());
                           });
}

std::map<juce::String, std::tuple<juce::Result, std::vector<juce::String>>> Application::Neuralyzer::Mcp::Operations::getGroupLayouts(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    return getGroupsValues(documentAcsr, identifiers, [](Group::Accessor const& groupAcsr)
                           {
                               // The group layout is ordered from the top to the bottom
                               return std::make_tuple(juce::Result::ok(), groupAcsr.getAttr<Group::AttrType::layout>());
                           });
}

std::map<juce::String, std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::ZoomRanges>> Application::Neuralyzer::Mcp::Operations::getGroupVerticalZoom(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    std::map<juce::String, std::tuple<juce::Result, ZoomRanges>> zooms;
    for(auto const& identifier : identifiers)
    {
        auto const [result, zoomAcsr] = getGroupVerticalZoomAcsr(documentAcsr, identifier);
        zooms.insert_or_assign(identifier, std::make_tuple(result, result.wasOk() ? getZoomRanges(zoomAcsr.value().get()) : ZoomRanges{}));
    }
    return zooms;
}

std::map<juce::String, std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::GroupPropertiesState>> Application::Neuralyzer::Mcp::Operations::setGroupProperties(Document::Director& documentDir, std::map<juce::String, GroupPropertiesModification> const& modifications)
{
    auto& documentAcsr = documentDir.getAccessor();
    std::map<juce::String, std::tuple<juce::Result, GroupPropertiesState>> results;
    // A group is not modified if one of its properties is invalid
    std::map<juce::String, GroupPropertiesModification> validModifications;
    std::map<juce::String, GroupPropertiesModification> undoableModifications;
    for(auto const& [identifier, modification] : modifications)
    {
        auto const result = checkGroupProperties(documentAcsr, identifier, modification);
        if(result.failed())
        {
            results.insert_or_assign(identifier, std::make_tuple(result, GroupPropertiesState{}));
            continue;
        }
        validModifications.insert_or_assign(identifier, modification);
        if(modification.name.has_value() || modification.layout.has_value())
        {
            undoableModifications.insert_or_assign(identifier, modification);
        }
    }
    // The names and the layouts are modified within a single undoable action
    if(!undoableModifications.empty())
    {
        performDocumentAction(documentDir, undoableModifications, juce::translate("Change group properties (agent)"), [&](juce::String const& identifier, GroupPropertiesModification const& modification)
                              {
                                  auto& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, identifier);
                                  if(modification.name.has_value())
                                  {
                                      groupAcsr.setAttr<Group::AttrType::name>(modification.name.value(), NotificationType::synchronous);
                                  }
                                  if(modification.layout.has_value())
                                  {
                                      groupAcsr.setAttr<Group::AttrType::layout>(modification.layout.value(), NotificationType::synchronous);
                                  }
                                  return juce::Result::ok();
                              });
    }
    // The vertical zooms are modified after the layouts (that can change the track of the vertical zoom of the group)
    for(auto const& [identifier, modification] : validModifications)
    {
        auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, identifier);
        GroupPropertiesState state;
        if(modification.name.has_value())
        {
            state.name = groupAcsr.getAttr<Group::AttrType::name>();
        }
        if(modification.layout.has_value())
        {
            state.layout = groupAcsr.getAttr<Group::AttrType::layout>();
        }
        if(modification.verticalZoom.has_value())
        {
            auto const [result, zoomAcsr] = getGroupVerticalZoomAcsr(documentAcsr, identifier);
            if(result.wasOk())
            {
                state.verticalZoom = std::get<1_z>(setVisibleRange(zoomAcsr.value().get(), modification.verticalZoom.value()));
            }
        }
        results.insert_or_assign(identifier, std::make_tuple(juce::Result::ok(), std::move(state)));
    }
    return results;
}

std::vector<std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::GroupState>> Application::Neuralyzer::Mcp::Operations::createGroups(Document::Director& documentDir, std::vector<GroupCreation> const& groups)
{
    auto& documentAcsr = documentDir.getAccessor();
    ScopedDocumentAction actionGuard(documentDir);
    std::vector<std::tuple<juce::Result, GroupState>> results;
    PlacementChain chain;
    auto hasChanged = false;
    for(auto const& group : groups)
    {
        if(isRelativePlacement(group.placement) && !Document::Tools::hasGroupAcsr(documentAcsr, group.anchorIdentifier))
        {
            results.emplace_back(juce::Result::fail(juce::String(R"(The anchor group "GROUPID" doesn't exist.)").replace("GROUPID", group.anchorIdentifier)), GroupState{});
            continue;
        }
        auto const [placement, anchorIdentifier] = chain.get({}, group.placement, group.anchorIdentifier);
        auto const [placementResult, position] = getPlacementIndex(documentAcsr.getAttr<Document::AttrType::layout>(), placement, anchorIdentifier);
        if(placementResult.failed())
        {
            results.emplace_back(placementResult, GroupState{});
            continue;
        }
        auto const [result, identifier] = documentDir.addGroup(position, NotificationType::synchronous);
        if(result.failed())
        {
            results.emplace_back(juce::Result::fail(juce::String(R"(Failed to create group "NAME": REASON.)").replace("NAME", group.name).replace("REASON", result.getErrorMessage())), GroupState{});
            continue;
        }
        chain.set({}, group.placement, group.anchorIdentifier, identifier);
        auto& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, identifier);
        if(group.name.isNotEmpty())
        {
            groupAcsr.setAttr<Group::AttrType::name>(group.name, NotificationType::synchronous);
        }
        results.emplace_back(juce::Result::ok(), GroupState{identifier, groupAcsr.getAttr<Group::AttrType::name>(), {}});
        hasChanged = true;
    }
    if(hasChanged)
    {
        actionGuard.commit(juce::translate("Create groups (agent)"));
    }
    else
    {
        actionGuard.abort();
    }
    return results;
}

std::map<juce::String, juce::Result> Application::Neuralyzer::Mcp::Operations::removeGroups(Document::Director& documentDir, std::vector<juce::String> const& identifiers)
{
    auto const& documentAcsr = documentDir.getAccessor();
    return performDocumentAction(documentDir, identifiers, juce::translate("Remove groups (agent)"), [&](juce::String const& identifier)
                                 {
                                     if(!Document::Tools::hasGroupAcsr(documentAcsr, identifier))
                                     {
                                         return createGroupNotFoundError(identifier);
                                     }
                                     auto const result = documentDir.removeGroup(identifier, NotificationType::synchronous);
                                     if(result.failed())
                                     {
                                         return juce::Result::fail(juce::String(R"(The group could not be removed: REASON.)").replace("REASON", result.getErrorMessage()));
                                     }
                                     return juce::Result::ok();
                                 });
}

namespace Application::Neuralyzer::Mcp::Operations
{
    // Gets the graphics properties supported by a type of track
    static TrackGraphics getSupportedGraphics(Track::GraphicsSettings const& settings, Track::FrameType frameType)
    {
        auto const isLabel = frameType == Track::FrameType::label;
        auto const isValue = frameType == Track::FrameType::value;
        auto const isVector = frameType == Track::FrameType::vector;
        TrackGraphics graphics;
        if(isVector)
        {
            graphics.colourMap = settings.colours.map;
        }
        if(isLabel || isValue)
        {
            graphics.backgroundColour = settings.colours.background;
            graphics.foregroundColour = settings.colours.foreground;
            graphics.textColour = settings.colours.text;
            graphics.shadowColour = settings.colours.shadow;
            graphics.fontName = settings.font.getName();
            graphics.fontStyle = settings.font.getStyle();
            graphics.fontSize = settings.font.getHeight();
            graphics.lineWidth = settings.lineWidth;
        }
        if(isLabel)
        {
            graphics.durationColour = settings.colours.duration;
            graphics.labelLayoutPosition = settings.labelLayout.position;
            graphics.labelLayoutJustification = settings.labelLayout.justification;
            graphics.markerStyle = settings.markerStyle;
        }
        if(isValue || isVector)
        {
            graphics.unit = settings.unit.value_or(juce::String{});
        }
        return graphics;
    }

    // Merges the properties of a track with the default properties supported by the track
    static TrackGraphics mergeGraphics(TrackGraphics const& graphics, TrackGraphics const& defaultGraphics, TrackGraphics const& supported)
    {
        TrackGraphics merged;
        auto const merge = [&](auto TrackGraphics::* member)
        {
            using value_t = std::decay_t<decltype(merged.*member)>;
            merged.*member = (graphics.*member).has_value() ? graphics.*member : ((supported.*member).has_value() ? defaultGraphics.*member : value_t{});
        };
        merge(&TrackGraphics::colourMap);
        merge(&TrackGraphics::backgroundColour);
        merge(&TrackGraphics::foregroundColour);
        merge(&TrackGraphics::durationColour);
        merge(&TrackGraphics::textColour);
        merge(&TrackGraphics::shadowColour);
        merge(&TrackGraphics::fontName);
        merge(&TrackGraphics::fontStyle);
        merge(&TrackGraphics::fontSize);
        merge(&TrackGraphics::lineWidth);
        merge(&TrackGraphics::unit);
        merge(&TrackGraphics::labelLayoutPosition);
        merge(&TrackGraphics::labelLayoutJustification);
        merge(&TrackGraphics::markerStyle);
        return merged;
    }

    // Gets the names of the properties that are defined but not supported
    static juce::StringArray getUnsupportedGraphics(TrackGraphics const& graphics, TrackGraphics const& supported)
    {
        juce::StringArray names;
        auto const check = [&](auto const& property, auto const& supportedProperty, juce::String const& name)
        {
            if(property.has_value() && !supportedProperty.has_value())
            {
                names.addIfNotAlreadyThere(name);
            }
        };
        check(graphics.colourMap, supported.colourMap, "colour map");
        check(graphics.backgroundColour, supported.backgroundColour, "background colour");
        check(graphics.foregroundColour, supported.foregroundColour, "foreground colour");
        check(graphics.durationColour, supported.durationColour, "duration colour");
        check(graphics.textColour, supported.textColour, "text colour");
        check(graphics.shadowColour, supported.shadowColour, "shadow colour");
        check(graphics.fontName, supported.fontName, "font");
        check(graphics.fontStyle, supported.fontStyle, "font");
        check(graphics.fontSize, supported.fontSize, "font");
        check(graphics.lineWidth, supported.lineWidth, "line width");
        check(graphics.unit, supported.unit, "unit");
        check(graphics.labelLayoutPosition, supported.labelLayoutPosition, "label layout");
        check(graphics.labelLayoutJustification, supported.labelLayoutJustification, "label layout");
        check(graphics.markerStyle, supported.markerStyle, "marker style");
        return names;
    }
} // namespace Application::Neuralyzer::Mcp::Operations

std::map<juce::String, std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::TrackGraphics>> Application::Neuralyzer::Mcp::Operations::getTrackGraphics(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    return getTracksValues(documentAcsr, identifiers, [](Track::Accessor const& trackAcsr)
                           {
                               auto const frameType = Track::Tools::getFrameType(trackAcsr);
                               if(!frameType.has_value())
                               {
                                   return std::make_tuple(juce::Result::fail("The track type is unknown. The analysis might be running."), TrackGraphics{});
                               }
                               return std::make_tuple(juce::Result::ok(), getSupportedGraphics(trackAcsr.getAttr<Track::AttrType::graphicsSettings>(), frameType.value()));
                           });
}

std::vector<juce::Colour> Application::Neuralyzer::Mcp::Operations::getColourMapColours(Track::ColourMap colourMap, size_t count)
{
    std::vector<juce::Colour> colours;
    colours.reserve(count);
    for(auto index = 0_z; index < count; ++index)
    {
        auto const position = (static_cast<double>(index) + 0.5) / static_cast<double>(count);
        auto const colour = tinycolormap::GetColor(position, colourMap);
        colours.push_back(juce::Colour::fromFloatRGBA(static_cast<float>(colour.r()), static_cast<float>(colour.g()), static_cast<float>(colour.b()), 1.0f));
    }
    return colours;
}

std::map<juce::String, juce::Result> Application::Neuralyzer::Mcp::Operations::setTrackGraphics(Document::Director& documentDir, TrackGraphics const& defaultGraphics, std::vector<std::pair<juce::String, TrackGraphics>> const& graphics, std::optional<TrackColourGradient> const& gradient)
{
    auto& documentAcsr = documentDir.getAccessor();
    auto const gradientColours = gradient.has_value() ? getColourMapColours(gradient->colourMap, graphics.size()) : std::vector<juce::Colour>{};
    auto position = 0_z;
    return performDocumentAction(documentDir, graphics, juce::translate("Change track graphics (agent)"), [&](juce::String const& identifier, TrackGraphics const& requestedGraphics)
                                 {
                                     // The tracks are performed in their order
                                     auto const index = position++;
                                     if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                                     {
                                         return createTrackNotFoundError(identifier);
                                     }
                                     auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                                     auto const frameType = Track::Tools::getFrameType(trackAcsr);
                                     if(!frameType.has_value())
                                     {
                                         return juce::Result::fail("The track type is unknown. The analysis might be running.");
                                     }
                                     auto settings = trackAcsr.getAttr<Track::AttrType::graphicsSettings>();
                                     auto const supported = getSupportedGraphics(settings, frameType.value());
                                     auto const unsupported = getUnsupportedGraphics(requestedGraphics, supported);
                                     if(!unsupported.isEmpty())
                                     {
                                         return juce::Result::fail(juce::String("The TYPE tracks don't support the properties: NAMES.").replace("TYPE", juce::String(std::string(magic_enum::enum_name(frameType.value())))).replace("NAMES", unsupported.joinIntoString(", ")));
                                     }
                                     // The colors of the gradient replace the default colors supported by the track (the
                                     // supported properties contain the current colors of the track)
                                     auto trackDefaultGraphics = defaultGraphics;
                                     if(gradient.has_value() && index < gradientColours.size())
                                     {
                                         for(auto const property : gradient->properties)
                                         {
                                             auto const& currentColour = supported.*property;
                                             if(currentColour.has_value())
                                             {
                                                 trackDefaultGraphics.*property = gradientColours.at(index).withAlpha(currentColour->getAlpha());
                                             }
                                         }
                                     }
                                     auto const trackGraphics = mergeGraphics(requestedGraphics, trackDefaultGraphics, supported);

                                     settings.colours.map = trackGraphics.colourMap.value_or(settings.colours.map);
                                     settings.colours.background = trackGraphics.backgroundColour.value_or(settings.colours.background);
                                     settings.colours.foreground = trackGraphics.foregroundColour.value_or(settings.colours.foreground);
                                     settings.colours.duration = trackGraphics.durationColour.value_or(settings.colours.duration);
                                     settings.colours.text = trackGraphics.textColour.value_or(settings.colours.text);
                                     settings.colours.shadow = trackGraphics.shadowColour.value_or(settings.colours.shadow);
                                     if(trackGraphics.fontName.has_value() || trackGraphics.fontStyle.has_value() || trackGraphics.fontSize.has_value())
                                     {
                                         auto const fontName = trackGraphics.fontName.value_or(settings.font.getName());
                                         auto const fontStyle = trackGraphics.fontStyle.value_or(settings.font.getStyle());
                                         auto const fontSize = trackGraphics.fontSize.value_or(settings.font.getHeight());
                                         settings.font = juce::FontOptions(fontName, fontStyle, fontSize);
                                     }
                                     settings.lineWidth = trackGraphics.lineWidth.value_or(settings.lineWidth);
                                     if(trackGraphics.unit.has_value())
                                     {
                                         settings.unit = trackGraphics.unit->isEmpty() ? std::optional<juce::String>{} : trackGraphics.unit;
                                     }
                                     settings.labelLayout.position = trackGraphics.labelLayoutPosition.value_or(settings.labelLayout.position);
                                     settings.labelLayout.justification = trackGraphics.labelLayoutJustification.value_or(settings.labelLayout.justification);
                                     settings.markerStyle = trackGraphics.markerStyle.value_or(settings.markerStyle);
                                     trackAcsr.setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
                                     return juce::Result::ok();
                                 });
}

namespace Application::Neuralyzer::Mcp::Operations
{
    // Gets if a track is linked to the other tracks of its multitrack output (as Track::Director::isLinked())
    static bool isLinkedTrack(Track::Accessor const& trackAcsr)
    {
        return trackAcsr.getAttr<Track::AttrType::voiceLink>().isNotEmpty() && trackAcsr.getAttr<Track::AttrType::file>().isEmpty();
    }

    // Gets the linked tracks that share a voice link (the tracks of the voices of the same analysis)
    static std::tuple<juce::Result, LinkedTracks> getLinkedTracksWithLink(Document::Accessor const& documentAcsr, juce::String const& voiceLink)
    {
        std::vector<std::tuple<size_t, TrackState>> tracks;
        Plugin::Description description;
        for(auto const& trackAcsr : documentAcsr.getAcsrs<Document::AcsrType::tracks>())
        {
            auto const& acsr = trackAcsr.get();
            if(!isLinkedTrack(acsr) || acsr.getAttr<Track::AttrType::voiceLink>() != voiceLink)
            {
                continue;
            }
            // The voices of the descriptions of the linked tracks are synchronized
            description = acsr.getAttr<Track::AttrType::description>();
            auto const& voices = description.voices;
            auto const index = static_cast<size_t>(std::distance(voices.cbegin(), std::find(voices.cbegin(), voices.cend(), acsr.getAttr<Track::AttrType::key>().feature)));
            tracks.emplace_back(index, TrackState{acsr.getAttr<Track::AttrType::identifier>(), acsr.getAttr<Track::AttrType::name>()});
        }
        if(tracks.empty() || !description.hasVoices())
        {
            return {juce::Result::fail("The track doesn't belong to a multitrack output (only the tracks created with a multitrack plugin output and not loaded from a file are linked)."), {}};
        }
        std::stable_sort(tracks.begin(), tracks.end(), [](auto const& lhs, auto const& rhs)
                         {
                             return std::get<0_z>(lhs) < std::get<0_z>(rhs);
                         });

        auto const& descriptor = description.voiceDescriptor;
        LinkedTracks linkedTracks;
        linkedTracks.count = description.voices.size();
        linkedTracks.minCount = Plugin::Tools::clampVoiceCount(descriptor, 0_z);
        linkedTracks.maxCount = Plugin::Tools::clampVoiceCount(descriptor, std::numeric_limits<size_t>::max());
        linkedTracks.defaultCount = Plugin::Tools::clampVoiceCount(descriptor, static_cast<size_t>(std::max(std::round(descriptor.defaultValue), 0.0f)));
        for(auto& track : tracks)
        {
            linkedTracks.tracks.push_back(std::move(std::get<1_z>(track)));
        }
        return {juce::Result::ok(), std::move(linkedTracks)};
    }

    // Gets the voice link of a track (or an error if the track is not linked to other tracks of a multitrack output)
    static std::tuple<juce::Result, juce::String> getTrackVoiceLink(Document::Accessor const& documentAcsr, juce::String const& identifier)
    {
        if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
        {
            return {createTrackNotFoundError(identifier), {}};
        }
        auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
        if(!isLinkedTrack(trackAcsr) || !trackAcsr.getAttr<Track::AttrType::description>().hasVoices())
        {
            return {juce::Result::fail("The track doesn't belong to a multitrack output (only the tracks created with a multitrack plugin output and not loaded from a file are linked)."), {}};
        }
        return {juce::Result::ok(), trackAcsr.getAttr<Track::AttrType::voiceLink>()};
    }

    // Applies the default parameter values (ignored if the track doesn't have the parameter) and the parameter values
    // of the track (that must exist) clamped to the ranges of the parameters
    // The alias parameters are applied after the other parameters because their values depend on the values of the
    // parameters they represent (that they modify). The effective values of the requested parameters and of the other
    // parameters modified are returned.
    static std::tuple<juce::Result, std::map<std::string, float>> applyParameters(Track::Director& trackDir, std::map<std::string, float> const& defaultValues, std::map<std::string, float> const& values)
    {
        auto& trackAcsr = trackDir.getAccessor();
        auto const& parameters = trackAcsr.getAttr<Track::AttrType::description>().parameters;
        auto const findParameter = [&](std::string const& parameterIdentifier)
        {
            return std::find_if(parameters.cbegin(), parameters.cend(), [&](auto const& parameter)
                                {
                                    return parameter.identifier == parameterIdentifier;
                                });
        };
        for(auto const& value : values)
        {
            if(findParameter(value.first) == parameters.cend())
            {
                return {juce::Result::fail(juce::String(R"(The parameter "PARAM" doesn't exist.)").replace("PARAM", value.first)), {}};
            }
        }

        auto mergedValues = values;
        mergedValues.insert(defaultValues.cbegin(), defaultValues.cend());
        auto const previousState = trackAcsr.getAttr<Track::AttrType::state>();
        auto state = previousState;
        std::set<std::string> appliedIdentifiers;
        std::vector<std::pair<std::string, float>> aliasValues;
        for(auto const& [parameterIdentifier, value] : mergedValues)
        {
            auto const it = findParameter(parameterIdentifier);
            if(it != parameters.cend())
            {
                auto const clampedValue = std::clamp(value, it->minValue, it->maxValue);
                if(it->isAlias)
                {
                    aliasValues.emplace_back(parameterIdentifier, clampedValue);
                }
                else
                {
                    state.parameters[parameterIdentifier] = clampedValue;
                }
                appliedIdentifiers.insert(parameterIdentifier);
            }
        }
        if(appliedIdentifiers.empty())
        {
            return {juce::Result::ok(), {}};
        }
        if(!aliasValues.empty())
        {
            // The alias parameters are applied with an instance of the plugin
            auto const currentAliasValues = trackDir.getAliasParameterValues();
            for(auto const& [parameterIdentifier, value] : aliasValues)
            {
                if(currentAliasValues.count(parameterIdentifier) == 0_z)
                {
                    return {juce::Result::fail(juce::String(R"(The parameter "PARAM" is an alias parameter that cannot be applied because the plugin cannot be loaded. Set the parameters it represents instead.)").replace("PARAM", parameterIdentifier)), {}};
                }
                state = trackDir.getStateWithParameterValue(state, parameterIdentifier, value);
            }
        }
        trackAcsr.setAttr<Track::AttrType::state>(state, NotificationType::synchronous);

        auto const& newParameters = trackAcsr.getAttr<Track::AttrType::state>().parameters;
        auto const newAliasValues = aliasValues.empty() ? std::map<std::string, float>{} : trackDir.getAliasParameterValues();
        std::map<std::string, float> appliedValues;
        for(auto const& parameterIdentifier : appliedIdentifiers)
        {
            auto const& newValues = newParameters.count(parameterIdentifier) > 0_z ? newParameters : newAliasValues;
            auto const it = newValues.find(parameterIdentifier);
            if(it != newValues.cend())
            {
                appliedValues[parameterIdentifier] = it->second;
            }
        }
        // The parameters modified by the alias parameters
        for(auto const& [parameterIdentifier, value] : newParameters)
        {
            auto const it = previousState.parameters.find(parameterIdentifier);
            if(it == previousState.parameters.cend() || std::abs(it->second - value) > std::numeric_limits<float>::epsilon())
            {
                appliedValues[parameterIdentifier] = value;
            }
        }
        return {juce::Result::ok(), std::move(appliedValues)};
    }

    // Gets the identifier of the group of a new track (the top group if the requested group is empty)
    static std::tuple<juce::Result, juce::String> getNewTrackGroup(Document::Accessor const& documentAcsr, juce::String const& groupIdentifier)
    {
        auto const& layout = documentAcsr.getAttr<Document::AttrType::layout>();
        if(layout.empty())
        {
            return {juce::Result::fail("The document has no group. Create a group first."), {}};
        }
        auto const identifier = groupIdentifier.isEmpty() ? layout.front() : groupIdentifier;
        if(!Document::Tools::hasGroupAcsr(documentAcsr, identifier))
        {
            return {createGroupNotFoundError(identifier), {}};
        }
        return {juce::Result::ok(), identifier};
    }

    // Creates tracks in order within a single undoable action that is committed if at least one track has been created
    // The function creates a track and returns the result of the operation and the identifiers of the new tracks (the
    // first one is the requested track, the other ones are the other linked tracks of its multitrack output)
    template <typename Creation, typename Function>
    static std::vector<std::tuple<juce::Result, NewTrackState>> createTracks(Document::Director& documentDir, std::vector<Creation> const& tracks, juce::String const& actionName, Function&& function)
    {
        auto& documentAcsr = documentDir.getAccessor();
        // Ensures that the identifiers of the groups and the tracks are unique and valid before adding tracks
        auto const references = documentDir.sanitize(NotificationType::synchronous);
        auto const getReference = [&](juce::String const& identifier)
        {
            auto const it = references.find(identifier);
            return it != references.cend() ? it->second : identifier;
        };
        ScopedDocumentAction actionGuard(documentDir);
        std::vector<std::tuple<juce::Result, NewTrackState>> results;
        auto hasChanged = false;
        for(auto const& track : tracks)
        {
            auto const [groupResult, groupIdentifier] = getNewTrackGroup(documentAcsr, getReference(track.groupIdentifier));
            if(groupResult.failed())
            {
                results.emplace_back(groupResult, NewTrackState{});
                continue;
            }
            std::tuple<juce::Result, std::vector<juce::String>> const creation = function(track, groupIdentifier);
            auto const& [result, identifiers] = creation;
            if(result.failed() || identifiers.empty())
            {
                results.emplace_back(result.failed() ? result : juce::Result::fail("The track could not be created."), NewTrackState{});
                continue;
            }
            auto const getName = [&](juce::String const& identifier)
            {
                return Document::Tools::getTrackAcsr(documentAcsr, identifier).getAttr<Track::AttrType::name>();
            };
            NewTrackState state{identifiers.front(), getName(identifiers.front()), {}};
            for(auto index = 1_z; index < identifiers.size(); ++index)
            {
                state.linkedTracks.push_back({identifiers.at(index), getName(identifiers.at(index))});
            }
            results.emplace_back(juce::Result::ok(), std::move(state));
            hasChanged = true;
        }
        if(hasChanged)
        {
            actionGuard.commit(actionName);
        }
        else
        {
            actionGuard.abort();
        }
        return results;
    }
} // namespace Application::Neuralyzer::Mcp::Operations

std::map<juce::String, std::tuple<juce::Result, std::map<std::string, float>>> Application::Neuralyzer::Mcp::Operations::setTrackParameters(Document::Director& documentDir, std::map<std::string, float> const& defaultValues, std::map<juce::String, std::map<std::string, float>> const& values)
{
    auto& documentAcsr = documentDir.getAccessor();
    using result_t = std::tuple<juce::Result, std::map<std::string, float>>;
    return performDocumentAction(documentDir, values, juce::translate("Change track parameters (agent)"), [&](juce::String const& identifier, std::map<std::string, float> const& trackValues)
                                 {
                                     if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                                     {
                                         return result_t{createTrackNotFoundError(identifier), {}};
                                     }
                                     auto result = applyParameters(documentDir.getTrackDirector(identifier), defaultValues, trackValues);
                                     if(std::get<0>(result).wasOk() && std::get<1>(result).empty())
                                     {
                                         return result_t{juce::Result::fail("The track has none of the parameters."), {}};
                                     }
                                     return result;
                                 });
}

std::vector<std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::NewTrackState>> Application::Neuralyzer::Mcp::Operations::createTracksWithPlugins(Document::Director& documentDir, Track::PresetList::Accessor const& presetListAcsr, Track::GraphicsSettings const& globalGraphicPreset, std::map<std::string, float> const& defaultParameters, std::vector<PluginTrackCreation> const& tracks)
{
    auto& documentAcsr = documentDir.getAccessor();
    return createTracks(documentDir, tracks, juce::translate("Create tracks (agent)"), [&](PluginTrackCreation const& track, juce::String const& groupIdentifier)
                        {
                            auto const& key = track.key;
                            if(key.identifier.find(':') == std::string::npos)
                            {
                                return std::make_tuple(juce::Result::fail(R"(The plugin identifier must be in the form 'maker:plugin'. Consider calling the tools 'search_docs' and 'get_vamp_plugin_descriptions' first to get the correct values.)"), std::vector<juce::String>{});
                            }
                            // If the output is a multitrack output, the linked tracks are created (the number of tracks is
                            // clamped to the range of the plugin and the default number of the plugin is used if not defined)
                            auto const [result, identifiers] = Tools::addPluginTrack(documentDir, presetListAcsr, globalGraphicPreset, groupIdentifier, key, track.linkedTrackCount.value_or(0_z));
                            if(result.failed() || identifiers.empty())
                            {
                                return std::make_tuple(juce::Result::fail(juce::String(R"(Failed to create the track with the plugin {"identifier":"PLUGINID", "feature":"PLUGINFEATURE"}: REASON Consider calling the tools 'search_docs' and 'get_vamp_plugin_descriptions' to get the correct plugin identifier and feature.)").replace("PLUGINID", key.identifier).replace("PLUGINFEATURE", key.feature).replace("REASON", result.getErrorMessage())), std::vector<juce::String>{});
                            }
                            // The parameters are only applied to the first track because they are synchronized with the
                            // other linked tracks
                            auto const parametersResult = std::get<0>(applyParameters(documentDir.getTrackDirector(identifiers.front()), defaultParameters, track.parameters));
                            if(parametersResult.failed())
                            {
                                // The tracks are removed because one of the parameters is invalid
                                for(auto const& identifier : identifiers)
                                {
                                    auto const removeResult = documentDir.removeTrack(identifier, NotificationType::synchronous);
                                    MiscWeakAssert(removeResult.wasOk());
                                }
                                return std::make_tuple(parametersResult, std::vector<juce::String>{});
                            }
                            if(track.name.has_value())
                            {
                                // The name of each linked track is suffixed with the number of the track
                                for(auto index = 0_z; index < identifiers.size(); ++index)
                                {
                                    auto const name = identifiers.size() > 1_z ? track.name.value() + " " + juce::String(index + 1_z) : track.name.value();
                                    Document::Tools::getTrackAcsr(documentAcsr, identifiers.at(index)).setAttr<Track::AttrType::name>(name, NotificationType::synchronous);
                                }
                            }
                            return std::make_tuple(juce::Result::ok(), identifiers);
                        });
}

std::vector<std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::NewTrackState>> Application::Neuralyzer::Mcp::Operations::createTracksWithFiles(Document::Director& documentDir, Track::GraphicsSettings const& globalGraphicPreset, std::vector<FileTrackCreation> const& tracks)
{
    return createTracks(documentDir, tracks, juce::translate("Create tracks from files (agent)"), [&](FileTrackCreation const& track, juce::String const& groupIdentifier)
                        {
                            if(!track.file.existsAsFile() || !track.file.hasReadAccess())
                            {
                                return std::make_tuple(juce::Result::fail(juce::String("The file 'FILEPATH' does not exist or is not readable.").replace("FILEPATH", track.file.getFullPathName())), std::vector<juce::String>{});
                            }
                            auto const [result, identifier] = Tools::addFileTrack(documentDir, globalGraphicPreset, groupIdentifier, track.file);
                            return std::make_tuple(result, result.wasOk() ? std::vector<juce::String>{identifier} : std::vector<juce::String>{});
                        });
}

std::map<juce::String, juce::Result> Application::Neuralyzer::Mcp::Operations::removeTracks(Document::Director& documentDir, std::vector<juce::String> const& identifiers)
{
    auto const& documentAcsr = documentDir.getAccessor();
    return performDocumentAction(documentDir, identifiers, juce::translate("Remove tracks (agent)"), [&](juce::String const& identifier)
                                 {
                                     if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                                     {
                                         return createTrackNotFoundError(identifier);
                                     }
                                     auto const result = documentDir.removeTrack(identifier, NotificationType::synchronous);
                                     if(result.failed())
                                     {
                                         return juce::Result::fail(juce::String(R"(The track could not be removed: REASON.)").replace("REASON", result.getErrorMessage()));
                                     }
                                     return juce::Result::ok();
                                 });
}

std::map<juce::String, juce::Result> Application::Neuralyzer::Mcp::Operations::moveTracks(Document::Director& documentDir, std::vector<TrackMove> const& moves)
{
    auto const& documentAcsr = documentDir.getAccessor();
    std::vector<std::pair<juce::String, TrackMove>> items;
    for(auto const& move : moves)
    {
        items.emplace_back(move.identifier, move);
    }
    PlacementChain chain;
    return performDocumentAction(documentDir, items, juce::translate("Move tracks (agent)"), [&](juce::String const& identifier, TrackMove const& move)
                                 {
                                     if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier) || !Document::Tools::isTrackInGroup(documentAcsr, identifier))
                                     {
                                         return createTrackNotFoundError(identifier);
                                     }
                                     // The track is moved within its group if no group is defined
                                     auto const currentGroupIdentifier = Document::Tools::getGroupAcsrForTrack(documentAcsr, identifier).getAttr<Group::AttrType::identifier>();
                                     auto const [groupResult, groupIdentifier] = getTrackPlacementGroup(documentAcsr, move.groupIdentifier, move.placement, move.anchorIdentifier, currentGroupIdentifier);
                                     if(groupResult.failed())
                                     {
                                         return groupResult;
                                     }
                                     auto const [placement, anchorIdentifier] = chain.get(groupIdentifier, move.placement, move.anchorIdentifier);
                                     if(isRelativePlacement(placement) && anchorIdentifier == identifier)
                                     {
                                         return juce::Result::fail("A track cannot be placed relative to itself.");
                                     }
                                     // The index is defined in the layout of the destination group without the moved track
                                     auto layout = Document::Tools::getGroupAcsr(documentAcsr, groupIdentifier).getAttr<Group::AttrType::layout>();
                                     layout.erase(std::remove(layout.begin(), layout.end(), identifier), layout.end());
                                     auto const [indexResult, index] = getPlacementIndex(layout, placement, anchorIdentifier);
                                     if(indexResult.failed())
                                     {
                                         return indexResult;
                                     }
                                     auto const result = documentDir.moveTrack(groupIdentifier, index, identifier, NotificationType::synchronous);
                                     if(result.failed())
                                     {
                                         return juce::Result::fail(juce::String(R"(The track could not be moved: REASON.)").replace("REASON", result.getErrorMessage()));
                                     }
                                     chain.set(groupIdentifier, move.placement, move.anchorIdentifier, identifier);
                                     return juce::Result::ok();
                                 });
}

std::map<juce::String, juce::Result> Application::Neuralyzer::Mcp::Operations::setTrackInputs(Document::Director& documentDir, std::map<juce::String, std::map<juce::String, juce::String>> const& inputs)
{
    auto& documentAcsr = documentDir.getAccessor();
    auto const& hierarchyManager = documentDir.getHierarchyManager();
    return performDocumentAction(documentDir, inputs, juce::translate("Set track input (agent)"), [&](juce::String const& identifier, std::map<juce::String, juce::String> const& trackInputs)
                                 {
                                     if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                                     {
                                         return createTrackNotFoundError(identifier);
                                     }
                                     auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                                     for(auto const& [inputIdentifier, inputTrackIdentifier] : trackInputs)
                                     {
                                         if(!Track::Tools::supportsInputTrack(trackAcsr, inputIdentifier))
                                         {
                                             return juce::Result::fail(juce::String(R"(The track doesn't support the input "INPUTID".)").replace("INPUTID", inputIdentifier));
                                         }
                                         if(inputTrackIdentifier.isEmpty())
                                         {
                                             continue;
                                         }
                                         if(!Document::Tools::hasTrackAcsr(documentAcsr, inputTrackIdentifier))
                                         {
                                             return createTrackNotFoundError(inputTrackIdentifier);
                                         }
                                         if(!hierarchyManager.isTrackValidFor(identifier, inputIdentifier, inputTrackIdentifier))
                                         {
                                             return juce::Result::fail(juce::String(R"(The track "INPUTTRACKID" can't be used for the input "INPUTID" (unsupported type or circular dependency).)").replace("INPUTTRACKID", inputTrackIdentifier).replace("INPUTID", inputIdentifier));
                                         }
                                     }
                                     auto newInputs = trackAcsr.getAttr<Track::AttrType::inputs>();
                                     for(auto const& [inputIdentifier, inputTrackIdentifier] : trackInputs)
                                     {
                                         newInputs[inputIdentifier] = inputTrackIdentifier;
                                     }
                                     trackAcsr.setAttr<Track::AttrType::inputs>(newInputs, NotificationType::synchronous);
                                     return juce::Result::ok();
                                 });
}

namespace Application::Neuralyzer::Mcp::Operations
{
    // Checks if the results of a track are available (the read access must be acquired by the caller)
    static juce::Result checkResults(Track::Accessor const& trackAcsr, Track::Results const& results, Track::Result::Access const& access)
    {
        if(!Track::Tools::getFrameType(trackAcsr).has_value())
        {
            return juce::Result::fail("The track type is unknown. The analysis might be running.");
        }
        if(!static_cast<bool>(access))
        {
            return juce::Result::fail("The results are being processed.");
        }
        if(results.isEmpty())
        {
            return juce::Result::fail("The track has no results. The analysis might be running.");
        }
        return juce::Result::ok();
    }

    // Gets the time ranges of a selection (or a time range that contains all the results)
    static std::vector<juce::Range<double>> getTimeRanges(ResultsSelection const& selection)
    {
        if(!selection.timeRanges.empty())
        {
            return selection.timeRanges;
        }
        return {juce::Range<double>(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max())};
    }

    // Checks if a time is before the end of a time range that starts before this time
    // As with the selection of the results in the UI, the time range excludes its end (or contains only its start if it is empty)
    static bool isBeforeEnd(juce::Range<double> const& timeRange, double const time)
    {
        return timeRange.isEmpty() ? time <= timeRange.getStart() : time < timeRange.getEnd();
    }

    // Gets the equivalent of a time range for the exporter, that includes the end of the time range and exports all the
    // results if the time range is empty
    static juce::Range<double> getExporterTimeRange(juce::Range<double> const& timeRange)
    {
        auto const start = timeRange.getStart();
        if(timeRange.isEmpty())
        {
            return {start, std::nextafter(start, std::numeric_limits<double>::max())};
        }
        return {start, std::nextafter(timeRange.getEnd(), start)};
    }

    // Calls the function for each result of a channel in a time range that passes the extra thresholds (if applied)
    template <typename ResultType, typename Function>
    static void forEachResult(std::vector<ResultType> const& channelResults, juce::Range<double> const& timeRange, std::vector<std::optional<float>> const& extraThresholds, bool applyExtraThresholds, Function&& function)
    {
        auto it = std::lower_bound(channelResults.cbegin(), channelResults.cend(), timeRange.getStart(), Track::Result::lower_cmp<ResultType>);
        while(it != channelResults.cend() && isBeforeEnd(timeRange, std::get<0_z>(*it)))
        {
            if(!applyExtraThresholds || Track::Result::passThresholds(*it, extraThresholds))
            {
                function(*it);
            }
            ++it;
        }
    }

    // Calls the function with the results of all the channels (markers, points or columns)
    template <typename Function>
    static void visitResults(Track::Results const& results, Function&& function)
    {
        if(auto const markers = results.getMarkers())
        {
            function(*markers);
        }
        else if(auto const points = results.getPoints())
        {
            function(*points);
        }
        else if(auto const columns = results.getColumns())
        {
            function(*columns);
        }
    }

    // Applies the function to the available results of each track (or returns an error if the track doesn't exist
    // or if its results are not available). The function returns the result of the operation and the value of the track
    template <typename Function>
    static auto getTracksResults(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers, Function&& function)
    {
        using result_t = decltype(function(std::declval<Track::Accessor const&>(), std::declval<Track::Results const&>()));
        return getTracksValues(documentAcsr, identifiers, [&](Track::Accessor const& trackAcsr) -> result_t
                               {
                                   auto const& results = trackAcsr.getAttr<Track::AttrType::results>();
                                   auto const access = results.getReadAccess();
                                   auto const result = checkResults(trackAcsr, results, access);
                                   if(result.failed())
                                   {
                                       return result_t{result, {}};
                                   }
                                   return function(trackAcsr, results);
                               });
    }

    // Checks if the results of a track can be modified (the analysis is not running and the results are available)
    static juce::Result checkEditableResults(Track::Accessor const& trackAcsr)
    {
        if(std::get<0_z>(trackAcsr.getAttr<Track::AttrType::processing>()))
        {
            return juce::Result::fail("The analysis of the track is running.");
        }
        auto const& results = trackAcsr.getAttr<Track::AttrType::results>();
        auto const access = results.getReadAccess();
        return checkResults(trackAcsr, results, access);
    }

    // Gets the number of channels of the results (the results must not be accessed for writing)
    static size_t getNumChannels(Track::Results const& results)
    {
        auto const access = results.getReadAccess();
        return static_cast<bool>(access) ? results.getNumChannels().value_or(0_z) : 0_z;
    }

    // Counts the results of all the channels (the results must not be accessed for writing)
    static size_t countAllResults(Track::Results const& results)
    {
        auto const access = results.getReadAccess();
        auto count = 0_z;
        if(static_cast<bool>(access))
        {
            visitResults(results, [&](auto const& channels)
                         {
                             for(auto const& channel : channels)
                             {
                                 count += channel.size();
                             }
                         });
        }
        return count;
    }

    // Converts new results that are markers without labels to points without values (breaks) if the track is a value track,
    // because the CSV loader parses the rows with empty values as markers without labels if all the values are empty
    static Track::Results convertEmptyMarkersToBreaks(Track::Results const& trackResults, Track::Results const& newResults)
    {
        {
            auto const access = trackResults.getReadAccess();
            if(!static_cast<bool>(access) || trackResults.getPoints() == nullptr)
            {
                return newResults;
            }
        }
        auto const access = newResults.getReadAccess();
        auto const markers = newResults.getMarkers();
        if(!static_cast<bool>(access) || markers == nullptr)
        {
            return newResults;
        }
        std::vector<Track::Results::Points> points;
        for(auto const& channel : *markers)
        {
            auto& pointChannel = points.emplace_back();
            for(auto const& marker : channel)
            {
                if(!std::get<2_z>(marker).empty())
                {
                    return newResults;
                }
                pointChannel.push_back({std::get<0_z>(marker), std::get<1_z>(marker), std::optional<float>{}, std::get<3_z>(marker)});
            }
        }
        return Track::Results(std::move(points));
    }

    // Checks if new results (read access acquired by the caller) can replace the results of a track in a time range
    static juce::Result checkNewResults(Track::Results const& trackResults, Track::Results const& newResults, juce::Range<double> const& timeRange)
    {
        {
            auto const access = trackResults.getReadAccess();
            if(!static_cast<bool>(access))
            {
                return juce::Result::fail("The results are being processed.");
            }
            if(trackResults.getColumns() != nullptr)
            {
                return juce::Result::fail("The results of vector tracks cannot be modified.");
            }
            if(trackResults.getMarkers() != nullptr && newResults.getMarkers() == nullptr)
            {
                return juce::Result::fail("The track is a label track but the third column of the CSV results contains numbers instead of labels.");
            }
            if(trackResults.getPoints() != nullptr && newResults.getPoints() == nullptr)
            {
                return juce::Result::fail("The track is a value track but the third column of the CSV results contains labels instead of numbers.");
            }
            auto const numTrackChannels = trackResults.getNumChannels().value_or(0_z);
            auto const numNewChannels = newResults.getNumChannels().value_or(0_z);
            if(numNewChannels > numTrackChannels)
            {
                return juce::Result::fail(juce::String("The CSV results contain NEWCOUNT channels but the track only has TRACKCOUNT channels.").replace("NEWCOUNT", juce::String(numNewChannels)).replace("TRACKCOUNT", juce::String(numTrackChannels)));
            }
        }
        auto result = juce::Result::ok();
        visitResults(newResults, [&](auto const& channels)
                     {
                         for(auto const& channel : channels)
                         {
                             if(!std::is_sorted(channel.cbegin(), channel.cend(), [](auto const& lhs, auto const& rhs)
                                                {
                                                    return std::get<0_z>(lhs) < std::get<0_z>(rhs);
                                                }))
                             {
                                 result = juce::Result::fail("The rows of the CSV results must be in chronological order.");
                                 return;
                             }
                             auto const isOutside = [&](double const time)
                             {
                                 return time < timeRange.getStart() || !isBeforeEnd(timeRange, time);
                             };
                             if(!channel.empty() && (isOutside(std::get<0_z>(channel.front())) || isOutside(std::get<0_z>(channel.back()))))
                             {
                                 result = juce::Result::fail("The times of the CSV results must be within the time range.");
                                 return;
                             }
                         }
                     });
        return result;
    }
} // namespace Application::Neuralyzer::Mcp::Operations

std::vector<std::tuple<juce::Result, Plugin::Description>> Application::Neuralyzer::Mcp::Operations::getVampPluginDescriptions(PluginList::Scanner& scanner, std::vector<Plugin::Key> const& keys)
{
    std::vector<std::tuple<juce::Result, Plugin::Description>> descriptions;
    std::map<Plugin::Key, Plugin::Description> plugins;
    try
    {
        // The scan errors of the other plugins are ignored
        plugins = std::get<0_z>(scanner.getPlugins(48000.0, false));
    }
    catch(std::exception const& e)
    {
        auto const error = juce::Result::fail(juce::String("The scan of the plugins failed: REASON").replace("REASON", e.what()));
        return std::vector<std::tuple<juce::Result, Plugin::Description>>(keys.size(), std::make_tuple(error, Plugin::Description{}));
    }
    for(auto const& key : keys)
    {
        auto const it = plugins.find(key);
        if(it == plugins.cend())
        {
            descriptions.emplace_back(juce::Result::fail(juce::String(R"(The plugin {"identifier":"PLUGINID", "feature":"PLUGINFEATURE"} is not installed. Consider calling the tool 'search_docs' to get the correct plugin identifier and feature.)").replace("PLUGINID", key.identifier).replace("PLUGINFEATURE", key.feature)), Plugin::Description{});
        }
        else
        {
            descriptions.emplace_back(juce::Result::ok(), it->second);
        }
    }
    return descriptions;
}

bool Application::Neuralyzer::Mcp::Operations::hasTrackResultsPending(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    return std::any_of(identifiers.cbegin(), identifiers.cend(), [&](juce::String const& identifier)
                       {
                           if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                           {
                               return false;
                           }
                           auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                           auto const& results = trackAcsr.getAttr<Track::AttrType::results>();
                           auto const access = results.getReadAccess();
                           return checkResults(trackAcsr, results, access).failed();
                       });
}

std::map<juce::String, std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::ResultsSnapshot>> Application::Neuralyzer::Mcp::Operations::getTrackResultsSnapshots(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    return getTracksResults(documentAcsr, identifiers, [](Track::Accessor const& trackAcsr, Track::Results const& results)
                            {
                                ResultsSnapshot snapshot;
                                snapshot.type = Track::Tools::getFrameType(trackAcsr).value_or(Track::FrameType::label);
                                snapshot.results = results;
                                snapshot.extraThresholds = trackAcsr.getAttr<Track::AttrType::extraThresholds>();
                                snapshot.extraOutputs = trackAcsr.getAttr<Track::AttrType::description>().extraOutputs;
                                return std::make_tuple(juce::Result::ok(), std::move(snapshot));
                            });
}

std::tuple<juce::Result, size_t> Application::Neuralyzer::Mcp::Operations::countResults(ResultsSnapshot const& snapshot, ResultsSelection const& selection)
{
    auto const access = snapshot.results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return {juce::Result::fail("The results are being processed."), 0_z};
    }
    auto count = 0_z;
    visitResults(snapshot.results, [&](auto const& channels)
                 {
                     for(auto channel = 0_z; channel < channels.size(); ++channel)
                     {
                         if(!selection.channels.empty() && selection.channels.count(channel) == 0_z)
                         {
                             continue;
                         }
                         for(auto const& timeRange : getTimeRanges(selection))
                         {
                             forEachResult(channels.at(channel), timeRange, snapshot.extraThresholds, selection.applyExtraThresholds, [&]([[maybe_unused]] auto const& result)
                                           {
                                               ++count;
                                           });
                         }
                     }
                 });
    return {juce::Result::ok(), count};
}

std::map<juce::String, std::tuple<juce::Result, std::string>> Application::Neuralyzer::Mcp::Operations::getTrackResultsRaw(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers, ResultsSelection const& selection, Track::Exporter::CsvHeaderType headerType, char separator)
{
    return getTracksResults(documentAcsr, identifiers, [&](Track::Accessor const& trackAcsr, [[maybe_unused]] Track::Results const& results)
                            {
                                std::atomic<bool> shouldAbort{false};
                                std::ostringstream stream;
                                stream << std::fixed;
                                stream << std::setprecision(4);
                                auto const exportRange = [&](juce::Range<double> const& timeRange)
                                {
                                    return Track::Exporter::toCsv(trackAcsr, timeRange, selection.channels, stream, headerType, separator, false, selection.applyExtraThresholds, "\n", false, false, shouldAbort);
                                };
                                if(selection.timeRanges.empty())
                                {
                                    auto const result = exportRange(juce::Range<double>(-1.0, std::numeric_limits<double>::max()));
                                    return std::make_tuple(result, result.wasOk() ? stream.str() : std::string{});
                                }
                                for(auto const& timeRange : selection.timeRanges)
                                {
                                    stream << "Time Range: " << timeRange.getStart() << "s to " << timeRange.getEnd() << "s\n";
                                    auto const result = exportRange(getExporterTimeRange(timeRange));
                                    if(result.failed())
                                    {
                                        return std::make_tuple(result, std::string{});
                                    }
                                    stream << "\n";
                                }
                                return std::make_tuple(juce::Result::ok(), stream.str());
                            });
}

std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::ResultsSummary> Application::Neuralyzer::Mcp::Operations::summarizeResults(ResultsSnapshot const& snapshot, ResultsSelection const& selection)
{
    auto const access = snapshot.results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return {juce::Result::fail("The results are being processed."), {}};
    }
    auto const& extraThresholds = snapshot.extraThresholds;
    auto const& extraOutputs = snapshot.extraOutputs;
    ResultsSummary summary;
    summary.type = snapshot.type;
    visitResults(snapshot.results, [&](auto const& channels)
                 {
                     using result_t = typename std::decay_t<decltype(channels)>::value_type::value_type;
                     for(auto channel = 0_z; channel < channels.size(); ++channel)
                     {
                         if(!selection.channels.empty() && selection.channels.count(channel) == 0_z)
                         {
                             continue;
                         }
                         ResultsChannelSummary channelSummary;
                         channelSummary.channel = channel;
                         auto const summarize = [&](juce::Range<double> const& timeRange, std::optional<juce::Range<double>> const& requestedTimeRange)
                         {
                             ResultsRangeSummary rangeSummary;
                             rangeSummary.timeRange = requestedTimeRange;
                             std::vector<float> values;
                             std::vector<std::vector<float>> extraValues(extraOutputs.size());
                             if constexpr(std::is_same_v<result_t, Track::Results::Point>)
                             {
                                 rangeSummary.breakCount = 0_z;
                             }
                             forEachResult(channels.at(channel), timeRange, extraThresholds, selection.applyExtraThresholds, [&](result_t const& result)
                                           {
                                               ++rangeSummary.count;
                                               auto const& resultExtraValues = std::get<3_z>(result);
                                               if(resultExtraValues.size() > extraValues.size())
                                               {
                                                   extraValues.resize(resultExtraValues.size());
                                               }
                                               for(auto index = 0_z; index < resultExtraValues.size(); ++index)
                                               {
                                                   extraValues.at(index).push_back(resultExtraValues.at(index));
                                               }
                                               if constexpr(std::is_same_v<result_t, Track::Results::Point>)
                                               {
                                                   if(std::get<2_z>(result).has_value())
                                                   {
                                                       values.push_back(std::get<2_z>(result).value());
                                                   }
                                                   else
                                                   {
                                                       rangeSummary.breakCount = rangeSummary.breakCount.value_or(0_z) + 1_z;
                                                   }
                                               }
                                               else if constexpr(std::is_same_v<result_t, Track::Results::Column>)
                                               {
                                                   values.insert(values.end(), std::get<2_z>(result).cbegin(), std::get<2_z>(result).cend());
                                               }
                                           });
                             if constexpr(!std::is_same_v<result_t, Track::Results::Marker>)
                             {
                                 rangeSummary.values = computeStatistics(values);
                             }
                             for(auto index = 0_z; index < extraValues.size(); ++index)
                             {
                                 ExtraValuesSummary extraValuesSummary;
                                 extraValuesSummary.index = index;
                                 auto const defaultName = juce::String("Extra INDEX").replace("INDEX", juce::String(index + 1_z));
                                 if(index < extraOutputs.size())
                                 {
                                     auto const& extraOutput = extraOutputs.at(index);
                                     extraValuesSummary.identifier = extraOutput.identifier;
                                     extraValuesSummary.name = extraOutput.name.empty() ? defaultName : juce::String(extraOutput.name);
                                     extraValuesSummary.unit = extraOutput.unit;
                                 }
                                 else
                                 {
                                     extraValuesSummary.name = defaultName;
                                 }
                                 extraValuesSummary.statistics = computeStatistics(extraValues.at(index));
                                 rangeSummary.extraValues.push_back(std::move(extraValuesSummary));
                             }
                             channelSummary.ranges.push_back(std::move(rangeSummary));
                         };
                         if(selection.timeRanges.empty())
                         {
                             summarize(getTimeRanges(selection).front(), std::nullopt);
                         }
                         for(auto const& timeRange : selection.timeRanges)
                         {
                             summarize(timeRange, timeRange);
                         }
                         summary.channels.push_back(std::move(channelSummary));
                     }
                 });
    return {juce::Result::ok(), std::move(summary)};
}

std::map<juce::String, juce::Result> Application::Neuralyzer::Mcp::Operations::setTrackResultsRaw(Document::Director& documentDir, std::map<juce::String, ResultsModification> const& modifications, char separator)
{
    auto const& documentAcsr = documentDir.getAccessor();
    auto& undoManager = documentDir.getUndoManager();
    return performResultsAction(documentDir, modifications, juce::translate("Set track results (agent)"), [&](juce::String const& identifier, ResultsModification const& modification)
                                {
                                    if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                                    {
                                        return createTrackNotFoundError(identifier);
                                    }
                                    auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                                    if(auto const result = checkEditableResults(trackAcsr); result.failed())
                                    {
                                        return result;
                                    }
                                    std::istringstream stream(modification.results);
                                    std::atomic<bool> shouldAbort{false};
                                    std::atomic<float> advancement{0.0f};
                                    auto const loadedResults = Track::Loader::loadFromCsv(stream, separator, false, '\n', false, shouldAbort, advancement);
                                    if(auto const* error = std::get_if<juce::String>(&loadedResults))
                                    {
                                        return juce::Result::fail("Failed to parse the CSV results: " + *error);
                                    }
                                    auto const data = convertEmptyMarkersToBreaks(trackAcsr.getAttr<Track::AttrType::results>(), std::get<Track::Results>(loadedResults));
                                    auto const access = data.getReadAccess();
                                    if(!static_cast<bool>(access))
                                    {
                                        return juce::Result::fail("The parsed results cannot be accessed.");
                                    }
                                    if(data.isEmpty())
                                    {
                                        return juce::Result::fail("The CSV results are empty.");
                                    }

                                    auto const timeRange = modification.timeRange.value_or(juce::Range<double>(0.0, std::numeric_limits<double>::max()));
                                    if(auto const result = checkNewResults(trackAcsr.getAttr<Track::AttrType::results>(), data, timeRange); result.failed())
                                    {
                                        return result;
                                    }

                                    // As with the edition of the results in the UI, the time range is erased and the results are inserted at their own times
                                    auto const trackAccessorFn = documentDir.getSafeTrackAccessorFn(identifier);
                                    visitResults(data, [&](auto const& channels)
                                                 {
                                                     for(auto channel = 0_z; channel < channels.size(); ++channel)
                                                     {
                                                         auto const& channelResults = channels.at(channel);
                                                         undoManager.perform(std::make_unique<Track::Result::Modifier::ActionErase>(trackAccessorFn, channel, timeRange, false, timeRange.getEnd()).release());
                                                         if(!channelResults.empty())
                                                         {
                                                             auto const destination = std::get<0_z>(channelResults.front());
                                                             undoManager.perform(std::make_unique<Track::Result::Modifier::ActionPaste>(trackAccessorFn, channel, channelResults, destination, false, timeRange.getEnd()).release());
                                                         }
                                                     }
                                                 });
                                    return juce::Result::ok();
                                });
}

std::map<juce::String, std::tuple<juce::Result, size_t>> Application::Neuralyzer::Mcp::Operations::eraseTrackResults(Document::Director& documentDir, std::vector<juce::String> const& identifiers, std::vector<juce::Range<double>> const& timeRanges, std::set<size_t> const& channels)
{
    auto const& documentAcsr = documentDir.getAccessor();
    auto& undoManager = documentDir.getUndoManager();
    return performResultsAction(documentDir, identifiers, juce::translate("Erase track results (agent)"), [&](juce::String const& identifier) -> std::tuple<juce::Result, size_t>
                                {
                                    if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                                    {
                                        return {createTrackNotFoundError(identifier), 0_z};
                                    }
                                    auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                                    if(auto const result = checkEditableResults(trackAcsr); result.failed())
                                    {
                                        return {result, 0_z};
                                    }
                                    auto const numChannels = getNumChannels(trackAcsr.getAttr<Track::AttrType::results>());
                                    for(auto const channel : channels)
                                    {
                                        if(channel >= numChannels)
                                        {
                                            return {juce::Result::fail(juce::String("The channel INDEX doesn't exist, the track has COUNT channels.").replace("INDEX", juce::String(channel)).replace("COUNT", juce::String(numChannels))), 0_z};
                                        }
                                    }

                                    // As with the deletion of the frames in the UI, the results are erased for each channel and each time range
                                    auto const previousCount = countAllResults(trackAcsr.getAttr<Track::AttrType::results>());
                                    auto const trackAccessorFn = documentDir.getSafeTrackAccessorFn(identifier);
                                    for(auto channel = 0_z; channel < numChannels; ++channel)
                                    {
                                        if(!channels.empty() && !channels.contains(channel))
                                        {
                                            continue;
                                        }
                                        for(auto const& timeRange : timeRanges)
                                        {
                                            undoManager.perform(std::make_unique<Track::Result::Modifier::ActionErase>(trackAccessorFn, channel, timeRange, false, timeRange.getEnd()).release());
                                        }
                                    }
                                    auto const currentCount = countAllResults(trackAcsr.getAttr<Track::AttrType::results>());
                                    return {juce::Result::ok(), previousCount - std::min(previousCount, currentCount)};
                                });
}

std::map<juce::String, juce::Result> Application::Neuralyzer::Mcp::Operations::getExportItems(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers, Document::Exporter::Options const& options)
{
    auto const checkTrack = [&](juce::String const& identifier)
    {
        if(options.useImageFormat())
        {
            return juce::Result::ok();
        }
        auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
        if(std::get<0_z>(trackAcsr.getAttr<Track::AttrType::processing>()))
        {
            return juce::Result::fail("The analysis of the track is running.");
        }
        auto const frameType = Track::Tools::getFrameType(trackAcsr);
        if(!frameType.has_value())
        {
            return juce::Result::fail("The track type is unknown. The analysis might be running.");
        }
        if(!options.isCompatible(frameType.value()))
        {
            return juce::Result::fail(juce::String("The track is not compatible with the FORMAT format.").replace("FORMAT", options.getFormatName()));
        }
        return juce::Result::ok();
    };

    std::map<juce::String, juce::Result> items;
    for(auto const& identifier : identifiers)
    {
        if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
        {
            items.insert_or_assign(identifier, checkTrack(identifier));
        }
        else if(!Document::Tools::hasGroupAcsr(documentAcsr, identifier))
        {
            items.insert_or_assign(identifier, juce::Result::fail(juce::String(R"(The item (group or track) "ITEMID" doesn't exist.)").replace("ITEMID", identifier)));
        }
        else if(options.useImageFormat())
        {
            items.insert_or_assign(identifier, juce::Result::ok());
        }
        else
        {
            // The text and binary formats export the results of the tracks, so the group is replaced by its tracks
            auto const trackIdentifiers = Document::Tools::getEffectiveTrackIdentifiers(documentAcsr, identifier);
            if(trackIdentifiers.empty())
            {
                items.insert_or_assign(identifier, juce::Result::fail("The group doesn't contain any track."));
            }
            for(auto const& trackIdentifier : trackIdentifiers)
            {
                items.insert_or_assign(trackIdentifier, checkTrack(trackIdentifier));
            }
        }
    }
    return items;
}

std::map<juce::String, std::tuple<juce::Result, std::vector<juce::File>>> Application::Neuralyzer::Mcp::Operations::saveTrackResults(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers, Document::Exporter::Options const& options, juce::File const& directory, std::atomic<bool> const& shouldAbort)
{
    auto const getFiles = [&]()
    {
        auto const childFiles = directory.findChildFiles(juce::File::findFiles, false, "*");
        return std::set<juce::File>(childFiles.begin(), childFiles.end());
    };

    std::map<juce::String, std::tuple<juce::Result, std::vector<juce::File>>> results;
    for(auto const& identifier : identifiers)
    {
        // The files created by the export of an item are the files that didn't exist before
        auto const previousFiles = getFiles();
        auto const result = Document::Exporter::exportTo(documentAcsr, directory, {}, {}, "", identifier, options, shouldAbort);
        std::vector<juce::File> files;
        std::ranges::copy_if(getFiles(), std::back_inserter(files), [&](juce::File const& file)
                             {
                                 return !previousFiles.contains(file);
                             });
        if(result.failed())
        {
            results.insert_or_assign(identifier, std::make_tuple(result, std::vector<juce::File>{}));
        }
        else if(files.empty())
        {
            results.insert_or_assign(identifier, std::make_tuple(juce::Result::fail("The export completed successfully, but no files were created."), std::vector<juce::File>{}));
        }
        else
        {
            results.insert_or_assign(identifier, std::make_tuple(juce::Result::ok(), std::move(files)));
        }
    }
    return results;
}

Application::Neuralyzer::Mcp::Operations::Statistics Application::Neuralyzer::Mcp::Operations::computeStatistics(std::vector<float> const& values)
{
    if(values.empty())
    {
        return {};
    }
    Statistics statistics;
    auto const [minIt, maxIt] = std::minmax_element(values.cbegin(), values.cend());
    statistics.min = *minIt;
    statistics.max = *maxIt;

    auto sorted = values;
    std::sort(sorted.begin(), sorted.end());
    auto const mid = sorted.size() / 2_z;
    statistics.median = sorted.size() % 2_z == 0_z ? (static_cast<double>(sorted.at(mid - 1_z)) + static_cast<double>(sorted.at(mid))) / 2.0 : static_cast<double>(sorted.at(mid));

    auto const count = static_cast<long double>(values.size());
    auto const mean = std::accumulate(values.cbegin(), values.cend(), 0.0L) / count;
    auto variance = 0.0L;
    auto skewness = 0.0L;
    for(auto const& value : values)
    {
        auto const centered = static_cast<long double>(value) - mean;
        auto const power = centered * centered;
        variance += power;
        skewness += power * centered;
    }
    variance /= count;
    skewness /= count;
    auto const stdDeviation = std::sqrt(variance);
    statistics.mean = static_cast<double>(mean);
    statistics.variance = static_cast<double>(variance);
    statistics.skewness = stdDeviation > 0.0L ? static_cast<double>(skewness / (stdDeviation * stdDeviation * stdDeviation)) : 0.0;
    return statistics;
}

std::map<juce::String, std::tuple<juce::Result, Track::FrameType>> Application::Neuralyzer::Mcp::Operations::getTrackTypes(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    return getTracksValues(documentAcsr, identifiers, [](Track::Accessor const& trackAcsr)
                           {
                               auto const frameType = Track::Tools::getFrameType(trackAcsr);
                               if(!frameType.has_value())
                               {
                                   return std::make_tuple(juce::Result::fail("The track type is unknown. The analysis might be running."), Track::FrameType::label);
                               }
                               return std::make_tuple(juce::Result::ok(), frameType.value());
                           });
}

std::map<juce::String, std::tuple<juce::Result, Plugin::Description>> Application::Neuralyzer::Mcp::Operations::getTrackDescriptions(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    return getTracksValues(documentAcsr, identifiers, [](Track::Accessor const& trackAcsr)
                           {
                               return std::make_tuple(juce::Result::ok(), trackAcsr.getAttr<Track::AttrType::description>());
                           });
}

std::map<juce::String, std::tuple<juce::Result, std::map<std::string, float>>> Application::Neuralyzer::Mcp::Operations::getTrackParameters(Document::Director& documentDir, std::vector<juce::String> const& identifiers)
{
    return getTracksValues(documentDir.getAccessor(), identifiers, [&](Track::Accessor const& trackAcsr)
                           {
                               // The values of the alias parameters are not part of the state (they depend on the parameters they represent)
                               auto values = trackAcsr.getAttr<Track::AttrType::state>().parameters;
                               auto const aliasValues = documentDir.getTrackDirector(trackAcsr.getAttr<Track::AttrType::identifier>()).getAliasParameterValues();
                               values.insert(aliasValues.cbegin(), aliasValues.cend());
                               return std::make_tuple(juce::Result::ok(), std::move(values));
                           });
}

std::map<juce::String, std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::LinkedTracks>> Application::Neuralyzer::Mcp::Operations::getLinkedTracks(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    std::map<juce::String, std::tuple<juce::Result, LinkedTracks>> results;
    for(auto const& identifier : identifiers)
    {
        auto const [linkResult, voiceLink] = getTrackVoiceLink(documentAcsr, identifier);
        results.insert_or_assign(identifier, linkResult.failed() ? std::make_tuple(linkResult, LinkedTracks{}) : getLinkedTracksWithLink(documentAcsr, voiceLink));
    }
    return results;
}

std::map<juce::String, std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::LinkedTracks>> Application::Neuralyzer::Mcp::Operations::setLinkedTrackCount(Document::Director& documentDir, std::map<juce::String, size_t> const& counts)
{
    auto& documentAcsr = documentDir.getAccessor();
    using result_t = std::tuple<juce::Result, LinkedTracks>;
    return performDocumentAction(documentDir, counts, juce::translate("Change the number of tracks (agent)"), [&](juce::String const& identifier, size_t count)
                                 {
                                     auto const [linkResult, voiceLink] = getTrackVoiceLink(documentAcsr, identifier);
                                     if(linkResult.failed())
                                     {
                                         return result_t{linkResult, {}};
                                     }
                                     auto const previous = getLinkedTracksWithLink(documentAcsr, voiceLink);
                                     if(std::get<0_z>(previous).failed())
                                     {
                                         return previous;
                                     }
                                     // The track itself can be removed if it no longer exists so the linked tracks are retrieved with the link
                                     auto const result = documentDir.setVoiceCount(voiceLink, count, NotificationType::synchronous);
                                     if(result.failed())
                                     {
                                         return result_t{result, {}};
                                     }
                                     auto current = getLinkedTracksWithLink(documentAcsr, voiceLink);
                                     if(std::get<0_z>(current).failed())
                                     {
                                         // All the remaining linked tracks have been removed
                                         auto linkedTracks = std::get<1_z>(previous);
                                         linkedTracks.count = std::clamp(count, linkedTracks.minCount, linkedTracks.maxCount);
                                         linkedTracks.tracks.clear();
                                         return result_t{juce::Result::ok(), std::move(linkedTracks)};
                                     }
                                     return current;
                                 });
}

std::map<juce::String, std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::ZoomRanges>> Application::Neuralyzer::Mcp::Operations::getTrackVerticalZoom(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    return getTracksValues(documentAcsr, identifiers, [](Track::Accessor const& trackAcsr)
                           {
                               auto const [result, zoomAcsr] = getTrackVerticalZoomAcsr(trackAcsr);
                               return std::make_tuple(result, result.wasOk() ? getZoomRanges(zoomAcsr.value().get()) : ZoomRanges{});
                           });
}

std::map<juce::String, std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::ZoomRanges>> Application::Neuralyzer::Mcp::Operations::getTrackValueRange(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    return getTracksValues(documentAcsr, identifiers, [](Track::Accessor const& trackAcsr)
                           {
                               auto const [result, zoomAcsr] = getTrackValueZoomAcsr(trackAcsr);
                               return std::make_tuple(result, result.wasOk() ? getZoomRanges(zoomAcsr.value().get()) : ZoomRanges{});
                           });
}

std::map<juce::String, std::tuple<juce::Result, std::map<juce::String, juce::String>>> Application::Neuralyzer::Mcp::Operations::getTrackInputs(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    return getTracksValues(documentAcsr, identifiers, [](Track::Accessor const& trackAcsr)
                           {
                               if(!Track::Tools::supportsInputTracks(trackAcsr))
                               {
                                   return std::make_tuple(juce::Result::fail("The track doesn't support input tracks."), std::map<juce::String, juce::String>{});
                               }
                               return std::make_tuple(juce::Result::ok(), trackAcsr.getAttr<Track::AttrType::inputs>());
                           });
}

std::map<juce::String, std::tuple<juce::Result, std::vector<Application::Neuralyzer::Mcp::Operations::CompatibleInput>>> Application::Neuralyzer::Mcp::Operations::getTrackCompatibleInputs(Document::Director& documentDir, std::map<juce::String, std::set<juce::String>> const& inputs)
{
    auto const& documentAcsr = documentDir.getAccessor();
    auto const& hierarchyManager = documentDir.getHierarchyManager();
    std::map<juce::String, std::tuple<juce::Result, std::vector<CompatibleInput>>> results;
    for(auto const& [identifier, inputIdentifiers] : inputs)
    {
        if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
        {
            results.insert_or_assign(identifier, std::make_tuple(createTrackNotFoundError(identifier), std::vector<CompatibleInput>{}));
            continue;
        }
        std::vector<CompatibleInput> compatibleInputs;
        for(auto const& inputIdentifier : inputIdentifiers)
        {
            CompatibleInput compatibleInput;
            compatibleInput.inputIdentifier = inputIdentifier;
            for(auto const& trackInfo : hierarchyManager.getAvailableTracksFor(identifier, inputIdentifier))
            {
                compatibleInput.compatibleIdentifiers.push_back(trackInfo.identifier);
            }
            compatibleInputs.push_back(std::move(compatibleInput));
        }
        results.insert_or_assign(identifier, std::make_tuple(juce::Result::ok(), std::move(compatibleInputs)));
    }
    return results;
}

std::map<juce::String, std::tuple<juce::Result, bool>> Application::Neuralyzer::Mcp::Operations::getTrackIsVisibleInGroup(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    return getTracksValues(documentAcsr, identifiers, [](Track::Accessor const& trackAcsr)
                           {
                               return std::make_tuple(juce::Result::ok(), trackAcsr.getAttr<Track::AttrType::showInGroup>());
                           });
}

std::map<juce::String, std::tuple<juce::Result, std::vector<std::optional<float>>>> Application::Neuralyzer::Mcp::Operations::getTrackExtraThresholds(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    return getTracksValues(documentAcsr, identifiers, [](Track::Accessor const& trackAcsr)
                           {
                               return std::make_tuple(juce::Result::ok(), trackAcsr.getAttr<Track::AttrType::extraThresholds>());
                           });
}

std::map<juce::String, std::tuple<juce::Result, juce::String>> Application::Neuralyzer::Mcp::Operations::getTrackNames(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    return getTracksValues(documentAcsr, identifiers, [](Track::Accessor const& trackAcsr)
                           {
                               return std::make_tuple(juce::Result::ok(), trackAcsr.getAttr<Track::AttrType::name>());
                           });
}

std::map<juce::String, std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::TrackPropertiesState>> Application::Neuralyzer::Mcp::Operations::setTrackProperties(Document::Director& documentDir, std::map<juce::String, TrackPropertiesModification> const& modifications)
{
    auto& documentAcsr = documentDir.getAccessor();
    std::map<juce::String, std::tuple<juce::Result, TrackPropertiesState>> results;
    // A track is not modified if one of its properties is invalid or not supported by its type
    std::map<juce::String, TrackPropertiesModification> validModifications;
    std::map<juce::String, TrackPropertiesModification> undoableModifications;
    for(auto const& [identifier, modification] : modifications)
    {
        auto const result = checkTrackProperties(documentAcsr, identifier, modification);
        if(result.failed())
        {
            results.insert_or_assign(identifier, std::make_tuple(result, TrackPropertiesState{}));
            continue;
        }
        validModifications.insert_or_assign(identifier, modification);
        if(modification.name.has_value() || modification.visibleInGroup.has_value() || modification.extraThresholds.has_value())
        {
            undoableModifications.insert_or_assign(identifier, modification);
        }
    }
    // The names, the visibilities and the extra thresholds are modified within a single undoable action
    if(!undoableModifications.empty())
    {
        performDocumentAction(documentDir, undoableModifications, juce::translate("Change track properties (agent)"), [&](juce::String const& identifier, TrackPropertiesModification const& modification)
                              {
                                  auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                                  if(modification.name.has_value())
                                  {
                                      trackAcsr.setAttr<Track::AttrType::name>(modification.name.value(), NotificationType::synchronous);
                                  }
                                  if(modification.visibleInGroup.has_value())
                                  {
                                      trackAcsr.setAttr<Track::AttrType::showInGroup>(modification.visibleInGroup.value(), NotificationType::synchronous);
                                  }
                                  if(modification.extraThresholds.has_value())
                                  {
                                      trackAcsr.setAttr<Track::AttrType::extraThresholds>(modification.extraThresholds.value(), NotificationType::synchronous);
                                  }
                                  return juce::Result::ok();
                              });
    }
    // The zooms are not part of the undoable actions
    for(auto const& [identifier, modification] : validModifications)
    {
        auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
        TrackPropertiesState state;
        if(modification.name.has_value())
        {
            state.name = trackAcsr.getAttr<Track::AttrType::name>();
        }
        if(modification.visibleInGroup.has_value())
        {
            state.visibleInGroup = trackAcsr.getAttr<Track::AttrType::showInGroup>();
        }
        if(modification.extraThresholds.has_value())
        {
            state.extraThresholds = trackAcsr.getAttr<Track::AttrType::extraThresholds>();
        }
        if(modification.valueRange.has_value())
        {
            auto const [result, zoomAcsr] = getTrackValueZoomAcsr(trackAcsr);
            if(result.wasOk())
            {
                state.valueRange = setValueRange(trackAcsr, zoomAcsr.value().get(), modification.valueRange.value());
            }
        }
        if(modification.verticalZoom.has_value())
        {
            auto const [result, zoomAcsr] = getTrackVerticalZoomAcsr(trackAcsr);
            if(result.wasOk())
            {
                state.verticalZoom = std::get<1_z>(setVisibleRange(zoomAcsr.value().get(), modification.verticalZoom.value()));
            }
        }
        results.insert_or_assign(identifier, std::make_tuple(juce::Result::ok(), std::move(state)));
    }
    return results;
}

namespace Application::Neuralyzer::Mcp::Operations
{
    class StatisticsUnitTest
    : public juce::UnitTest
    {
    public:
        StatisticsUnitTest()
        : juce::UnitTest("Mcp Operations Statistics", "Application")
        {
        }

        ~StatisticsUnitTest() override = default;

        void runTest() override
        {
            beginTest("empty");
            {
                auto const statistics = computeStatistics({});
                expect(!statistics.min.has_value() && !statistics.max.has_value() && !statistics.median.has_value());
                expect(!statistics.mean.has_value() && !statistics.variance.has_value() && !statistics.skewness.has_value());
            }

            beginTest("single value");
            {
                auto const statistics = computeStatistics({2.0f});
                expectEquals(statistics.min.value_or(0.0f), 2.0f);
                expectEquals(statistics.max.value_or(0.0f), 2.0f);
                expectEquals(statistics.median.value_or(0.0), 2.0);
                expectEquals(statistics.mean.value_or(0.0), 2.0);
                expectEquals(statistics.variance.value_or(1.0), 0.0);
                expectEquals(statistics.skewness.value_or(1.0), 0.0);
            }

            beginTest("values");
            {
                auto const statistics = computeStatistics({4.0f, 1.0f, 3.0f, 2.0f});
                expectEquals(statistics.min.value_or(0.0f), 1.0f);
                expectEquals(statistics.max.value_or(0.0f), 4.0f);
                expectEquals(statistics.median.value_or(0.0), 2.5);
                expectEquals(statistics.mean.value_or(0.0), 2.5);
                expectEquals(statistics.variance.value_or(0.0), 1.25);
                expectWithinAbsoluteError(statistics.skewness.value_or(1.0), 0.0, 1e-9);
            }

            beginTest("skewness");
            {
                auto const statistics = computeStatistics({1.0f, 1.0f, 1.0f, 5.0f});
                expect(statistics.skewness.value_or(0.0) > 0.0);
            }
        }
    };

    static StatisticsUnitTest statisticsUnitTest;

    class LinkedTracksUnitTest
    : public juce::UnitTest
    {
    public:
        LinkedTracksUnitTest()
        : juce::UnitTest("Mcp Operations Linked Tracks", "Application")
        {
        }

        ~LinkedTracksUnitTest() override = default;

        void runTest() override
        {
            // The test requires the IVE example plugins (VAMP_PATH must contain their directory)
            static auto const identifier = std::string("ive_plugin_example:ivemultivoices");
            auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
            auto const pluginKeys = pluginLoader != nullptr ? pluginLoader->listPlugins(false) : std::vector<std::string>{};
            if(std::find(pluginKeys.cbegin(), pluginKeys.cend(), identifier) == pluginKeys.cend())
            {
                logMessage("The plugin " + identifier + " cannot be found, the test is skipped");
                return;
            }

            Document::Accessor documentAcsr;
            juce::AudioFormatManager audioFormatManager;
            juce::UndoManager undoManager;
            Document::Director documentDir(documentAcsr, audioFormatManager, undoManager);
            Track::PresetList::Accessor presetListAcsr;

            documentDir.startAction();
            auto const [groupResult, groupIdentifier] = documentDir.addGroup(0_z, NotificationType::synchronous);
            documentDir.endAction(ActionState::newTransaction, "New Group");
            expect(groupResult.wasOk());

            auto const getTrackIdentifiers = [](LinkedTracks const& linkedTracks)
            {
                std::vector<juce::String> identifiers;
                for(auto const& track : linkedTracks.tracks)
                {
                    identifiers.push_back(track.identifier);
                }
                return identifiers;
            };

            std::vector<juce::String> trackIdentifiers;
            beginTest("creation");
            {
                PluginTrackCreation creation;
                creation.key = {identifier, "voice1"};
                creation.groupIdentifier = groupIdentifier;
                creation.name = "Track";
                creation.linkedTrackCount = 4_z;
                auto const results = createTracksWithPlugins(documentDir, presetListAcsr, {}, {}, {creation});
                expectEquals(results.size(), 1_z);
                auto const& [result, state] = results.front();
                expect(result.wasOk());
                expectEquals(state.linkedTracks.size(), 3_z);
                expectEquals(state.name, juce::String("Track 1"));
                trackIdentifiers.push_back(state.identifier);
                for(auto const& linkedTrack : state.linkedTracks)
                {
                    trackIdentifiers.push_back(linkedTrack.identifier);
                }
                expectEquals(trackIdentifiers.size(), 4_z);
                expectEquals(Document::Tools::getTrackAcsr(documentAcsr, trackIdentifiers.back()).getAttr<Track::AttrType::name>(), juce::String("Track 4"));
                // The linked tracks are created from the bottom to the top (the first track at the bottom)
                auto layout = Document::Tools::getGroupAcsr(documentAcsr, groupIdentifier).getAttr<Group::AttrType::layout>();
                std::reverse(layout.begin(), layout.end());
                expect(layout == trackIdentifiers);
            }

            beginTest("get linked tracks");
            {
                auto const results = getLinkedTracks(documentAcsr, {trackIdentifiers.at(2_z), "unknown"});
                auto const& [result, linkedTracks] = results.at(trackIdentifiers.at(2_z));
                expect(result.wasOk());
                expectEquals(linkedTracks.count, 4_z);
                expectEquals(linkedTracks.minCount, 1_z);
                expectEquals(linkedTracks.maxCount, 8_z);
                expectEquals(linkedTracks.defaultCount, 3_z);
                expect(getTrackIdentifiers(linkedTracks) == trackIdentifiers);
                expect(std::get<0_z>(results.at("unknown")).failed());
            }

            beginTest("set linked track count");
            {
                // The requested track is removed because it no longer exists
                auto const results = setLinkedTrackCount(documentDir, {{trackIdentifiers.back(), 2_z}});
                auto const& [result, linkedTracks] = results.at(trackIdentifiers.back());
                expect(result.wasOk());
                expectEquals(linkedTracks.count, 2_z);
                expect(getTrackIdentifiers(linkedTracks) == std::vector<juce::String>{trackIdentifiers.at(0_z), trackIdentifiers.at(1_z)});
                expect(!Document::Tools::hasTrackAcsr(documentAcsr, trackIdentifiers.back()));
            }

            beginTest("clamped linked track count");
            {
                auto const results = setLinkedTrackCount(documentDir, {{trackIdentifiers.front(), 100_z}});
                auto const& [result, linkedTracks] = results.at(trackIdentifiers.front());
                expect(result.wasOk());
                expectEquals(linkedTracks.count, 8_z);
                expectEquals(linkedTracks.tracks.size(), 8_z);
                expectEquals(linkedTracks.tracks.front().identifier, trackIdentifiers.front());
            }

            beginTest("color gradient");
            {
                auto const linkedTracks = std::get<1_z>(getLinkedTracks(documentAcsr, {trackIdentifiers.front()}).at(trackIdentifiers.front()));
                expectEquals(linkedTracks.tracks.size(), 8_z);
                std::vector<std::pair<juce::String, TrackGraphics>> graphics;
                for(auto const& track : linkedTracks.tracks)
                {
                    graphics.emplace_back(track.identifier, TrackGraphics{});
                }
                // The colors of a track override the gradient
                graphics.back().second.textColour = juce::Colours::red;
                auto& firstTrackAcsr = Document::Tools::getTrackAcsr(documentAcsr, linkedTracks.tracks.front().identifier);
                auto firstSettings = firstTrackAcsr.getAttr<Track::AttrType::graphicsSettings>();
                firstSettings.colours.foreground = firstSettings.colours.foreground.withAlpha(0.5f);
                firstTrackAcsr.setAttr<Track::AttrType::graphicsSettings>(firstSettings, NotificationType::synchronous);

                TrackColourGradient gradient;
                gradient.colourMap = Track::ColourMap::Viridis;
                // The value tracks don't support the duration color that is ignored
                gradient.properties = {&TrackGraphics::foregroundColour, &TrackGraphics::textColour, &TrackGraphics::durationColour};
                auto const results = setTrackGraphics(documentDir, {}, graphics, gradient);
                auto const colours = getColourMapColours(gradient.colourMap, graphics.size());
                expectEquals(colours.size(), graphics.size());
                expect(colours.front() != colours.back());
                for(auto index = 0_z; index < graphics.size(); ++index)
                {
                    auto const& trackIdentifier = graphics.at(index).first;
                    expect(results.at(trackIdentifier).wasOk());
                    auto const& settings = Document::Tools::getTrackAcsr(documentAcsr, trackIdentifier).getAttr<Track::AttrType::graphicsSettings>();
                    expect(settings.colours.foreground.withAlpha(1.0f) == colours.at(index));
                    expect(settings.colours.text == (index == graphics.size() - 1_z ? juce::Colours::red : colours.at(index)));
                }
                // The alpha of the current colors is preserved
                expectEquals(firstTrackAcsr.getAttr<Track::AttrType::graphicsSettings>().colours.foreground.getAlpha(), firstSettings.colours.foreground.getAlpha());
            }

            beginTest("no linked tracks");
            {
                // A track without voice link is not linked to the other tracks of the multitrack output
                Document::Tools::getTrackAcsr(documentAcsr, trackIdentifiers.front()).setAttr<Track::AttrType::voiceLink>(juce::String{}, NotificationType::synchronous);
                expect(std::get<0_z>(getLinkedTracks(documentAcsr, {trackIdentifiers.front()}).at(trackIdentifiers.front())).failed());
                expect(std::get<0_z>(setLinkedTrackCount(documentDir, {{trackIdentifiers.front(), 2_z}}).at(trackIdentifiers.front())).failed());
            }
        }
    };

    static LinkedTracksUnitTest linkedTracksUnitTest;

    class AliasParametersUnitTest
    : public juce::UnitTest
    {
    public:
        AliasParametersUnitTest()
        : juce::UnitTest("Mcp Operations Alias Parameters", "Application")
        {
        }

        ~AliasParametersUnitTest() override = default;

        void runTest() override
        {
            // The test requires the IVE example plugins (VAMP_PATH must contain their directory)
            static auto const identifier = std::string("ive_plugin_example:ivealiasparameters");
            auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
            auto const pluginKeys = pluginLoader != nullptr ? pluginLoader->listPlugins(false) : std::vector<std::string>{};
            if(std::find(pluginKeys.cbegin(), pluginKeys.cend(), identifier) == pluginKeys.cend())
            {
                logMessage("The plugin " + identifier + " cannot be found, the test is skipped");
                return;
            }

            Document::Accessor documentAcsr;
            juce::AudioFormatManager audioFormatManager;
            juce::UndoManager undoManager;
            Document::Director documentDir(documentAcsr, audioFormatManager, undoManager);
            Track::PresetList::Accessor presetListAcsr;

            documentDir.startAction();
            auto const [groupResult, groupIdentifier] = documentDir.addGroup(0_z, NotificationType::synchronous);
            documentDir.endAction(ActionState::newTransaction, "New Group");
            expect(groupResult.wasOk());

            // The window duration (in ms) is the real parameter and the window size (in samples) is the alias parameter
            static auto constexpr sampleRate = 48000.0;
            juce::String trackIdentifier;
            auto const getParameters = [&]()
            {
                auto const results = getTrackParameters(documentDir, {trackIdentifier});
                auto const& [result, values] = results.at(trackIdentifier);
                expect(result.wasOk());
                return values;
            };

            beginTest("creation");
            {
                PluginTrackCreation creation;
                creation.key = {identifier, "window"};
                creation.groupIdentifier = groupIdentifier;
                auto const results = createTracksWithPlugins(documentDir, presetListAcsr, {}, {}, {creation});
                expectEquals(results.size(), 1_z);
                expect(std::get<0_z>(results.front()).wasOk());
                trackIdentifier = std::get<1_z>(results.front()).identifier;
                expect(Document::Tools::hasTrackAcsr(documentAcsr, trackIdentifier));
                Document::Tools::getTrackAcsr(documentAcsr, trackIdentifier).setAttr<Track::AttrType::sampleRate>(sampleRate, NotificationType::synchronous);
                auto const values = getParameters();
                expect(values.count("size") > 0_z && values.count("duration") > 0_z);
                expectEquals(values.at("duration"), 20.0f);
                expectEquals(values.at("size"), 960.0f);
            }

            beginTest("set the alias parameter");
            {
                auto const results = setTrackParameters(documentDir, {}, {{trackIdentifier, {{"size", 4800.0f}}}});
                auto const& [result, values] = results.at(trackIdentifier);
                expect(result.wasOk());
                // The real parameter modified by the alias parameter is returned
                expect(values.count("size") > 0_z && values.count("duration") > 0_z);
                expectEquals(values.at("size"), 4800.0f);
                expectEquals(values.at("duration"), 100.0f);
                expect(Document::Tools::getTrackAcsr(documentAcsr, trackIdentifier).getAttr<Track::AttrType::state>().parameters.count("size") == 0_z);
            }

            beginTest("get the parameters");
            {
                auto const values = getParameters();
                expect(values.count("size") > 0_z && values.count("duration") > 0_z);
                expectEquals(values.at("size"), 4800.0f);
                expectEquals(values.at("duration"), 100.0f);
            }

            beginTest("set the real parameter");
            {
                auto const results = setTrackParameters(documentDir, {}, {{trackIdentifier, {{"duration", 50.0f}}}});
                auto const& [result, values] = results.at(trackIdentifier);
                expect(result.wasOk());
                expect(values.count("duration") > 0_z && values.count("size") == 0_z);
                expectEquals(getParameters().at("size"), 2400.0f);
            }

            beginTest("set both parameters");
            {
                // The alias parameter is applied after the real parameter
                auto const results = setTrackParameters(documentDir, {}, {{trackIdentifier, {{"duration", 10.0f}, {"size", 960.0f}}}});
                expect(std::get<0_z>(results.at(trackIdentifier)).wasOk());
                auto const values = getParameters();
                expectEquals(values.at("duration"), 20.0f);
                expectEquals(values.at("size"), 960.0f);
            }
        }
    };

    static AliasParametersUnitTest aliasParametersUnitTest;

    class ResultsEditionUnitTest
    : public juce::UnitTest
    {
    public:
        ResultsEditionUnitTest()
        : juce::UnitTest("Mcp Operations Results Edition", "Application")
        {
        }

        ~ResultsEditionUnitTest() override = default;

        void runTest() override
        {
            Document::Accessor documentAcsr;
            juce::AudioFormatManager audioFormatManager;
            juce::UndoManager undoManager;
            Document::Director documentDir(documentAcsr, audioFormatManager, undoManager);

            documentDir.startAction();
            auto const [groupResult, groupIdentifier] = documentDir.addGroup(0_z, NotificationType::synchronous);
            // The track identifier is captured by the lambdas below (structured bindings cannot be captured with some compilers)
            auto const trackCreation = documentDir.addTrack(groupIdentifier, 0_z, NotificationType::synchronous);
            auto const trackIdentifier = std::get<1_z>(trackCreation);
            documentDir.endAction(ActionState::newTransaction, "New Track");
            expect(groupResult.wasOk() && std::get<0_z>(trackCreation).wasOk());
            auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, trackIdentifier);

            using Times = std::vector<double>;
            auto const setTimes = [&](Times const& times)
            {
                std::vector<Track::Results::Points> channels(1_z);
                for(auto const time : times)
                {
                    channels.front().push_back({});
                    std::get<0_z>(channels.front().back()) = time;
                    std::get<2_z>(channels.front().back()) = 1.0f;
                }
                trackAcsr.setAttr<Track::AttrType::results>(Track::Results(std::move(channels)), NotificationType::synchronous);
            };
            auto const getTimes = [&]()
            {
                Times times;
                auto const& results = trackAcsr.getAttr<Track::AttrType::results>();
                auto const access = results.getReadAccess();
                auto const points = results.getPoints();
                if(static_cast<bool>(access) && points != nullptr && !points->empty())
                {
                    for(auto const& point : points->front())
                    {
                        times.push_back(std::get<0_z>(point));
                    }
                }
                return times;
            };
            auto const setResults = [&](std::string const& csv, std::optional<juce::Range<double>> const& timeRange)
            {
                ResultsModification modification;
                modification.results = csv;
                modification.timeRange = timeRange;
                return setTrackResultsRaw(documentDir, {{trackIdentifier, modification}}, ',').at(trackIdentifier);
            };

            beginTest("set results at their own times");
            {
                // The time range excludes its end
                setTimes({0.0, 1.0, 2.0, 3.0, 4.0});
                expect(setResults("1.5,0,2\n2.5,0,3\n", juce::Range<double>(1.0, 3.0)).wasOk());
                expect(getTimes() == Times{0.0, 1.5, 2.5, 3.0, 4.0});
                undoManager.undo();
                expect(getTimes() == Times{0.0, 1.0, 2.0, 3.0, 4.0});
                expect(setResults("0.5,0,2\n", std::nullopt).wasOk());
                expect(getTimes() == Times{0.5});
            }

            beginTest("set invalid results");
            {
                setTimes({0.0, 1.0, 2.0, 3.0, 4.0});
                expect(setResults("0.5,0,2\n", juce::Range<double>(1.0, 3.0)).failed());
                expect(setResults("3,0,2\n", juce::Range<double>(1.0, 3.0)).failed());
                expect(setResults("2.5,0,2\n1.5,0,2\n", juce::Range<double>(1.0, 3.0)).failed());
                expect(setResults("1.5,0,label\n", juce::Range<double>(1.0, 3.0)).failed());
                expect(setResults("1.5,0,2\n\n1.5,0,2\n", juce::Range<double>(1.0, 3.0)).failed());
                expect(setResults("", juce::Range<double>(1.0, 3.0)).failed());
                expect(getTimes() == Times{0.0, 1.0, 2.0, 3.0, 4.0});
            }

            beginTest("set breaks");
            {
                auto const getBreakCount = [&]()
                {
                    auto const snapshot = std::get<1_z>(getTrackResultsSnapshots(documentAcsr, {trackIdentifier}).at(trackIdentifier));
                    auto const summary = std::get<1_z>(summarizeResults(snapshot, {}));
                    return summary.channels.empty() || summary.channels.front().ranges.empty() ? std::optional<size_t>{} : summary.channels.front().ranges.front().breakCount;
                };
                setTimes({0.0, 1.0, 2.0, 3.0, 4.0});
                expect(getBreakCount() == 0_z);
                // A single break (parsed as a marker without label) and a break with values
                expect(setResults("1.5,0,\n", juce::Range<double>(1.5, 1.5)).wasOk());
                expect(setResults("3,0,2\n3.5,0,\n", juce::Range<double>(3.0, 4.0)).wasOk());
                expect(getTimes() == Times{0.0, 1.0, 1.5, 2.0, 3.0, 3.5, 4.0});
                expect(getBreakCount() == 2_z);
            }

            beginTest("erase results");
            {
                setTimes({0.0, 1.0, 2.0, 3.0, 4.0});
                // The time ranges exclude their end
                auto const results = eraseTrackResults(documentDir, {trackIdentifier, "unknown"}, {{1.0, 3.0}, {3.5, 5.0}}, {});
                auto const& [result, count] = results.at(trackIdentifier);
                expect(result.wasOk());
                expectEquals(count, 3_z);
                expect(getTimes() == Times{0.0, 3.0});
                expect(std::get<0_z>(results.at("unknown")).failed());
                undoManager.undo();
                expect(getTimes() == Times{0.0, 1.0, 2.0, 3.0, 4.0});
            }

            beginTest("erase a single result");
            {
                setTimes({0.0, 1.0, 2.0});
                auto const results = eraseTrackResults(documentDir, {trackIdentifier}, {{1.0, 1.0}}, {});
                expectEquals(std::get<1_z>(results.at(trackIdentifier)), 1_z);
                expect(getTimes() == Times{0.0, 2.0});
            }

            beginTest("erase a time range without results");
            {
                // The result at time zero must be preserved
                setTimes({0.0, 1.0, 2.0});
                auto const results = eraseTrackResults(documentDir, {trackIdentifier}, {{0.2, 0.8}}, {});
                auto const& [result, count] = results.at(trackIdentifier);
                expect(result.wasOk());
                expectEquals(count, 0_z);
                expect(getTimes() == Times{0.0, 1.0, 2.0});
            }

            beginTest("get results in time ranges");
            {
                // As the erased results, the time ranges exclude their end
                setTimes({0.0, 1.0, 2.0, 3.0, 4.0});
                auto const countRows = [&](std::vector<juce::Range<double>> const& timeRanges)
                {
                    ResultsSelection selection;
                    selection.timeRanges = timeRanges;
                    auto const [result, csv] = getTrackResultsRaw(documentAcsr, {trackIdentifier}, selection, Track::Exporter::CsvHeaderType::none, ',').at(trackIdentifier);
                    expect(result.wasOk());
                    auto const lines = juce::StringArray::fromLines(juce::String(csv));
                    return static_cast<size_t>(std::count_if(lines.begin(), lines.end(), [](juce::String const& line)
                                                             {
                                                                 return line.isNotEmpty() && juce::CharacterFunctions::isDigit(line[0]);
                                                             }));
                };
                auto const countResults = [&](std::vector<juce::Range<double>> const& timeRanges)
                {
                    ResultsSelection selection;
                    selection.timeRanges = timeRanges;
                    auto const snapshot = std::get<1_z>(getTrackResultsSnapshots(documentAcsr, {trackIdentifier}).at(trackIdentifier));
                    return std::get<1_z>(Operations::countResults(snapshot, selection));
                };
                expectEquals(countRows({{1.0, 3.0}}), 2_z);
                expectEquals(countResults({{1.0, 3.0}}), 2_z);
                expectEquals(countRows({{1.0, 1.0}}), 1_z);
                expectEquals(countResults({{1.0, 1.0}}), 1_z);
                expectEquals(countRows({{0.2, 0.8}}), 0_z);
                expectEquals(countResults({{0.2, 0.8}}), 0_z);
                expectEquals(countRows({}), 5_z);
                expectEquals(countResults({}), 5_z);
            }

            beginTest("erase an invalid channel");
            {
                setTimes({0.0, 1.0, 2.0});
                expect(std::get<0_z>(eraseTrackResults(documentDir, {trackIdentifier}, {{0.0, 2.0}}, {1_z}).at(trackIdentifier)).failed());
                expect(getTimes() == Times{0.0, 1.0, 2.0});
            }
        }
    };

    static ResultsEditionUnitTest resultsEditionUnitTest;

    class TrackOrderUnitTest
    : public juce::UnitTest
    {
    public:
        TrackOrderUnitTest()
        : juce::UnitTest("Mcp Operations Track Order", "Application")
        {
        }

        ~TrackOrderUnitTest() override = default;

        void runTest() override
        {
            // The test requires the IVE example plugins (VAMP_PATH must contain their directory)
            static auto const identifier = std::string("ive_plugin_example:ivealiasparameters");
            auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
            auto const pluginKeys = pluginLoader != nullptr ? pluginLoader->listPlugins(false) : std::vector<std::string>{};
            if(std::find(pluginKeys.cbegin(), pluginKeys.cend(), identifier) == pluginKeys.cend())
            {
                logMessage("The plugin " + identifier + " cannot be found, the test is skipped");
                return;
            }

            Document::Accessor documentAcsr;
            juce::AudioFormatManager audioFormatManager;
            juce::UndoManager undoManager;
            Document::Director documentDir(documentAcsr, audioFormatManager, undoManager);
            Track::PresetList::Accessor presetListAcsr;

            documentDir.startAction();
            // The group identifier is captured by the lambdas below (structured bindings cannot be captured with some compilers)
            auto const groupCreation = documentDir.addGroup(0_z, NotificationType::synchronous);
            auto const groupIdentifier = std::get<1_z>(groupCreation);
            documentDir.endAction(ActionState::newTransaction, "New Group");
            expect(std::get<0_z>(groupCreation).wasOk());

            auto const getLayout = [&](juce::String const& group)
            {
                return Document::Tools::getGroupAcsr(documentAcsr, group).getAttr<Group::AttrType::layout>();
            };

            // The tracks are identified by their names in the expectations
            std::map<juce::String, juce::String> tracks;
            auto const getTracks = [&](std::vector<juce::String> const& names)
            {
                std::vector<juce::String> identifiers;
                for(auto const& name : names)
                {
                    identifiers.push_back(tracks.count(name) > 0_z ? tracks.at(name) : juce::String{});
                }
                return identifiers;
            };
            // The value tracks use the output of the random point plugin and the vector tracks use the output of the alias
            // parameters plugin (two bins)
            auto const createTracks = [&](std::vector<std::tuple<juce::String, bool>> const& requests, juce::String const& requestedGroupIdentifier = {})
            {
                std::vector<PluginTrackCreation> creations;
                for(auto const& [name, isVector] : requests)
                {
                    PluginTrackCreation creation;
                    creation.key = isVector ? Plugin::Key{identifier, "window"} : Plugin::Key{"ive_plugin_example:iverandompoint", "points"};
                    creation.groupIdentifier = requestedGroupIdentifier;
                    creation.name = name;
                    creations.push_back(std::move(creation));
                }
                std::vector<juce::Result> results;
                for(auto const& [result, state] : createTracksWithPlugins(documentDir, presetListAcsr, {}, {}, creations))
                {
                    results.push_back(result);
                    if(result.wasOk())
                    {
                        tracks[state.name] = state.identifier;
                    }
                }
                return results;
            };
            auto const allSucceeded = [](std::vector<juce::Result> const& results)
            {
                return std::all_of(results.cbegin(), results.cend(), [](auto const& result)
                                   {
                                       return result.wasOk();
                                   });
            };

            beginTest("creation positions");
            {
                // Each new track is inserted at the top of the group
                expect(allSucceeded(createTracks({{"A", false}, {"B", false}}, groupIdentifier)));
                expect(getLayout(groupIdentifier) == getTracks({"B", "A"}));
                // Each new vector track is inserted at the bottom of the group
                expect(allSucceeded(createTracks({{"V", true}, {"W", true}}, groupIdentifier)));
                expect(getLayout(groupIdentifier) == getTracks({"B", "A", "V", "W"}));
                // The tracks are created in the top group by default
                expect(allSucceeded(createTracks({{"C", false}})));
                expect(getLayout(groupIdentifier) == getTracks({"C", "B", "A", "V", "W"}));
            }

            juce::String otherGroupIdentifier;
            beginTest("group placements");
            {
                auto const createGroup = [&](Placement placement, juce::String const& anchorIdentifier)
                {
                    GroupCreation creation;
                    creation.placement = placement;
                    creation.anchorIdentifier = anchorIdentifier;
                    return creation;
                };
                auto const results = createGroups(documentDir, {createGroup(Placement::top, {}), createGroup(Placement::top, {}), createGroup(Placement::below, groupIdentifier), createGroup(Placement::above, "unknown")});
                expectEquals(results.size(), 4_z);
                expect(std::get<0_z>(results.at(0_z)).wasOk() && std::get<0_z>(results.at(1_z)).wasOk() && std::get<0_z>(results.at(2_z)).wasOk());
                expect(std::get<0_z>(results.at(3_z)).failed());
                auto const& first = std::get<1_z>(results.at(0_z)).identifier;
                auto const& second = std::get<1_z>(results.at(1_z)).identifier;
                auto const& third = std::get<1_z>(results.at(2_z)).identifier;
                expect(documentAcsr.getAttr<Document::AttrType::layout>() == std::vector<juce::String>{first, second, groupIdentifier, third});
                // The groups are appended to the document by default
                expect(std::get<0_z>(createGroups(documentDir, {GroupCreation{}}).front()).wasOk());
                expectEquals(documentAcsr.getAttr<Document::AttrType::layout>().size(), 5_z);
                expect(documentAcsr.getAttr<Document::AttrType::layout>().at(3_z) == third);
                otherGroupIdentifier = third;
            }

            beginTest("invalid creation");
            {
                PluginTrackCreation creation;
                creation.key = {identifier, "window"};
                creation.groupIdentifier = "unknown";
                expect(std::get<0_z>(createTracksWithPlugins(documentDir, presetListAcsr, {}, {}, {creation}).front()).failed());
                expect(getLayout(groupIdentifier) == getTracks({"C", "B", "A", "V", "W"}));
                expect(getLayout(otherGroupIdentifier).empty());
            }

            beginTest("moves");
            {
                auto const createMove = [&](juce::String const& name, Placement placement, juce::String const& anchorName, juce::String const& requestedGroupIdentifier = {})
                {
                    TrackMove move;
                    move.identifier = tracks.at(name);
                    move.groupIdentifier = requestedGroupIdentifier;
                    move.placement = placement;
                    move.anchorIdentifier = anchorName.isEmpty() ? juce::String{} : tracks.at(anchorName);
                    return move;
                };
                // The tracks are moved within their group by default and the tracks with the same placement are placed
                // in the order of the requests from the top to the bottom
                auto results = moveTracks(documentDir, {createMove("V", Placement::top, ""), createMove("W", Placement::top, "")});
                expect(results.at(tracks.at("V")).wasOk() && results.at(tracks.at("W")).wasOk());
                expect(getLayout(groupIdentifier) == getTracks({"V", "W", "C", "B", "A"}));
                expect(moveTracks(documentDir, {createMove("A", Placement::below, "W")}).at(tracks.at("A")).wasOk());
                expect(getLayout(groupIdentifier) == getTracks({"V", "W", "A", "C", "B"}));
                expect(moveTracks(documentDir, {createMove("A", Placement::above, "V")}).at(tracks.at("A")).wasOk());
                expect(getLayout(groupIdentifier) == getTracks({"A", "V", "W", "C", "B"}));
                // The tracks can be moved to another group
                results = moveTracks(documentDir, {createMove("B", Placement::bottom, "", otherGroupIdentifier), createMove("C", Placement::below, "B")});
                expect(results.at(tracks.at("B")).wasOk() && results.at(tracks.at("C")).wasOk());
                expect(getLayout(groupIdentifier) == getTracks({"A", "V", "W"}));
                expect(getLayout(otherGroupIdentifier) == getTracks({"B", "C"}));
                // The invalid moves are ignored
                expect(moveTracks(documentDir, {createMove("A", Placement::above, "A")}).at(tracks.at("A")).failed());
                expect(moveTracks(documentDir, {createMove("A", Placement::above, "B", groupIdentifier)}).at(tracks.at("A")).failed());
                TrackMove unknownMove;
                unknownMove.identifier = "unknown";
                expect(moveTracks(documentDir, {unknownMove}).at("unknown").failed());
                expect(getLayout(groupIdentifier) == getTracks({"A", "V", "W"}));
            }

            beginTest("document state");
            {
                auto const groups = getDocumentState(documentAcsr);
                auto const it = std::find_if(groups.cbegin(), groups.cend(), [&](auto const& group)
                                             {
                                                 return group.identifier == groupIdentifier;
                                             });
                expect(it != groups.cend());
                std::vector<juce::String> identifiers;
                for(auto const& track : it->tracks)
                {
                    identifiers.push_back(track.identifier);
                }
                expect(identifiers == getLayout(groupIdentifier));
                expectEquals(it->tracks.front().name, juce::String("A"));
            }

            beginTest("group layouts");
            {
                auto const getResults = getGroupLayouts(documentAcsr, {groupIdentifier});
                expect(std::get<0_z>(getResults.at(groupIdentifier)).wasOk());
                expect(std::get<1_z>(getResults.at(groupIdentifier)) == getLayout(groupIdentifier));

                auto const layout = getTracks({"W", "A", "V"});
                GroupPropertiesModification modification;
                modification.name = "Group";
                modification.layout = layout;
                auto const setResults = setGroupProperties(documentDir, {{groupIdentifier, modification}});
                expect(std::get<0_z>(setResults.at(groupIdentifier)).wasOk());
                expect(std::get<1_z>(setResults.at(groupIdentifier)).layout == std::make_optional(layout));
                expect(!std::get<1_z>(setResults.at(groupIdentifier)).verticalZoom.has_value());
                expect(getLayout(groupIdentifier) == layout);
                expect(std::get<1_z>(getGroupLayouts(documentAcsr, {groupIdentifier}).at(groupIdentifier)) == layout);
                expectEquals(std::get<1_z>(getGroupNames(documentAcsr, {groupIdentifier}).at(groupIdentifier)), juce::String("Group"));

                // A group is not modified if one of its properties is invalid
                modification.name = "Other";
                modification.layout = std::vector<juce::String>{layout.front()};
                expect(std::get<0_z>(setGroupProperties(documentDir, {{groupIdentifier, modification}}).at(groupIdentifier)).failed());
                expect(getLayout(groupIdentifier) == layout);
                expectEquals(std::get<1_z>(getGroupNames(documentAcsr, {groupIdentifier}).at(groupIdentifier)), juce::String("Group"));
                expect(std::get<0_z>(setGroupProperties(documentDir, {{"unknown", modification}}).at("unknown")).failed());
            }

            beginTest("track properties");
            {
                auto const track = getTracks({"A"}).front();
                TrackPropertiesModification modification;
                modification.name = "Renamed";
                modification.visibleInGroup = false;
                modification.extraThresholds = std::vector<std::optional<float>>{};
                auto const results = setTrackProperties(documentDir, {{track, modification}});
                expect(std::get<0_z>(results.at(track)).wasOk());
                auto const& state = std::get<1_z>(results.at(track));
                expect(state.name == std::make_optional(juce::String("Renamed")));
                expect(state.visibleInGroup == std::make_optional(false));
                expect(!state.verticalZoom.has_value() && !state.valueRange.has_value());
                expectEquals(std::get<1_z>(getTrackNames(documentAcsr, {track}).at(track)), juce::String("Renamed"));
                expect(!std::get<1_z>(getTrackIsVisibleInGroup(documentAcsr, {track}).at(track)));

                // A track is not modified if one of its properties is invalid (an empty range)
                TrackPropertiesModification invalidModification;
                invalidModification.name = "Invalid";
                invalidModification.valueRange = juce::Range<double>{1.0, 1.0};
                expect(std::get<0_z>(setTrackProperties(documentDir, {{track, invalidModification}}).at(track)).failed());
                expectEquals(std::get<1_z>(getTrackNames(documentAcsr, {track}).at(track)), juce::String("Renamed"));
                expect(std::get<0_z>(setTrackProperties(documentDir, {{"unknown", modification}}).at("unknown")).failed());
            }
        }
    };

    static TrackOrderUnitTest trackOrderUnitTest;
} // namespace Application::Neuralyzer::Mcp::Operations

ANALYSE_FILE_END
