#include "AnlApplicationTools.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

std::tuple<juce::String, size_t> Application::Tools::getNewTrackPosition()
{
    auto const& documentAcsr = Instance::get().getDocumentAccessor();
    auto const focusedTrack = Document::Tools::getFocusedTrack(documentAcsr);
    if(focusedTrack.has_value())
    {
        auto const& groupAcsr = Document::Tools::getGroupAcsrForTrack(documentAcsr, *focusedTrack);
        auto const groupIdentifier = groupAcsr.getAttr<Group::AttrType::identifier>();
        auto const position = Document::Tools::getTrackPosition(documentAcsr, *focusedTrack);
        return std::make_tuple(groupIdentifier, position + 1_z);
    }
    auto const focusedGroup = Document::Tools::getFocusedGroup(documentAcsr);
    if(focusedGroup.has_value())
    {
        return std::make_tuple(*focusedGroup, 0_z);
    }
    return std::make_tuple(juce::String(""), 0_z);
}

void Application::Tools::addPluginTrack(std::tuple<juce::String, size_t> position, Plugin::Key const& key, Plugin::Description const& description)
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

    auto const identifier = documentDir.addTrack(groupIdentifier, trackPosition, NotificationType::synchronous);
    if(identifier.has_value())
    {
        auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, *identifier);
        trackAcsr.setAttr<Track::AttrType::name>(description.name, NotificationType::synchronous);
        trackAcsr.setAttr<Track::AttrType::key>(key, NotificationType::synchronous);

        auto const& acsr = Instance::get().getApplicationAccessor();
        LookAndFeel::ColourChart const colourChart(acsr.getAttr<AttrType::colourMode>());
        auto colours = trackAcsr.getAttr<Track::AttrType::colours>();
        colours.foreground = colourChart.get(LookAndFeel::ColourChart::Type::inactive);
        colours.text = colourChart.get(LookAndFeel::ColourChart::Type::text);
        trackAcsr.setAttr<Track::AttrType::colours>(colours, NotificationType::synchronous);

        auto& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupIdentifier);
        groupAcsr.setAttr<Group::AttrType::expanded>(true, NotificationType::synchronous);

        documentDir.endAction(ActionState::newTransaction, juce::translate("New Track"));
        // If the group is not expanded, we have to wait a few ms before the new track becomes fully visible
        juce::Timer::callAfterDelay(500, [idtf = *identifier]()
                                    {
                                        if(auto* window = Instance::get().getWindow())
                                        {
                                            window->moveKeyboardFocusTo(idtf);
                                        }
                                    });
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
        auto const& acsr = Instance::get().getApplicationAccessor();
        LookAndFeel::ColourChart const colourChart(acsr.getAttr<AttrType::colourMode>());
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
                                        if(auto* window = Instance::get().getWindow())
                                        {
                                            window->moveKeyboardFocusTo(idtf);
                                        }
                                    });
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::InfoIcon)
                                 .withTitle(juce::translate("Track imported!"))
                                 .withMessage(juce::translate("The new track has been imported from the file FLNAME into the document.").replace("FLNAME", file.getFullPathName()))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
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
