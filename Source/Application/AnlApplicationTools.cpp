#include "AnlApplicationTools.h"
#include "../Document/AnlDocumentSelection.h"
#include "../Track/AnlTrackTools.h"
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
    auto const& selectedIdentifier = selectedItem.value();
    if(Document::Tools::hasGroupAcsr(documentAcsr, selectedIdentifier))
    {
        return std::make_tuple(selectedIdentifier, 0_z);
    }
    if(Document::Tools::hasTrackAcsr(documentAcsr, selectedIdentifier))
    {
        auto const& groupAcsr = Document::Tools::getGroupAcsrForTrack(documentAcsr, selectedIdentifier);
        auto const groupIdentifier = groupAcsr.getAttr<Group::AttrType::identifier>();
        auto const position = Document::Tools::getTrackPosition(documentAcsr, selectedIdentifier);
        return std::make_tuple(groupIdentifier, position + 1_z);
    }
    MiscWeakAssert(false);
    return std::make_tuple(juce::String{}, 0_z);
}

std::tuple<juce::Result, juce::String, size_t> Application::Tools::prepareForNewTracks(juce::String const& groupIdentifier, size_t const trackPosition)
{
    auto& documentDir = Instance::get().getDocumentDirector();
    // Sanitize the document to make sure the group identifier is up to date and valid before adding the track to it.
    auto references = documentDir.sanitize(NotificationType::synchronous);
    auto identifier = references.count(groupIdentifier) > 0_z ? references.at(groupIdentifier) : groupIdentifier;
    // If there is no group, create a new one. The new track will then be added to this new group.
    auto const& documentAcsr = Instance::get().getDocumentAccessor();
    if(documentAcsr.getNumAcsrs<Document::AcsrType::groups>() == 0_z)
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
    // If the group does not exist, add the track to the first group.
    if(!Document::Tools::hasGroupAcsr(documentAcsr, identifier))
    {
        auto const& groupAcsr = documentAcsr.getAcsr<Document::AcsrType::groups>(0);
        identifier = groupAcsr.getAttr<Group::AttrType::identifier>();
        auto const trackCount = groupAcsr.getAttr<Group::AttrType::layout>().size();
        return std::make_tuple(juce::Result::ok(), identifier, std::min(trackCount, trackPosition));
    }
    // Use the group and the track position
    auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, identifier);
    auto const trackCount = groupAcsr.getAttr<Group::AttrType::layout>().size();
    return std::make_tuple(juce::Result::ok(), identifier, std::min(trackCount, trackPosition));
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
        MiscWeakAssert(Document::Tools::isTrackInGroup(documentAcsr, trackIdentifier));
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

