#include "AnlApplicationTools.h"
#include "../Document/AnlDocumentSelection.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

size_t Application::Tools::getNewGroupPosition()
{
    auto const& documentAcsr = Instance::get().getDocumentAccessor();
    auto const selectedItem = Document::Selection::getFarthestItem(documentAcsr);
    if(!selectedItem.has_value())
    {
        return documentAcsr.getNumAcsrs<Document::AcsrType::groups>();
    }
    else if(Document::Tools::hasGroupAcsr(documentAcsr, *selectedItem))
    {
        return Document::Tools::getGroupPosition(documentAcsr, *selectedItem) + 1_z;
    }
    else if(Document::Tools::hasTrackAcsr(documentAcsr, *selectedItem))
    {
        auto const& groupAcsr = Document::Tools::getGroupAcsrForTrack(documentAcsr, *selectedItem);
        auto const groupIdentifier = groupAcsr.getAttr<Group::AttrType::identifier>();
        return Document::Tools::getGroupPosition(documentAcsr, groupIdentifier) + 1_z;
    }
    return 0_z;
}

std::tuple<juce::String, size_t> Application::Tools::getNewTrackPosition()
{
    auto const& documentAcsr = Instance::get().getDocumentAccessor();
    auto const selectedItem = Document::Selection::getFarthestItem(documentAcsr);
    if(!selectedItem.has_value())
    {
        return std::make_tuple(juce::String{}, 0_z);
    }
    else if(Document::Tools::hasGroupAcsr(documentAcsr, *selectedItem))
    {
        return std::make_tuple(*selectedItem, 0_z);
    }
    else if(Document::Tools::hasTrackAcsr(documentAcsr, *selectedItem))
    {
        auto const& groupAcsr = Document::Tools::getGroupAcsrForTrack(documentAcsr, *selectedItem);
        auto const groupIdentifier = groupAcsr.getAttr<Group::AttrType::identifier>();
        auto const position = Document::Tools::getTrackPosition(documentAcsr, *selectedItem);
        return std::make_tuple(groupIdentifier, position + 1_z);
    }
    MiscWeakAssert(false);
    return std::make_tuple(juce::String{}, 0_z);
}

std::tuple<juce::Result, juce::String, size_t> Application::Tools::prepareForNewTracks(juce::String const& groupIdentifier, size_t const trackPosition)
{
    auto& documentDir = Instance::get().getDocumentDirector();
    auto references = documentDir.sanitize(NotificationType::synchronous);
    auto identifier = references.count(groupIdentifier) > 0_z ? references.at(groupIdentifier) : groupIdentifier;
    auto& documentAcsr = Instance::get().getDocumentAccessor();
    if(identifier.isEmpty() && documentAcsr.getNumAcsrs<Document::AcsrType::groups>() == 0_z)
    {
        auto const [result, newIdentifier] = documentDir.addGroup(0, NotificationType::synchronous);
        if(result.failed())
        {
            return std::make_tuple(result, juce::String(), 0_z);
        }
        references = documentDir.sanitize(NotificationType::synchronous);
        identifier = references.count(newIdentifier) > 0_z ? references.at(newIdentifier) : newIdentifier;
        return std::make_tuple(juce::Result::ok(), identifier, 0_z);
    }
    return std::make_tuple(juce::Result::ok(), identifier, trackPosition);
}

void Application::Tools::revealTracks(std::set<juce::String> const& trackIdentifiers)
{
    auto copy = trackIdentifiers;
    auto& documentDir = Instance::get().getDocumentDirector();
    auto const newReferences = documentDir.sanitize(NotificationType::synchronous);
    for(auto const& newReference : newReferences)
    {
        if(copy.count(newReference.first) > 0_z)
        {
            copy.erase(newReference.first);
            copy.insert(newReference.second);
        }
    }

    auto& documentAcsr = Instance::get().getDocumentAccessor();
    for(auto const& trackIdentifier : copy)
    {
        anlWeakAssert(Document::Tools::isTrackInGroup(documentAcsr, trackIdentifier));
        if(Document::Tools::isTrackInGroup(documentAcsr, trackIdentifier))
        {
            auto& groupAcsr = Document::Tools::getGroupAcsrForTrack(documentAcsr, trackIdentifier);
            groupAcsr.setAttr<Group::AttrType::expanded>(true, NotificationType::synchronous);
        }
    }

    // If the group is not expanded, we have to wait a few ms before the new track becomes fully visible
    juce::Timer::callAfterDelay(500, [identifiers = std::move(copy)]()
                                {
                                    Document::Selection::clearAll(Instance::get().getDocumentAccessor(), NotificationType::synchronous);
                                    for(auto const& trackIdentifier : identifiers)
                                    {
                                        Document::Selection::selectItem(Instance::get().getDocumentAccessor(), {trackIdentifier, {}}, false, false, NotificationType::synchronous);
                                    }
                                });
}

