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

void Application::Tools::addPluginTracks(std::tuple<juce::String, size_t> position, std::set<Plugin::Key> const& keys)
{
    auto& documentAcsr = Instance::get().getDocumentAccessor();
    auto& documentDir = Instance::get().getDocumentDirector();
    documentDir.startAction();

    // Creates a group if there is none
    auto groupIdentifier = std::get<0_z>(position);
    auto trackPosition = std::get<1_z>(position);
    if(documentAcsr.getNumAcsrs<Document::AcsrType::groups>() == 0_z)
    {
        anlWeakAssert(groupIdentifier.isEmpty());
        auto const identifier = documentDir.addGroup(0, NotificationType::synchronous);
        anlWeakAssert(identifier.has_value());
        if(!identifier.has_value())
        {
            documentDir.endAction(ActionState::abort);
            auto const options = juce::MessageBoxOptions()
                                     .withIconType(juce::AlertWindow::WarningIcon)
                                     .withTitle(juce::translate("Group cannot be created!"))
                                     .withMessage(juce::translate("The group needed for the new track cannot be inserted into the document."))
                                     .withButton(juce::translate("Ok"));
            juce::AlertWindow::showAsync(options, nullptr);
            return;
        }
        groupIdentifier = *identifier;
        trackPosition = 0_z;
    }

    anlWeakAssert(documentAcsr.getNumAcsrs<Document::AcsrType::groups>() > 0_z);
    if(groupIdentifier.isEmpty())
    {
        auto const& layout = documentAcsr.getAttr<Document::AttrType::layout>();
        groupIdentifier = layout.front();
        trackPosition = 0_z;
    }

    auto& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupIdentifier);
    std::optional<juce::String> lastTrack;
    for(auto const& key : keys)
    {
        auto const identifier = documentDir.addTrack(groupIdentifier, trackPosition, NotificationType::synchronous);
        if(identifier.has_value())
        {
            auto const groupChannelStates = Group::Tools::getChannelVisibilityStates(groupAcsr);
            std::vector<bool> trackChannelsLayout(groupChannelStates.size(), false);
            std::transform(groupChannelStates.cbegin(), groupChannelStates.cend(), trackChannelsLayout.begin(), [](auto const& visibleState)
                           {
                               return visibleState == Group::ChannelVisibilityState::visible;
                           });

            auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, *identifier);
            auto getPluginName = [&]() -> juce::String
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
            };

            trackAcsr.setAttr<Track::AttrType::name>(getPluginName(), NotificationType::synchronous);
            trackAcsr.setAttr<Track::AttrType::key>(key, NotificationType::synchronous);
            trackAcsr.setAttr<Track::AttrType::channelsLayout>(trackChannelsLayout, NotificationType::synchronous);

            auto const colourChart = Instance::getColourChart();
            auto colours = trackAcsr.getAttr<Track::AttrType::colours>();
            colours.foreground = colourChart.get(LookAndFeel::ColourChart::Type::inactive);
            colours.text = colourChart.get(LookAndFeel::ColourChart::Type::text);
            trackAcsr.setAttr<Track::AttrType::colours>(colours, NotificationType::synchronous);

            groupAcsr.setAttr<Group::AttrType::expanded>(true, NotificationType::synchronous);
            lastTrack = identifier;
            ++trackPosition;
        }
    }

    if(lastTrack.has_value())
    {
        // If the group is not expanded, we have to wait a few ms before the new track becomes fully visible
        juce::Timer::callAfterDelay(500, [idtf = *lastTrack]()
                                    {
                                        Document::Selection::selectItem(Instance::get().getDocumentAccessor(), {idtf, {}}, true, false, NotificationType::synchronous);
                                    });
        documentDir.endAction(ActionState::newTransaction, juce::translate("New Tracks"));
    }
    else
    {
        documentDir.endAction(ActionState::abort);
    }
}

void Application::Tools::addFileTrack(std::tuple<juce::String, size_t> position, Track::FileInfo const& fileInfo)
{
    auto const& file = fileInfo.file;
    juce::WildcardFileFilter wildcardFilter(Instance::getWildCardForImportFormats(), file.getParentDirectory().getFullPathName(), "");
    if(!wildcardFilter.isFileSuitable(file))
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("File format not supported!"))
                                 .withMessage(juce::translate("The application only supports JSON, CSV, CUE, SDIF formats."))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
        return;
    }

    auto& documentAcsr = Instance::get().getDocumentAccessor();
    auto& documentDir = Instance::get().getDocumentDirector();
    documentDir.startAction();

    // Creates a group if there is none
    auto groupIdentifier = std::get<0_z>(position);
    auto trackPosition = std::get<1_z>(position);
    if(documentAcsr.getNumAcsrs<Document::AcsrType::groups>() == 0_z)
    {
        anlWeakAssert(groupIdentifier.isEmpty());

        auto const identifier = documentDir.addGroup(0, NotificationType::synchronous);
        anlWeakAssert(identifier.has_value());
        if(!identifier.has_value())
        {
            documentDir.endAction(ActionState::abort);
            auto const options = juce::MessageBoxOptions()
                                     .withIconType(juce::AlertWindow::WarningIcon)
                                     .withTitle(juce::translate("Group cannot be created!"))
                                     .withMessage(juce::translate("The group needed for the new track cannot be inserted into the document."))
                                     .withButton(juce::translate("Ok"));
            juce::AlertWindow::showAsync(options, nullptr);
            return;
        }
        groupIdentifier = *identifier;
        trackPosition = 0_z;
    }

    anlWeakAssert(documentAcsr.getNumAcsrs<Document::AcsrType::groups>() > 0_z);
    if(groupIdentifier.isEmpty())
    {
        auto const& layout = documentAcsr.getAttr<Document::AttrType::layout>();
        groupIdentifier = layout.front();
        trackPosition = 0_z;
    }

    auto const identifier = documentDir.addTrack(groupIdentifier, trackPosition, NotificationType::synchronous);
    if(identifier.has_value())
    {
        auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, *identifier);
        trackAcsr.setAttr<Track::AttrType::name>(file.getFileNameWithoutExtension(), NotificationType::synchronous);
        auto const colourChart = Instance::getColourChart();
        auto colours = trackAcsr.getAttr<Track::AttrType::colours>();
        colours.foreground = colourChart.get(LookAndFeel::ColourChart::Type::inactive);
        colours.text = colourChart.get(LookAndFeel::ColourChart::Type::text);
        trackAcsr.setAttr<Track::AttrType::colours>(colours, NotificationType::synchronous);

        trackAcsr.setAttr<Track::AttrType::file>(fileInfo, NotificationType::synchronous);

        auto& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupIdentifier);
        groupAcsr.setAttr<Group::AttrType::expanded>(true, NotificationType::synchronous);

        documentDir.endAction(ActionState::newTransaction, juce::translate("New Track"));
        // If the group is not expanded, we have to wait a few ms before the new track becomes fully visible
        juce::Timer::callAfterDelay(500, [idtf = *identifier]()
                                    {
                                        Document::Selection::selectItem(Instance::get().getDocumentAccessor(), {idtf, {}}, true, false, NotificationType::synchronous);
                                    });
    }
    else
    {
        documentDir.endAction(ActionState::abort);
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("Track cannot be created!"))
                                 .withMessage(juce::translate("The new track cannot be created into the document."))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    }
}

ANALYSE_FILE_END