std::tuple<juce::Result, std::vector<juce::String>> Application::Tools::addPluginTrack(Document::Director& documentDir, Track::PresetList::Accessor const& presetListAcsr, Track::GraphicsSettings const& globalGraphicPreset, juce::String const& groupIdentifier, size_t const trackPosition, Plugin::Key const& key, size_t const voiceCount)
{
    auto const& procPresets = presetListAcsr.getAttr<Track::PresetList::AttrType::processor>();
    auto const procPresetIt = procPresets.find(key);
    auto const& graphicPresets = presetListAcsr.getAttr<Track::PresetList::AttrType::graphic>();
    auto const graphicPresetIt = graphicPresets.find(key);

    // The descriptions of the voices of the output (or the description of the output if it has no voices),
    // the outputs can depend on the parameters so the processor preset is used if defined
    auto const descriptions = [&]()
    {
        try
        {
            auto const state = procPresetIt != procPresets.cend() ? procPresetIt->second : Plugin::State{};
            auto const defaultDescriptions = PluginList::Scanner::loadVoiceDescriptions(key, 48000.0, state, {});
            if(voiceCount == 0_z || defaultDescriptions.empty() || !defaultDescriptions.front().hasVoices() || defaultDescriptions.front().voices.empty())
            {
                return defaultDescriptions;
            }
            auto const& description = defaultDescriptions.front();
            auto const& firstVoice = description.voices.front();
            return PluginList::Scanner::loadVoiceDescriptions({key.identifier, firstVoice}, 48000.0, state, {{firstVoice, Plugin::Tools::clampVoiceCount(description.voiceDescriptor, voiceCount)}});
        }
        catch(...)
        {
        }
        return std::vector<Plugin::Description>{};
    }();
    if(descriptions.empty() || std::any_of(descriptions.cbegin(), descriptions.cend(), [](auto const& description)
                                           {
                                               return !description.valid();
                                           }))
    {
        return std::make_tuple(juce::Result::fail(juce::translate("Failed to load plugin description for key {\"identifier\": \"PLUGINID\", \"feature\": \"PLUGINFEATURE\"}.").replace("PLUGINID", key.identifier).replace("PLUGINFEATURE", key.feature)), std::vector<juce::String>{});
    }

    // The tracks of the voices are linked so they share the same analysis and processor properties (even with a
    // single voice so that the number of voices can be changed)
    auto const hasVoices = descriptions.front().hasVoices() && !descriptions.front().voices.empty();
    auto const voiceLink = hasVoices ? juce::Uuid().toString() : juce::String{};
    auto const voiceCounts = hasVoices ? Plugin::VoiceCounts{{descriptions.front().voices.front(), descriptions.size()}} : Plugin::VoiceCounts{};
    auto& documentAcsr = documentDir.getAccessor();
    std::vector<juce::String> trackIdentifiers;
    for(auto const& description : descriptions)
    {
        auto const name = description.output.name.empty() ? description.name : description.name + " - " + description.output.name;
        // The tracks of the voices are inserted from the bottom to the top (the first voice at the bottom)
        auto const [result, trackIdentifier] = documentDir.addTrack(groupIdentifier, trackPosition, NotificationType::synchronous);
        if(result.failed())
        {
            for(auto const& identifier : trackIdentifiers)
            {
                [[maybe_unused]] auto const removeResult = documentDir.removeTrack(identifier, NotificationType::synchronous);
            }
            return std::make_tuple(result, std::vector<juce::String>{});
        }
        trackIdentifiers.push_back(trackIdentifier);
        auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, trackIdentifier);
        auto const& groupAcsr = Document::Tools::getGroupAcsrForTrack(documentAcsr, trackIdentifier);
        auto const groupChannelStates = Group::Tools::getChannelVisibilityStates(groupAcsr);

        std::vector<bool> trackChannelsLayout(groupChannelStates.size(), false);
        std::transform(groupChannelStates.cbegin(), groupChannelStates.cend(), trackChannelsLayout.begin(), [](auto const& visibleState)
                       {
                           return visibleState == Group::ChannelVisibilityState::visible;
                       });

        trackAcsr.setAttr<Track::AttrType::name>(name, NotificationType::synchronous);
        if(hasVoices)
        {
            // The link, the numbers of voices, the description and the state are set before the key so that a single analysis is requested
            auto state = procPresetIt != procPresets.cend() ? procPresetIt->second : description.defaultState;
            if(state.blockSize == 0_z)
            {
                state.blockSize = description.defaultState.blockSize == 0_z ? 64_z : description.defaultState.blockSize;
            }
            if(state.stepSize == 0_z)
            {
                state.stepSize = description.defaultState.stepSize == 0_z ? state.blockSize : description.defaultState.stepSize;
            }
            trackAcsr.setAttr<Track::AttrType::voiceLink>(voiceLink, NotificationType::synchronous);
            trackAcsr.setAttr<Track::AttrType::voiceCounts>(voiceCounts, NotificationType::synchronous);
            trackAcsr.setAttr<Track::AttrType::description>(description, NotificationType::synchronous);
            trackAcsr.setAttr<Track::AttrType::state>(state, NotificationType::synchronous);
            trackAcsr.setAttr<Track::AttrType::key>(Plugin::Key{key.identifier, description.output.identifier}, NotificationType::synchronous);
        }
        else
        {
            trackAcsr.setAttr<Track::AttrType::key>(key, NotificationType::synchronous);
            // Apply default presets if defined
            if(procPresetIt != procPresets.cend())
            {
                trackAcsr.setAttr<Track::AttrType::state>(procPresetIt->second, NotificationType::synchronous);
            }
        }

        if(graphicPresetIt != graphicPresets.cend())
        {
            trackAcsr.setAttr<Track::AttrType::graphicsSettings>(graphicPresetIt->second, NotificationType::synchronous);
        }
        else
        {
            // Use global graphic preset as fallback
            trackAcsr.setAttr<Track::AttrType::graphicsSettings>(globalGraphicPreset, NotificationType::synchronous);
        }

        trackAcsr.setAttr<Track::AttrType::channelsLayout>(trackChannelsLayout, NotificationType::synchronous);

        // Enable the logarithmic scale if the vertical zoom of the new track allows it and if the
        // tracks of the group already use it
        if(Track::Tools::hasVerticalZoomInHertz(trackAcsr))
        {
            auto layout = groupAcsr.getAttr<Group::AttrType::layout>();
            std::erase_if(layout, [&](juce::String const& otherTrackIdentifier)
                          {
                              return std::find(trackIdentifiers.cbegin(), trackIdentifiers.cend(), otherTrackIdentifier) != trackIdentifiers.cend() || !Track::Tools::hasVerticalZoomInHertz(Document::Tools::getTrackAcsr(documentAcsr, otherTrackIdentifier));
                          });
            auto const groupUsesLogScale = !layout.empty() && std::all_of(layout.cbegin(), layout.cend(), [&](juce::String const& otherTrackIdentifier)
                                                                          {
                                                                              return Document::Tools::getTrackAcsr(documentAcsr, otherTrackIdentifier).getAttr<Track::AttrType::zoomLogScale>();
                                                                          });
            if(groupUsesLogScale)
            {
                trackAcsr.setAttr<Track::AttrType::zoomLogScale>(true, NotificationType::synchronous);
            }
        }
    }
    return std::make_tuple(juce::Result::ok(), trackIdentifiers);
}