std::tuple<juce::Result, juce::String> Application::Tools::addPluginTrack(juce::String const& groupIdentifier, size_t const trackPosition, Plugin::Key const& key)
{
    auto const name = [&]() -> juce::String
    {
        try
        {
            auto const description = PluginList::Scanner::loadDescription(key, 48000.0);
            return description.name;
        }
        catch(...)
        {
            return "";
        }
    }();
    if(name.isEmpty())
    {
        return std::make_tuple(juce::Result::fail(juce::translate("Failed to load plugin description for key \"PLUGINKEY\".").replace("PLUGINKEY", key.identifier + ":" + key.feature)), juce::String());
    }

    auto& documentDir = Instance::get().getDocumentDirector();
    auto const [result, trackIdentifier] = documentDir.addTrack(groupIdentifier, trackPosition, NotificationType::synchronous);
    if(result.failed())
    {
        return std::make_tuple(result, juce::String());
    }
    auto& documentAcsr = Instance::get().getDocumentAccessor();
    auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, trackIdentifier);
    auto const& groupAcsr = Document::Tools::getGroupAcsrForTrack(documentAcsr, trackIdentifier);
    auto const groupChannelStates = Group::Tools::getChannelVisibilityStates(groupAcsr);

    std::vector<bool> trackChannelsLayout(groupChannelStates.size(), false);
    std::transform(groupChannelStates.cbegin(), groupChannelStates.cend(), trackChannelsLayout.begin(), [](auto const& visibleState)
                   {
                       return visibleState == Group::ChannelVisibilityState::visible;
                   });

    trackAcsr.setAttr<Track::AttrType::name>(name, NotificationType::synchronous);
    trackAcsr.setAttr<Track::AttrType::key>(key, NotificationType::synchronous);

    // Apply default presets if defined
    auto const procPresets = Instance::get().getTrackPresetListAccessor().getAttr<Track::PresetList::AttrType::processor>();
    auto const procPresetIt = procPresets.find(key);
    if(procPresetIt != procPresets.cend())
    {
        trackAcsr.setAttr<Track::AttrType::state>(procPresetIt->second, NotificationType::synchronous);
    }
    auto const graphicPresets = Instance::get().getTrackPresetListAccessor().getAttr<Track::PresetList::AttrType::graphic>();
    auto const graphicPresetIt = graphicPresets.find(key);
    if(graphicPresetIt != graphicPresets.cend())
    {
        trackAcsr.setAttr<Track::AttrType::graphicsSettings>(graphicPresetIt->second, NotificationType::synchronous);
    }
    else
    {
        // Use global graphic preset as fallback
        auto const& globalPreset = Instance::get().getApplicationAccessor().getAttr<AttrType::globalGraphicPreset>();
        trackAcsr.setAttr<Track::AttrType::graphicsSettings>(globalPreset, NotificationType::synchronous);
    }

    trackAcsr.setAttr<Track::AttrType::channelsLayout>(trackChannelsLayout, NotificationType::synchronous);
    return std::make_tuple(juce::Result::ok(), trackIdentifier);
}

