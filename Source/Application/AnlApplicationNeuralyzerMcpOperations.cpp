#include "AnlApplicationNeuralyzerMcpOperations.h"
#include "../Document/AnlDocumentSelection.h"
#include "../Document/AnlDocumentTools.h"
#include "../Group/AnlGroupTools.h"
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
        auto const& layout = groupAcsr.getAttr<Group::AttrType::layout>();
        for(auto it = layout.crbegin(); it != layout.crend(); ++it)
        {
            auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, *it);
            group.tracks.push_back({*it, trackAcsr.getAttr<Track::AttrType::name>()});
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

std::map<juce::String, juce::Result> Application::Neuralyzer::Mcp::Operations::setGroupNames(Document::Director& documentDir, std::map<juce::String, juce::String> const& names)
{
    auto& documentAcsr = documentDir.getAccessor();
    return performDocumentAction(documentDir, names, juce::translate("Change group name (agent)"), [&](juce::String const& identifier, juce::String const& name)
                                 {
                                     if(!Document::Tools::hasGroupAcsr(documentAcsr, identifier))
                                     {
                                         return createGroupNotFoundError(identifier);
                                     }
                                     auto& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, identifier);
                                     groupAcsr.setAttr<Group::AttrType::name>(name, NotificationType::synchronous);
                                     return juce::Result::ok();
                                 });
}

std::map<juce::String, std::tuple<juce::Result, std::vector<juce::String>>> Application::Neuralyzer::Mcp::Operations::getGroupLayouts(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    return getGroupsValues(documentAcsr, identifiers, [](Group::Accessor const& groupAcsr)
                           {
                               // The group layout is ordered from the top to the bottom
                               auto const& layout = groupAcsr.getAttr<Group::AttrType::layout>();
                               return std::make_tuple(juce::Result::ok(), std::vector<juce::String>(layout.crbegin(), layout.crend()));
                           });
}