std::tuple<juce::Result, juce::String> Application::Tools::addFileTrack(Document::Director& documentDir, Track::GraphicsSettings const& globalGraphicPreset, juce::String const& groupIdentifier, size_t const trackPosition, juce::File const& file)
{
    if(!file.existsAsFile())
    {
        return std::make_tuple(juce::Result::fail(juce::translate("The file \"FILENAME\" doesn't exist!").replace("FILENAME", file.getFullPathName())), juce::String());
    }

    auto const [result, trackIdentifier] = documentDir.addTrack(groupIdentifier, trackPosition, NotificationType::synchronous);
    if(result.failed())
    {
        return std::make_tuple(result, juce::String());
    }
    auto& documentAcsr = documentDir.getAccessor();
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
    trackAcsr.setAttr<Track::AttrType::graphicsSettings>(globalGraphicPreset, NotificationType::synchronous);

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
                                     .withButton(juce::translate("Ok"), 0);
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

static void addPluginTracksWithVoiceCounts(juce::String const& groupIdentifier, size_t const& trackFirstPosition, std::vector<Plugin::Key> const& keys, std::map<Plugin::Key, size_t> const& voiceCounts)
{
    auto& documentDir = Application::Instance::get().getDocumentDirector();
    documentDir.startAction();
    auto const prepareResult = Application::Tools::prepareForNewTracks(groupIdentifier, trackFirstPosition);
    if(std::get<0_z>(prepareResult).failed())
    {
        documentDir.endAction(ActionState::abort);
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("Tracks cannot be created!"))
                                 .withMessage(std::get<0_z>(prepareResult).getErrorMessage())
                                 .withButton(juce::translate("Ok"), 0);
        juce::AlertWindow::showAsync(options, nullptr);
        return;
    }

    auto const group = std::get<1_z>(prepareResult);
    auto position = std::get<2_z>(prepareResult);
    std::set<juce::String> trackIdentifiers;
    juce::StringArray failMessages;
    for(auto const& key : keys)
    {
        auto const voiceCountIt = voiceCounts.find(key);
        auto const voiceCount = voiceCountIt != voiceCounts.cend() ? voiceCountIt->second : 0_z;
        auto const [result, identifiers] = Application::Tools::addPluginTrack(documentDir, Application::Instance::get().getTrackPresetListAccessor(), Application::Instance::get().getApplicationAccessor().getAttr<Application::AttrType::globalGraphicPreset>(), group, position, key, voiceCount);
        if(result.failed())
        {
            failMessages.add(juce::String(key.identifier + ":" + key.feature) + ": " + result.getErrorMessage());
        }
        else
        {
            trackIdentifiers.insert(identifiers.cbegin(), identifiers.cend());
            position += identifiers.size();
        }
    }

    if(!trackIdentifiers.empty())
    {
        Application::Tools::revealTracks(trackIdentifiers);
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
                                 .withMessage(juce::translate("There are NUMFAILS tracks out of NUMTOTAL that could not be created in the document. They correspond to the plugins:\nPLUGINMESSAGES.").replace("NUMFAILS", juce::String(failMessages.size())).replace("NUMTOTAL", juce::String(keys.size())).replace("PLUGINMESSAGES", failMessages.joinIntoString("\n")))
                                 .withButton(juce::translate("Ok"), 0);
        juce::AlertWindow::showAsync(options, nullptr);
    }
}