std::tuple<juce::Result, juce::String> Application::Tools::addFileTrack(juce::String const& groupIdentifier, size_t const trackPosition, juce::File const& file)
{
    if(!file.existsAsFile())
    {
        return std::make_tuple(juce::Result::fail(juce::translate("The file \"FILENAME\" doesn't exist!").replace("FILENAME", file.getFullPathName())), juce::String());
    }

    auto& documentDir = Instance::get().getDocumentDirector();
    auto const [result, trackIdentifier] = documentDir.addTrack(groupIdentifier, trackPosition, NotificationType::synchronous);
    if(result.failed())
    {
        return std::make_tuple(result, juce::String());
    }
    auto& documentAcsr = Instance::get().getDocumentAccessor();
    auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, trackIdentifier);
    trackAcsr.setAttr<Track::AttrType::name>(file.getFileNameWithoutExtension(), NotificationType::synchronous);

    auto const& groupAcsr = Document::Tools::getGroupAcsrForTrack(documentAcsr, trackIdentifier);
    auto const groupChannelStates = Group::Tools::getChannelVisibilityStates(groupAcsr);

    std::vector<bool> trackChannelsLayout(groupChannelStates.size(), false);
    std::transform(groupChannelStates.cbegin(), groupChannelStates.cend(), trackChannelsLayout.begin(), [](auto const& visibleState)
                   {
                       return visibleState == Group::ChannelVisibilityState::visible;
                   });

    // Use global graphic preset
    auto const& globalPreset = Instance::get().getApplicationAccessor().getAttr<AttrType::globalGraphicPreset>();
    trackAcsr.setAttr<Track::AttrType::graphicsSettings>(globalPreset, NotificationType::synchronous);

    trackAcsr.setAttr<Track::AttrType::file>(Track::FileInfo{file}, NotificationType::synchronous);

    trackAcsr.setAttr<Track::AttrType::channelsLayout>(trackChannelsLayout, NotificationType::synchronous);
    return std::make_tuple(juce::Result::ok(), trackIdentifier);
}

void Application::Tools::addGroups(size_t numGroups, size_t firstGroupPosition)
{
    auto& documentDir = Instance::get().getDocumentDirector();
    documentDir.startAction();
    std::set<juce::String> groupIdentifiers;
    while(numGroups-- > 0_z)
    {
        auto const [result, groupIdentifier] = documentDir.addGroup(firstGroupPosition++, NotificationType::synchronous);
        if(result.failed())
        {
            documentDir.endAction(ActionState::abort);
            auto const options = juce::MessageBoxOptions()
                                     .withIconType(juce::AlertWindow::WarningIcon)
                                     .withTitle(juce::translate("Group cannot be created!"))
                                     .withMessage(result.getErrorMessage())
                                     .withButton(juce::translate("Ok"));
            juce::AlertWindow::showAsync(options, nullptr);
            return;
        }
        else
        {
            groupIdentifiers.insert(groupIdentifier);
        }
    }
    auto const references = documentDir.sanitize(NotificationType::synchronous);
    auto& documentAcsr = Instance::get().getDocumentAccessor();
    Document::Selection::clearAll(documentAcsr, NotificationType::synchronous);
    for(auto const& identifier : groupIdentifiers)
    {
        auto const groupIdentifier = references.count(identifier) > 0_z ? references.at(identifier) : identifier;
        Document::Selection::selectItem(documentAcsr, {groupIdentifier, {}}, false, false, NotificationType::synchronous);
    }
    documentDir.endAction(ActionState::newTransaction, juce::translate("New Group"));
}

void Application::Tools::addPluginTracks(juce::String const& groupIdentifier, size_t const& trackFirstPosition, std::vector<Plugin::Key> const& keys)
{
    auto& documentDir = Instance::get().getDocumentDirector();
    documentDir.startAction();
    auto const prepareResult = prepareForNewTracks(groupIdentifier, trackFirstPosition);
    if(std::get<0_z>(prepareResult).failed())
    {
        documentDir.endAction(ActionState::abort);
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("Tracks cannot be created!"))
                                 .withMessage(std::get<0_z>(prepareResult).getErrorMessage())
                                 .withButton(juce::translate("Ok"));
        return;
    }

    auto const group = std::get<1_z>(prepareResult);
    auto position = std::get<2_z>(prepareResult);
    std::set<juce::String> trackIdentifiers;
    juce::StringArray failMessages;
    for(auto const& key : keys)
    {
        auto const [result, identifier] = addPluginTrack(group, position, key);
        if(result.failed())
        {
            failMessages.add(juce::String(key.identifier + ":" + key.feature) + ": " + result.getErrorMessage());
        }
        else
        {
            trackIdentifiers.insert(identifier);
            ++position;
        }
    }

    if(!trackIdentifiers.empty())
    {
        revealTracks(trackIdentifiers);
        documentDir.endAction(ActionState::newTransaction, juce::translate("New Track(s)"));
    }
    else
    {
        documentDir.endAction(ActionState::abort);
    }
    if(!failMessages.isEmpty())
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("Tracks cannot be created!"))
                                 .withMessage(juce::translate("There are NUMFAILS tracks out of TOTAL that could not be created in the document. They correspond to the plugins:\n PLUGINMESSAGES.").replace("NUMFAILS", juce::String(failMessages.size())).replace("TOTAL", juce::String(keys.size())).replace("PLUGINMESSAGES", failMessages.joinIntoString("\n")))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    }
}