std::map<juce::String, juce::Result> Application::Neuralyzer::Mcp::Operations::setGroupLayouts(Document::Director& documentDir, std::map<juce::String, std::vector<juce::String>> const& layouts)
{
    auto& documentAcsr = documentDir.getAccessor();
    return performDocumentAction(documentDir, layouts, juce::translate("Change group layout (agent)"), [&](juce::String const& identifier, std::vector<juce::String> const& bottomToTopLayout)
                                 {
                                     if(!Document::Tools::hasGroupAcsr(documentAcsr, identifier))
                                     {
                                         return createGroupNotFoundError(identifier);
                                     }
                                     auto& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, identifier);
                                     // The group layout is ordered from the top to the bottom
                                     std::vector<juce::String> layout(bottomToTopLayout.crbegin(), bottomToTopLayout.crend());
                                     auto const currentLayout = groupAcsr.getAttr<Group::AttrType::layout>();
                                     if(std::set<juce::String>(layout.cbegin(), layout.cend()) != std::set<juce::String>(currentLayout.cbegin(), currentLayout.cend()) || layout.size() != currentLayout.size())
                                     {
                                         return juce::Result::fail("The layout must contain exactly the tracks of the group (only the order can be changed).");
                                     }
                                     groupAcsr.setAttr<Group::AttrType::layout>(layout, NotificationType::synchronous);
                                     return juce::Result::ok();
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

std::map<juce::String, std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::ZoomRanges>> Application::Neuralyzer::Mcp::Operations::setGroupVerticalZoom(Document::Accessor& documentAcsr, std::map<juce::String, juce::Range<double>> const& visibleRanges)
{
    std::map<juce::String, std::tuple<juce::Result, ZoomRanges>> zooms;
    for(auto const& [identifier, visibleRange] : visibleRanges)
    {
        auto const [result, zoomAcsr] = getGroupVerticalZoomAcsr(documentAcsr, identifier);
        zooms.insert_or_assign(identifier, result.wasOk() ? setVisibleRange(zoomAcsr.value().get(), visibleRange) : std::make_tuple(result, ZoomRanges{}));
    }
    return zooms;
}

std::vector<std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::GroupState>> Application::Neuralyzer::Mcp::Operations::createGroups(Document::Director& documentDir, std::vector<GroupCreation> const& groups)
{
    auto& documentAcsr = documentDir.getAccessor();
    ScopedDocumentAction actionGuard(documentDir);
    std::vector<std::tuple<juce::Result, GroupState>> results;
    auto hasChanged = false;
    for(auto const& group : groups)
    {
        auto const position = group.position.value_or(documentAcsr.getNumAcsrs<Document::AcsrType::groups>());
        auto const [result, identifier] = documentDir.addGroup(position, NotificationType::synchronous);
        if(result.failed())
        {
            results.emplace_back(juce::Result::fail(juce::String(R"(Failed to create group "NAME": REASON.)").replace("NAME", group.name).replace("REASON", result.getErrorMessage())), GroupState{});
            continue;
        }
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

std::map<juce::String, juce::Result> Application::Neuralyzer::Mcp::Operations::setTrackGraphics(Document::Director& documentDir, TrackGraphics const& defaultGraphics, std::map<juce::String, TrackGraphics> const& graphics)
{
    auto& documentAcsr = documentDir.getAccessor();
    return performDocumentAction(documentDir, graphics, juce::translate("Change track graphics (agent)"), [&](juce::String const& identifier, TrackGraphics const& requestedGraphics)
                                 {
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
                                     auto const trackGraphics = mergeGraphics(requestedGraphics, defaultGraphics, supported);

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
    // Applies the default parameter values (ignored if the track doesn't have the parameter) and the parameter values
    // of the track (that must exist) clamped to the ranges of the parameters
    static std::tuple<juce::Result, std::map<std::string, float>> applyParameters(Track::Accessor& trackAcsr, std::map<std::string, float> const& defaultValues, std::map<std::string, float> const& values)
    {
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
        auto state = trackAcsr.getAttr<Track::AttrType::state>();
        std::map<std::string, float> appliedValues;
        for(auto const& [parameterIdentifier, value] : mergedValues)
        {
            auto const it = findParameter(parameterIdentifier);
            if(it != parameters.cend())
            {
                auto const clampedValue = std::clamp(value, it->minValue, it->maxValue);
                state.parameters[parameterIdentifier] = clampedValue;
                appliedValues[parameterIdentifier] = clampedValue;
            }
        }
        if(!appliedValues.empty())
        {
            trackAcsr.setAttr<Track::AttrType::state>(state, NotificationType::synchronous);
        }
        return {juce::Result::ok(), std::move(appliedValues)};
    }

    // Gets the identifier of the group and the position in the group layout (from the top) of a new track
    static std::tuple<juce::Result, juce::String, size_t> getNewTrackPosition(Document::Accessor const& documentAcsr, std::map<juce::String, juce::String> const& references, juce::String const& groupIdentifier, std::optional<size_t> const& position)
    {
        auto const& layout = documentAcsr.getAttr<Document::AttrType::layout>();
        if(layout.empty())
        {
            return {juce::Result::fail("The document has no group. Create a group first."), {}, 0_z};
        }
        auto const requestedIdentifier = groupIdentifier.isEmpty() ? layout.front() : groupIdentifier;
        auto const identifier = references.count(requestedIdentifier) > 0_z ? references.at(requestedIdentifier) : requestedIdentifier;
        if(!Document::Tools::hasGroupAcsr(documentAcsr, identifier))
        {
            return {createGroupNotFoundError(identifier), {}, 0_z};
        }
        // The position is from the bottom and the group layout is ordered from the top to the bottom
        auto const numTracks = Document::Tools::getGroupAcsr(documentAcsr, identifier).getAttr<Group::AttrType::layout>().size();
        return {juce::Result::ok(), identifier, numTracks - std::min(position.value_or(numTracks), numTracks)};
    }

    // Creates tracks in order within a single undoable action that is committed if at least one track has been created
    // The function creates a track and returns the result of the operation and the identifier of the track
    template <typename Creation, typename Function>
    static std::vector<std::tuple<juce::Result, TrackState>> createTracks(Document::Director& documentDir, std::vector<Creation> const& tracks, juce::String const& actionName, Function&& function)
    {
        auto& documentAcsr = documentDir.getAccessor();
        // Ensures that the identifiers of the groups are unique and valid before adding tracks
        auto const references = documentDir.sanitize(NotificationType::synchronous);
        ScopedDocumentAction actionGuard(documentDir);
        std::vector<std::tuple<juce::Result, TrackState>> results;
        auto hasChanged = false;
        for(auto const& track : tracks)
        {
            auto const [positionResult, groupIdentifier, position] = getNewTrackPosition(documentAcsr, references, track.groupIdentifier, track.position);
            if(positionResult.failed())
            {
                results.emplace_back(positionResult, TrackState{});
                continue;
            }
            std::tuple<juce::Result, juce::String> const creation = function(track, groupIdentifier, position);
            auto const& [result, identifier] = creation;
            if(result.failed())
            {
                results.emplace_back(result, TrackState{});
                continue;
            }
            results.emplace_back(juce::Result::ok(), TrackState{identifier, Document::Tools::getTrackAcsr(documentAcsr, identifier).getAttr<Track::AttrType::name>()});
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
                                     auto result = applyParameters(Document::Tools::getTrackAcsr(documentAcsr, identifier), defaultValues, trackValues);
                                     if(std::get<0>(result).wasOk() && std::get<1>(result).empty())
                                     {
                                         return result_t{juce::Result::fail("The track has none of the parameters."), {}};
                                     }
                                     return result;
                                 });
}

std::vector<std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::TrackState>> Application::Neuralyzer::Mcp::Operations::createTracksWithPlugins(Document::Director& documentDir, Track::PresetList::Accessor const& presetListAcsr, Track::GraphicsSettings const& globalGraphicPreset, std::map<std::string, float> const& defaultParameters, std::vector<PluginTrackCreation> const& tracks)
{
    auto& documentAcsr = documentDir.getAccessor();
    return createTracks(documentDir, tracks, juce::translate("Create tracks (agent)"), [&](PluginTrackCreation const& track, juce::String const& groupIdentifier, size_t position)
                        {
                            auto const& key = track.key;
                            if(key.identifier.find(':') == std::string::npos)
                            {
                                return std::make_tuple(juce::Result::fail(R"(The plugin identifier must be in the form 'maker:plugin'. Consider calling the tools 'search_docs' and 'get_vamp_plugin_descriptions' first to get the correct values.)"), juce::String{});
                            }
                            auto const [result, identifier] = Tools::addPluginTrack(documentDir, presetListAcsr, globalGraphicPreset, groupIdentifier, position, key);
                            if(result.failed())
                            {
                                return std::make_tuple(juce::Result::fail(juce::String(R"(Failed to create the track with the plugin {"identifier":"PLUGINID", "feature":"PLUGINFEATURE"}: REASON Consider calling the tools 'search_docs' and 'get_vamp_plugin_descriptions' to get the correct plugin identifier and feature.)").replace("PLUGINID", key.identifier).replace("PLUGINFEATURE", key.feature).replace("REASON", result.getErrorMessage())), juce::String{});
                            }
                            auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                            auto const parametersResult = std::get<0>(applyParameters(trackAcsr, defaultParameters, track.parameters));
                            if(parametersResult.failed())
                            {
                                // The track is removed because one of its parameters is invalid
                                auto const removeResult = documentDir.removeTrack(identifier, NotificationType::synchronous);
                                MiscWeakAssert(removeResult.wasOk());
                                return std::make_tuple(parametersResult, juce::String{});
                            }
                            if(track.name.has_value())
                            {
                                trackAcsr.setAttr<Track::AttrType::name>(track.name.value(), NotificationType::synchronous);
                            }
                            return std::make_tuple(juce::Result::ok(), identifier);
                        });
}

std::vector<std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::TrackState>> Application::Neuralyzer::Mcp::Operations::createTracksWithFiles(Document::Director& documentDir, Track::GraphicsSettings const& globalGraphicPreset, std::vector<FileTrackCreation> const& tracks)
{
    return createTracks(documentDir, tracks, juce::translate("Create tracks from files (agent)"), [&](FileTrackCreation const& track, juce::String const& groupIdentifier, size_t position)
                        {
                            if(!track.file.existsAsFile() || !track.file.hasReadAccess())
                            {
                                return std::make_tuple(juce::Result::fail(juce::String("The file 'FILEPATH' does not exist or is not readable.").replace("FILEPATH", track.file.getFullPathName())), juce::String{});
                            }
                            return Tools::addFileTrack(documentDir, globalGraphicPreset, groupIdentifier, position, track.file);
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

std::map<juce::String, juce::Result> Application::Neuralyzer::Mcp::Operations::setTrackExtraThresholds(Document::Director& documentDir, std::map<juce::String, std::vector<std::optional<float>>> const& thresholds)
{
    auto& documentAcsr = documentDir.getAccessor();
    return performDocumentAction(documentDir, thresholds, juce::translate("Set track extra thresholds (agent)"), [&](juce::String const& identifier, std::vector<std::optional<float>> const& trackThresholds)
                                 {
                                     if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                                     {
                                         return createTrackNotFoundError(identifier);
                                     }
                                     auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                                     trackAcsr.setAttr<Track::AttrType::extraThresholds>(trackThresholds, NotificationType::synchronous);
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

    // Calls the function for each result of a channel in a time range that passes the extra thresholds (if applied)
    template <typename ResultType, typename Function>
    static void forEachResult(std::vector<ResultType> const& channelResults, juce::Range<double> const& timeRange, std::vector<std::optional<float>> const& extraThresholds, bool applyExtraThresholds, Function&& function)
    {
        auto it = std::lower_bound(channelResults.cbegin(), channelResults.cend(), timeRange.getStart(), Track::Result::lower_cmp<ResultType>);
        while(it != channelResults.cend() && std::get<0_z>(*it) <= timeRange.getEnd())
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
                                    auto const result = exportRange(timeRange);
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
    return performDocumentAction(documentDir, modifications, juce::translate("Set track results (agent)"), [&](juce::String const& identifier, ResultsModification const& modification)
                                 {
                                     if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                                     {
                                         return createTrackNotFoundError(identifier);
                                     }
                                     std::istringstream stream(modification.results);
                                     std::atomic<bool> shouldAbort{false};
                                     std::atomic<float> advancement{0.0f};
                                     auto const loadedResults = Track::Loader::loadFromCsv(stream, separator, false, '\n', false, shouldAbort, advancement);
                                     if(auto const* error = std::get_if<juce::String>(&loadedResults))
                                     {
                                         return juce::Result::fail("Failed to parse the CSV results: " + *error);
                                     }
                                     auto const& data = std::get<Track::Results>(loadedResults);
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
                                     auto const trackAccessorFn = documentDir.getSafeTrackAccessorFn(identifier);
                                     visitResults(data, [&](auto const& channels)
                                                  {
                                                      for(auto channel = 0_z; channel < channels.size(); ++channel)
                                                      {
                                                          undoManager.perform(std::make_unique<Track::Result::Modifier::ActionErase>(trackAccessorFn, channel, timeRange, false, timeRange.getEnd()).release());
                                                          undoManager.perform(std::make_unique<Track::Result::Modifier::ActionPaste>(trackAccessorFn, channel, channels.at(channel), timeRange.getStart(), false, timeRange.getEnd()).release());
                                                      }
                                                  });
                                     return juce::Result::ok();
                                 });
}

std::tuple<juce::Result, std::vector<juce::File>> Application::Neuralyzer::Mcp::Operations::saveTrackResults(Document::Accessor const& documentAcsr, std::set<juce::String> const& identifiers, Document::Exporter::Options const& options, juce::File const& directory)
{
    if(identifiers.empty())
    {
        return {juce::Result::fail("There is no track or group to export."), {}};
    }
    for(auto const& identifier : identifiers)
    {
        if(!Document::Tools::hasItem(documentAcsr, identifier))
        {
            return {juce::Result::fail(juce::String(R"(The item (group or track) "ITEMID" doesn't exist.)").replace("ITEMID", identifier)), {}};
        }
    }
    if(!options.isValid())
    {
        return {juce::Result::fail("The SDIF frame and matrix signatures must be 4-character signatures without '?'."), {}};
    }
    auto const directoryResult = directory.createDirectory();
    if(directoryResult.failed())
    {
        return {directoryResult, {}};
    }
    std::atomic<bool> shouldAbort{false};
    auto const exportResult = Document::Exporter::exportTo(documentAcsr, directory, {}, {}, "", identifiers, options, shouldAbort);
    if(exportResult.failed())
    {
        return {exportResult, {}};
    }
    auto const childFiles = directory.findChildFiles(juce::File::findFiles, false, "*");
    std::vector<juce::File> files(childFiles.begin(), childFiles.end());
    if(files.empty())
    {
        return {juce::Result::fail("The export completed successfully, but no files were created."), {}};
    }
    return {juce::Result::ok(), std::move(files)};
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

std::map<juce::String, std::tuple<juce::Result, std::map<std::string, float>>> Application::Neuralyzer::Mcp::Operations::getTrackParameters(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    return getTracksValues(documentAcsr, identifiers, [](Track::Accessor const& trackAcsr)
                           {
                               return std::make_tuple(juce::Result::ok(), trackAcsr.getAttr<Track::AttrType::state>().parameters);
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

std::map<juce::String, std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::ZoomRanges>> Application::Neuralyzer::Mcp::Operations::setTrackVerticalZoom(Document::Accessor& documentAcsr, std::map<juce::String, juce::Range<double>> const& visibleRanges)
{
    std::map<juce::String, std::tuple<juce::Result, ZoomRanges>> zooms;
    for(auto const& [identifier, visibleRange] : visibleRanges)
    {
        if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
        {
            zooms.insert_or_assign(identifier, std::make_tuple(createTrackNotFoundError(identifier), ZoomRanges{}));
            continue;
        }
        auto const [result, zoomAcsr] = getTrackVerticalZoomAcsr(Document::Tools::getTrackAcsr(documentAcsr, identifier));
        zooms.insert_or_assign(identifier, result.wasOk() ? setVisibleRange(zoomAcsr.value().get(), visibleRange) : std::make_tuple(result, ZoomRanges{}));
    }
    return zooms;
}

std::map<juce::String, std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::ZoomRanges>> Application::Neuralyzer::Mcp::Operations::getTrackValueRange(Document::Accessor const& documentAcsr, std::vector<juce::String> const& identifiers)
{
    return getTracksValues(documentAcsr, identifiers, [](Track::Accessor const& trackAcsr)
                           {
                               auto const [result, zoomAcsr] = getTrackValueZoomAcsr(trackAcsr);
                               return std::make_tuple(result, result.wasOk() ? getZoomRanges(zoomAcsr.value().get()) : ZoomRanges{});
                           });
}

std::map<juce::String, std::tuple<juce::Result, Application::Neuralyzer::Mcp::Operations::ZoomRanges>> Application::Neuralyzer::Mcp::Operations::setTrackValueRange(Document::Accessor& documentAcsr, std::map<juce::String, juce::Range<double>> const& globalRanges)
{
    std::map<juce::String, std::tuple<juce::Result, ZoomRanges>> zooms;
    for(auto const& [identifier, globalRange] : globalRanges)
    {
        if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
        {
            zooms.insert_or_assign(identifier, std::make_tuple(createTrackNotFoundError(identifier), ZoomRanges{}));
            continue;
        }
        auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
        auto const [result, zoomAcsr] = getTrackValueZoomAcsr(trackAcsr);
        if(result.failed())
        {
            zooms.insert_or_assign(identifier, std::make_tuple(result, ZoomRanges{}));
            continue;
        }
        auto& valueZoomAcsr = zoomAcsr.value().get();
        if(globalRange.isEmpty())
        {
            zooms.insert_or_assign(identifier, std::make_tuple(juce::Result::fail("The 'start' value must be less than the 'end' value."), getZoomRanges(valueZoomAcsr)));
            continue;
        }
        // The zoom value mode follows the range so the range is kept when the plugin or the results change
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
        zooms.insert_or_assign(identifier, std::make_tuple(juce::Result::ok(), getZoomRanges(valueZoomAcsr)));
    }
    return zooms;
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

std::map<juce::String, juce::Result> Application::Neuralyzer::Mcp::Operations::setTrackIsVisibleInGroup(Document::Director& documentDir, std::map<juce::String, bool> const& visibilities)
{
    auto& documentAcsr = documentDir.getAccessor();
    return performDocumentAction(documentDir, visibilities, juce::translate("Set track visibility in group (agent)"), [&](juce::String const& identifier, bool visible)
                                 {
                                     if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                                     {
                                         return createTrackNotFoundError(identifier);
                                     }
                                     auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                                     trackAcsr.setAttr<Track::AttrType::showInGroup>(visible, NotificationType::synchronous);
                                     return juce::Result::ok();
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

std::map<juce::String, juce::Result> Application::Neuralyzer::Mcp::Operations::setTrackNames(Document::Director& documentDir, std::map<juce::String, juce::String> const& names)
{
    auto& documentAcsr = documentDir.getAccessor();
    return performDocumentAction(documentDir, names, juce::translate("Change track name (agent)"), [&](juce::String const& identifier, juce::String const& name)
                                 {
                                     if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                                     {
                                         return createTrackNotFoundError(identifier);
                                     }
                                     auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                                     trackAcsr.setAttr<Track::AttrType::name>(name, NotificationType::synchronous);
                                     return juce::Result::ok();
                                 });
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
} // namespace Application::Neuralyzer::Mcp::Operations

ANALYSE_FILE_END