// Requests the numbers of voices of the outputs one after the other with call out boxes at the mouse position, the
// tracks are created when the numbers of voices of all the outputs are defined (or canceled if a box is dismissed)
static void requestVoiceCounts(juce::String const& groupIdentifier, size_t const trackFirstPosition, std::vector<Plugin::Key> const& keys, std::vector<std::tuple<Plugin::Key, Plugin::Description>> const& voiceOutputs, std::map<Plugin::Key, size_t> const& voiceCounts, juce::Component& associatedComponent)
{
    if(voiceCounts.size() >= voiceOutputs.size())
    {
        addPluginTracksWithVoiceCounts(groupIdentifier, trackFirstPosition, keys, voiceCounts);
        return;
    }

    class VoiceCountEditor
    : public juce::Component
    {
    public:
        VoiceCountEditor(Plugin::Description const& description, std::function<void(size_t)> fn)
        : mVoiceDescriptor(description.voiceDescriptor)
        , mDefaultCount(Plugin::Tools::clampVoiceCount(mVoiceDescriptor, static_cast<size_t>(std::max(std::round(mVoiceDescriptor.defaultValue), 0.0f))))
        , mApply(std::move(fn))
        , mName("Name", juce::translate("Number of tracks of PLUGINNAME to create").replace("PLUGINNAME", description.name))
        {
            mProperty = Plugin::Tools::createProperty(mVoiceDescriptor, [this]([[maybe_unused]] auto const& parameter, float value)
                                                      {
                                                          apply(static_cast<size_t>(std::max(std::round(value), 0.0f)));
                                                      });
            Plugin::Tools::setPropertyValue(*mProperty.get(), mVoiceDescriptor, {static_cast<float>(mDefaultCount)}, juce::NotificationType::dontSendNotification);
            // The return key of the text editor applies the displayed number of voices even if it is unchanged
            if(auto* propertyNumber = dynamic_cast<PropertyNumber*>(mProperty.get()))
            {
                propertyNumber->entry.onEditorReturnKey = [this]()
                {
                    apply(getDisplayedCount());
                };
            }
            addAndMakeVisible(mName);
            addAndMakeVisible(mProperty.get());
            addAndMakeVisible(mCreate);
            setWantsKeyboardFocus(true);
            auto const textWidth = juce::GlyphArrangement::getStringWidthInt(mName.getFont(), mName.getText());
            setSize(std::max(240, textWidth + mName.getBorderSize().getLeftAndRight() + 8), 0);
            resized();
        }

        ~VoiceCountEditor() override = default;

        // juce::Component
        void resized() override
        {
            auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
            mName.setBounds(bounds.removeFromTop(24));
            mProperty->setBounds(bounds.removeFromTop(mProperty->getHeight()));
            mCreate.setBounds(bounds.removeFromTop(mCreate.getHeight()));
            setSize(getWidth(), bounds.getY() + 2);
        }

        bool keyPressed(juce::KeyPress const& key) override
        {
            // The return key applies the displayed number of voices
            if(key == juce::KeyPress::returnKey)
            {
                apply(getDisplayedCount());
                return true;
            }
            return false;
        }

    private:
        size_t getDisplayedCount() const
        {
            if(auto const* propertyNumber = dynamic_cast<PropertyNumber const*>(mProperty.get()))
            {
                return static_cast<size_t>(std::max(std::round(propertyNumber->entry.getValue()), 0.0));
            }
            return mDefaultCount;
        }

        void apply(size_t count)
        {
            if(mApply == nullptr)
            {
                return;
            }
            // The function is called after the dismissal of the call out box (that deletes the editor)
            auto const fn = std::exchange(mApply, nullptr);
            auto const voiceCount = Plugin::Tools::clampVoiceCount(mVoiceDescriptor, count);
            if(auto* box = findParentComponentOfClass<juce::CallOutBox>())
            {
                box->dismiss();
            }
            juce::MessageManager::callAsync([=]()
                                            {
                                                fn(voiceCount);
                                            });
        }

        Plugin::Parameter const mVoiceDescriptor;
        size_t const mDefaultCount;
        std::function<void(size_t)> mApply;
        juce::Label mName;
        std::unique_ptr<juce::Component> mProperty;
        PropertyTextButton mCreate{juce::translate("Create"), juce::translate("Create the tracks"), [this]()
                                   {
                                       apply(getDisplayedCount());
                                   }};
    };

    auto const& voiceOutput = voiceOutputs.at(voiceCounts.size());
    auto const key = std::get<0_z>(voiceOutput);
    juce::Component::SafePointer<juce::Component> safeComponent(&associatedComponent);
    auto editor = std::make_unique<VoiceCountEditor>(std::get<1_z>(voiceOutput), [=](size_t count)
                                                     {
                                                         if(safeComponent.getComponent() == nullptr)
                                                         {
                                                             return;
                                                         }
                                                         auto newVoiceCounts = voiceCounts;
                                                         newVoiceCounts[key] = count;
                                                         requestVoiceCounts(groupIdentifier, trackFirstPosition, keys, voiceOutputs, newVoiceCounts, *safeComponent.getComponent());
                                                     });
    auto const mousePosition = juce::Desktop::getMousePosition();
    Tooltip::showCallOutBox(std::move(editor), associatedComponent, juce::Rectangle<int>(mousePosition.x - 2, mousePosition.y - 2, 4, 4));
}