void Application::Tools::addFileTracks(juce::String const& groupIdentifier, size_t const& trackFirstPosition, std::vector<juce::File> const& files)
{
    auto& documentDir = Instance::get().getDocumentDirector();
    documentDir.startAction();
    auto const prepareResult = prepareForNewTracks(groupIdentifier, trackFirstPosition);
    if(std::get<0_z>(prepareResult).failed())
    {
        documentDir.endAction(ActionState::abort);
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("Tracks cannot be created!"))
                                 .withMessage(std::get<0_z>(prepareResult).getErrorMessage())
                                 .withButton(juce::translate("Ok"));
        return;
    }

    auto const group = std::get<1_z>(prepareResult);
    auto position = std::get<2_z>(prepareResult);
    std::set<juce::String> trackIdentifiers;
    juce::StringArray failMessages;
    for(auto const& file : files)
    {
        auto const [result, identifier] = addFileTrack(group, position, file);
        if(result.failed())
        {
            failMessages.add(file.getFullPathName() + ": " + result.getErrorMessage());
        }
        else
        {
            trackIdentifiers.insert(identifier);
            ++position;
        }
    }

    if(!trackIdentifiers.empty())
    {
        revealTracks(trackIdentifiers);
        documentDir.endAction(ActionState::newTransaction, juce::translate("New Track(s)"));
    }
    else
    {
        documentDir.endAction(ActionState::abort);
    }
    if(!failMessages.isEmpty())
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("Tracks cannot be created!"))
                                 .withMessage(juce::translate("There are NUMFAILS tracks out of TOTAL that could not be created in the document. They correspond to the files:\n FILEMESSAGES.").replace("NUMFAILS", juce::String(failMessages.size())).replace("TOTAL", juce::String(files.size())).replace("FILEMESSAGES", failMessages.joinIntoString("\n")))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    }
}

void Application::Tools::notifyForNewVersion(Misc::Version const& upstreamVersion, Version const currentVersion, bool isCurrentVersionDev, bool warnIfUpToDate, juce::String const& product, juce::String const& project, std::function<void(int)> callback)
{
    if(upstreamVersion > currentVersion || (isCurrentVersionDev && upstreamVersion.tie() >= currentVersion.tie()))
    {
        auto options = juce::MessageBoxOptions()
                           .withIconType(juce::AlertWindow::AlertIconType::InfoIcon)
                           .withTitle(juce::translate("A new version is available!"))
                           .withMessage(juce::translate("PRODUCT VERSION has been published on the PROJECT page.").replace("PRODUCT", product).replace("VERSION", upstreamVersion.toString()).replace("PROJECT", project))
                           .withButton(juce::translate("Proceed to download page"));
        options = warnIfUpToDate ? options.withButton(juce::translate("Close the window")) : options.withButton(juce::translate("Ignore this version")).withButton(juce::translate("Remind me later"));
        juce::AlertWindow::showAsync(options, callback);
    }
    else if(warnIfUpToDate)
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::AlertIconType::InfoIcon)
                                 .withTitle(juce::translate("The version is up to date!"))
                                 .withMessage(juce::translate("PRODUCT VERSION is the latest version available on the PROJECT page.").replace("PRODUCT", product).replace("VERSION", currentVersion.toString()).replace("PROJECT", project))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    }
}

ANALYSE_FILE_END