void Application::Tools::addPluginTracks(juce::String const& groupIdentifier, size_t const& trackFirstPosition, std::vector<Plugin::Key> const& keys, juce::Component* associatedComponent)
{
    // The numbers of voices of the outputs with voices are requested before creating the tracks
    std::vector<std::tuple<Plugin::Key, Plugin::Description>> voiceOutputs;
    if(associatedComponent != nullptr)
    {
        for(auto const& key : keys)
        {
            try
            {
                auto const description = Instance::get().getPluginListScanner().getDescription(key, 48000.0);
                if(description.hasVoices() && !description.voices.empty())
                {
                    voiceOutputs.push_back({key, description});
                }
            }
            catch(...)
            {
            }
        }
    }
    if(voiceOutputs.empty())
    {
        addPluginTracksWithVoiceCounts(groupIdentifier, trackFirstPosition, keys, {});
        return;
    }

    requestVoiceCounts(groupIdentifier, trackFirstPosition, keys, voiceOutputs, {}, *associatedComponent);
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
                                 .withButton(juce::translate("Ok"), 0);
        juce::AlertWindow::showAsync(options, nullptr);
        return;
    }

    auto const group = std::get<1_z>(prepareResult);
    auto position = std::get<2_z>(prepareResult);
    std::set<juce::String> trackIdentifiers;
    juce::StringArray failMessages;
    for(auto const& file : files)
    {
        auto const [result, identifier] = addFileTrack(documentDir, Instance::get().getApplicationAccessor().getAttr<AttrType::globalGraphicPreset>(), group, position, file);
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
                                 .withMessage(juce::translate("There are NUMFAILS tracks out of NUMTOTAL that could not be created in the document. They correspond to the files:\nFILEMESSAGES.").replace("NUMFAILS", juce::String(failMessages.size())).replace("NUMTOTAL", juce::String(files.size())).replace("FILEMESSAGES", failMessages.joinIntoString("\n")))
                                 .withButton(juce::translate("Ok"), 0);
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
                           .withMessage(juce::translate("PRODUCTNAME APPVER has been published on the PROJECTNAME page.").replace("PRODUCTNAME", product).replace("APPVER", upstreamVersion.toString()).replace("PROJECTNAME", project))
                           .withButton(juce::translate("Proceed to download page"), 1);
        options = warnIfUpToDate ? options.withButton(juce::translate("Close the window"), 0) : options.withButton(juce::translate("Ignore this version"), 2).withButton(juce::translate("Remind me later"), 0);
        juce::AlertWindow::showAsync(options, callback);
    }
    else if(warnIfUpToDate)
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::AlertIconType::InfoIcon)
                                 .withTitle(juce::translate("The version is up to date!"))
                                 .withMessage(juce::translate("PRODUCTNAME APPVER is the latest version available on the PROJECTNAME page.").replace("PRODUCTNAME", product).replace("APPVER", currentVersion.toString()).replace("PROJECTNAME", project))
                                 .withButton(juce::translate("Ok"), 0);
        juce::AlertWindow::showAsync(options, nullptr);
    }
}

ANALYSE_FILE_END
