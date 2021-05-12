#include "AnlApplicationExporter.h"
#include "../Document/AnlDocumentTools.h"
#include "../Group/AnlGroupExporter.h"
#include "../Track/AnlTrackExporter.h"
#include "../Track/AnlTrackTools.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Exporter::Exporter()
: FloatingWindowContainer("Exporter", *this)
, mPropertyItem("Item", "The item to export.", "", std::vector<std::string>{}, [this](size_t index)
                {
                    juce::ignoreUnused(index);
                    sanitizeImageProperties();
                })
, mPropertyFormat("Format", "Select the export format", "", std::vector<std::string>{"JPEG", "PNG", "CSV", "XML", "JSON"}, [this](size_t index)
                  {
                      juce::ignoreUnused(index);
                      updateFormat();
                  })
, mPropertyGroupMode("Preserve group overlay", "Preserve group overlay of tracks when exporting", [this](bool state)
                     {
                         juce::ignoreUnused(state);
                         sanitizeImageProperties();
                     })
, mPropertyAutoSizeMode("Preserve item size", "Preserve the size of the track or the group", [this](bool state)
                        {
                            juce::ignoreUnused(state);
                            sanitizeImageProperties();
                        })
, mPropertyWidth("Image Width", "Set the width of the image", "pixel", juce::Range<float>{1.0f, 100000000}, 1.0f, [](float value)
                 {
                     juce::ignoreUnused(value);
                 })
, mPropertyHeight("Image Height", "Set the height of the image", "pixel", juce::Range<float>{1.0f, 100000000}, 1.0f, [](float value)
                  {
                      juce::ignoreUnused(value);
                  })
, mPropertyIgnoreGrids("Ignore Grid Tracks", "Ignore tracks with grid results", [this](bool state)
                       {
                           juce::ignoreUnused(state);
                           sanitizeImageProperties();
                       })
, mPropertyExport("Export", "Export the results", [this]()
                  {
                      exportToFile();
                  })
{
    addAndMakeVisible(mPropertyItem);
    addAndMakeVisible(mPropertyFormat);
    addAndMakeVisible(mPropertyGroupMode);
    addAndMakeVisible(mPropertyAutoSizeMode);
    addAndMakeVisible(mPropertyWidth);
    addAndMakeVisible(mPropertyHeight);
    addAndMakeVisible(mPropertyIgnoreGrids);
    addAndMakeVisible(mPropertyExport);

    mDocumentListener.onAttrChanged = [this](Document::Accessor const& acsr, Document::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Document::AttrType::file:
                break;
            case Document::AttrType::layout:
            {
                updateItems();
            }
            break;
        }
    };

    mDocumentListener.onAccessorInserted = mDocumentListener.onAccessorErased = [this](Document::Accessor const& acsr, Document::AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr, type, index);
        updateItems();
    };

    mPropertyWidth.entry.setValue(1920.0, juce::NotificationType::dontSendNotification);
    mPropertyHeight.entry.setValue(1200.0, juce::NotificationType::dontSendNotification);
    mPropertyFormat.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
    mPropertyIgnoreGrids.entry.setToggleState(true, juce::NotificationType::dontSendNotification);

    auto& documentAcsr = Instance::get().getDocumentAccessor();
    documentAcsr.addListener(mDocumentListener, NotificationType::synchronous);
    setSize(300, 200);
}

Application::Exporter::~Exporter()
{
    auto& documentAcsr = Instance::get().getDocumentAccessor();
    documentAcsr.removeListener(mDocumentListener);
}

void Application::Exporter::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyItem);
    setBounds(mPropertyFormat);
    setBounds(mPropertyGroupMode);
    setBounds(mPropertyAutoSizeMode);
    setBounds(mPropertyWidth);
    setBounds(mPropertyHeight);
    setBounds(mPropertyIgnoreGrids);
    setBounds(mPropertyExport);
    setSize(bounds.getWidth(), bounds.getY() + 2);
}

void Application::Exporter::show(juce::Point<int> const& pt)
{
    FloatingWindowContainer::show(pt);
    mFloatingWindow.runModalLoop();
}

std::pair<int, int> Application::Exporter::getSizeFor(juce::String const& identifier)
{
    auto* window = Instance::get().getWindow();
    if(!identifier.isEmpty() && window != nullptr)
    {
        auto const bounds = juce::Desktop::getInstance().getDisplays().logicalToPhysical(window->getPlotBounds(identifier));
        anlWeakAssert(!bounds.isEmpty());
        return {bounds.getWidth(), bounds.getHeight()};
    }
    return {0, 0};
}

juce::String Application::Exporter::getSelectedIdentifier() const
{
    auto const itemId = mPropertyItem.entry.getSelectedId();
    if(itemId % documentItemFactor == 0)
    {
        return {};
    }
    auto& documentAcsr = Instance::get().getDocumentAccessor();
    auto const documentLayout = copy_with_erased_if(documentAcsr.getAttr<Document::AttrType::layout>(), [&](auto const& groupId)
                                                    {
                                                        return !Document::Tools::hasGroupAcsr(documentAcsr, groupId);
                                                    });
    auto const groupIndex = static_cast<size_t>((itemId % documentItemFactor) / groupItemFactor - 1);
    anlWeakAssert(groupIndex < documentLayout.size());
    if(groupIndex >= documentLayout.size())
    {
        return {};
    }
    auto const groupId = documentLayout[groupIndex];
    if(itemId % groupItemFactor == 0)
    {
        return groupId;
    }
    anlWeakAssert(Document::Tools::hasGroupAcsr(documentAcsr, groupId));
    if(!Document::Tools::hasGroupAcsr(documentAcsr, groupId))
    {
        return {};
    }

    auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupId);
    auto const groupLayout = copy_with_erased_if(groupAcsr.getAttr<Group::AttrType::layout>(), [&](auto const& trackId)
                                                 {
                                                     return !Document::Tools::hasTrackAcsr(documentAcsr, trackId);
                                                 });
    auto const trackIndex = static_cast<size_t>((itemId % groupItemFactor) - 1);
    anlWeakAssert(trackIndex < groupLayout.size());
    if(trackIndex >= groupLayout.size())
    {
        return {};
    }
    return groupLayout[trackIndex];
}

void Application::Exporter::updateItems()
{
    auto const selectedId = getSelectedIdentifier();

    auto& documentAcsr = Instance::get().getDocumentAccessor();
    auto& entryBox = mPropertyItem.entry;
    entryBox.clear(juce::NotificationType::dontSendNotification);
    entryBox.addItem(juce::translate("All (Document)"), documentItemFactor);
    auto const documentLayout = copy_with_erased_if(documentAcsr.getAttr<Document::AttrType::layout>(), [&](auto const& groupId)
                                                    {
                                                        return !Document::Tools::hasGroupAcsr(documentAcsr, groupId);
                                                    });

    for(size_t documentLayoutIndex = 0; documentLayoutIndex < documentLayout.size(); ++documentLayoutIndex)
    {
        auto const& groupId = documentLayout[documentLayoutIndex];
        auto const groupItemId = groupItemFactor * static_cast<int>(documentLayoutIndex + 1_z) + documentItemFactor;
        anlWeakAssert(Document::Tools::hasGroupAcsr(documentAcsr, groupId));
        if(Document::Tools::hasGroupAcsr(documentAcsr, groupId))
        {
            auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupId);
            auto const& groupName = groupAcsr.getAttr<Group::AttrType::name>();
            entryBox.addItem(juce::translate("GROUPNAME (Group)").replace("GROUPNAME", groupName), groupItemId);
            if(groupId == selectedId)
            {
                entryBox.setSelectedId(groupItemId);
            }

            auto const groupLayout = copy_with_erased_if(groupAcsr.getAttr<Group::AttrType::layout>(), [&](auto const& trackId)
                                                         {
                                                             return !Document::Tools::hasTrackAcsr(documentAcsr, trackId);
                                                         });

            for(size_t groupLayoutIndex = 0; groupLayoutIndex < groupLayout.size(); ++groupLayoutIndex)
            {
                auto const& trackId = groupLayout[groupLayoutIndex];
                auto const trackItemId = groupItemId + static_cast<int>(groupLayoutIndex + 1_z);
                anlWeakAssert(Document::Tools::hasTrackAcsr(documentAcsr, trackId));
                if(Document::Tools::hasTrackAcsr(documentAcsr, trackId))
                {
                    auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, trackId);
                    auto const& trackName = trackAcsr.getAttr<Track::AttrType::name>();
                    entryBox.addItem(juce::translate("GROUPNAME: TRACKNAME (Track)").replace("GROUPNAME", groupName).replace("TRACKNAME", trackName), trackItemId);
                    if(trackId == selectedId)
                    {
                        entryBox.setSelectedId(trackItemId);
                    }
                }
            }
        }
    }

    if(mPropertyItem.entry.getSelectedId() == 0)
    {
        mPropertyItem.entry.setSelectedId(documentItemFactor);
    }
}

void Application::Exporter::updateFormat()
{
    auto const isImage = mPropertyFormat.entry.getSelectedItemIndex() < 2;
    mPropertyGroupMode.setVisible(isImage);
    mPropertyAutoSizeMode.setVisible(isImage);
    mPropertyWidth.setVisible(isImage);
    mPropertyHeight.setVisible(isImage);
    mPropertyIgnoreGrids.setVisible(!isImage);
    resized();
}

void Application::Exporter::sanitizeImageProperties()
{
    auto const& documentAcsr = Instance::get().getDocumentAccessor();
    auto const identifier = getSelectedIdentifier();
    auto const itemIsDocument = identifier.isEmpty();
    auto const itemIsGroup = Document::Tools::hasGroupAcsr(documentAcsr, identifier);
    auto const itemIsTrack = Document::Tools::hasTrackAcsr(documentAcsr, identifier);
    mPropertyGroupMode.setEnabled(itemIsGroup || itemIsDocument);

    if(itemIsDocument)
    {
        mPropertyWidth.setEnabled(true);
        mPropertyHeight.setEnabled(true);
    }

    if(itemIsTrack)
    {
        mPropertyGroupMode.setEnabled(false);
        mPropertyGroupMode.entry.setToggleState(false, juce::NotificationType::dontSendNotification);
    }

    auto const autoSize = mPropertyAutoSizeMode.entry.getToggleState();
    mPropertyWidth.setEnabled(!autoSize);
    mPropertyHeight.setEnabled(!autoSize);
    if(autoSize && !itemIsDocument)
    {
        auto const size = getSizeFor(identifier);
        mPropertyWidth.entry.setValue(std::get<0>(size), juce::NotificationType::dontSendNotification);
        mPropertyHeight.entry.setValue(std::get<1>(size), juce::NotificationType::dontSendNotification);
    }
}

void Application::Exporter::exportToFile()
{
    auto const identifier = getSelectedIdentifier();
    auto const format = magic_enum::enum_value<Format>(static_cast<size_t>(mPropertyFormat.entry.getSelectedItemIndex()));
    if(format == Format::JPEG || format == Format::PNG)
    {
        auto const width = static_cast<int>(mPropertyWidth.entry.getValue());
        auto const height = static_cast<int>(mPropertyHeight.entry.getValue());
        auto const autoSize = mPropertyAutoSizeMode.entry.getToggleState();
        auto const groupMode = mPropertyGroupMode.entry.getToggleState();
        exportToImage(format, identifier, autoSize, width, height, groupMode);
    }
    else
    {
        auto const ignoreGrids = mPropertyIgnoreGrids.entry.getToggleState();
        exportToText(format, identifier, ignoreGrids);
    }
}

void Application::Exporter::exportToImage(Format format, juce::String const& identifier, bool autoSize, int width, int height, bool groupMode)
{
    anlStrongAssert(format == Format::JPEG || format == Format::PNG);
    if(format != Format::JPEG && format != Format::PNG)
    {
        return;
    }
    auto const extension = juce::String(std::string(magic_enum::enum_name(format))).toLowerCase();
    auto& documentAcsr = Instance::get().getDocumentAccessor();
    auto& timeZoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
    auto exportTrack = [&](juce::String const& trackIdentifier, juce::File const& file)
    {
        auto const documentHasTrack = Document::Tools::hasTrackAcsr(documentAcsr, trackIdentifier);
        anlStrongAssert(documentHasTrack);
        if(!documentHasTrack)
        {
            return juce::Result::fail("Track is invalid");
        }
        auto const size = getSizeFor(trackIdentifier);
        auto const trackWidth = autoSize ? std::get<0>(size) : width;
        auto const trackheight = autoSize ? std::get<1>(size) : height;
        auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, trackIdentifier);
        return Track::Exporter::toImage(trackAcsr, timeZoomAcsr, file, trackWidth, trackheight);
    };

    auto exportGroup = [&](juce::String const& groupIdentifier, juce::File const& file)
    {
        auto const documentHasGroup = Document::Tools::hasGroupAcsr(documentAcsr, groupIdentifier);
        anlStrongAssert(documentHasGroup);
        if(!documentHasGroup)
        {
            return juce::Result::fail("Group is invalid");
        }
        auto const size = getSizeFor(groupIdentifier);
        auto const groupWidth = autoSize ? std::get<0>(size) : width;
        auto const groupheight = autoSize ? std::get<1>(size) : height;
        auto& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupIdentifier);
        return Group::Exporter::toImage(groupAcsr, timeZoomAcsr, file, groupWidth, groupheight);
    };

    auto exportGroupTracks = [&](juce::String const& groupIdentifier, juce::File const& file)
    {
        auto const documentHasGroup = Document::Tools::hasGroupAcsr(documentAcsr, groupIdentifier);
        anlStrongAssert(documentHasGroup);
        if(!documentHasGroup)
        {
            return juce::Result::fail("Group is invalid");
        }

        auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupIdentifier);
        auto const groupLayout = copy_with_erased_if(groupAcsr.getAttr<Group::AttrType::layout>(), [&](auto const& trackId)
                                                     {
                                                         return !Document::Tools::hasTrackAcsr(documentAcsr, trackId);
                                                     });
        auto const& groupName = juce::File::createLegalFileName(groupAcsr.getAttr<Group::AttrType::name>());
        for(auto const& trackIdentifier : groupLayout)
        {
            auto const documentHasTrack = Document::Tools::hasTrackAcsr(documentAcsr, trackIdentifier);
            anlStrongAssert(documentHasTrack);
            if(!documentHasTrack)
            {
                return juce::Result::fail("Track is invalid");
            }
            auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, trackIdentifier);
            auto const& trackName = juce::File::createLegalFileName(trackAcsr.getAttr<Track::AttrType::name>());
            auto const trackFile = file.getNonexistentChildFile(groupName + "_" + trackName, "." + extension);
            auto const result = exportTrack(trackIdentifier, trackFile);
            if(result.failed())
            {
                return result;
            }
        }
        return juce::Result::ok();
    };

    auto exportDocumentGroups = [&](juce::File const& file)
    {
        auto const documentLayout = copy_with_erased_if(documentAcsr.getAttr<Document::AttrType::layout>(), [&](auto const& groupId)
                                                        {
                                                            return !Document::Tools::hasGroupAcsr(documentAcsr, groupId);
                                                        });
        for(auto const& groupIdentifier : documentLayout)
        {
            auto const documentHasGroup = Document::Tools::hasGroupAcsr(documentAcsr, groupIdentifier);
            anlStrongAssert(documentHasGroup);
            if(!documentHasGroup)
            {
                return juce::Result::fail("Group is invalid");
            }

            auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupIdentifier);
            auto const& groupName = juce::File::createLegalFileName(groupAcsr.getAttr<Group::AttrType::name>());
            auto const groupFile = file.getNonexistentChildFile(groupName, "." + extension);
            auto const result = exportGroup(groupIdentifier, groupFile);
            if(result.failed())
            {
                return result;
            }
        }
        return juce::Result::ok();
    };

    if(identifier.isEmpty())
    {
        juce::FileChooser fc(juce::translate("Export as images"), {}, "");
        if(!fc.browseForDirectory())
        {
            return;
        }
        auto const file = fc.getResult();
        if(groupMode)
        {
            auto const result = exportDocumentGroups(file);
            if(result.failed())
            {
                AlertWindow::showMessage(AlertWindow::MessageType::warning, "Export as image failed!", result.getErrorMessage());
                return;
            }
            AlertWindow::showMessage(AlertWindow::MessageType::info, "Export as image succeeded!", "All the groups of the document have been exported as images into the directory DIRNAME.", {{"DIRNAME", file.getFullPathName()}});
        }
        else
        {
            auto const documentLayout = copy_with_erased_if(documentAcsr.getAttr<Document::AttrType::layout>(), [&](auto const& groupId)
                                                            {
                                                                return !Document::Tools::hasGroupAcsr(documentAcsr, groupId);
                                                            });

            for(auto const& groupIdentifier : documentLayout)
            {
                auto const result = exportGroupTracks(groupIdentifier, file);
                if(result.failed())
                {
                    AlertWindow::showMessage(AlertWindow::MessageType::warning, "Export as image failed!", result.getErrorMessage());
                    return;
                }
            }
            AlertWindow::showMessage(AlertWindow::MessageType::info, "Export as image succeeded!", "All the tracks of the document have been exported as images into the directory DIRNAME.", {{"DIRNAME", file.getFullPathName()}});
        }
    }
    else if(Document::Tools::hasGroupAcsr(documentAcsr, identifier))
    {
        if(groupMode)
        {
            juce::FileChooser fc(juce::translate("Export as image"), {}, ((format == Format::JPEG) ? "*.jpeg;*.jpg" : "*.png"));
            if(!fc.browseForFileToSave(true))
            {
                return;
            }
            auto const file = fc.getResult();
            auto const result = exportGroup(identifier, file);
            if(result.failed())
            {
                AlertWindow::showMessage(AlertWindow::MessageType::warning, "Export as image failed!", result.getErrorMessage());
            }
            else
            {
                AlertWindow::showMessage(AlertWindow::MessageType::info, "Export as image succeeded!", "The group has been exported as an image into the file FILENAME.", {{"FILENAME", file.getFullPathName()}});
            }
        }
        else
        {
            juce::FileChooser fc(juce::translate("Export as images"), {}, "");
            if(!fc.browseForDirectory())
            {
                return;
            }
            auto const file = fc.getResult();
            auto const result = exportGroupTracks(identifier, file);
            if(result.failed())
            {
                AlertWindow::showMessage(AlertWindow::MessageType::warning, "Export as image failed!", result.getErrorMessage());
                return;
            }
            AlertWindow::showMessage(AlertWindow::MessageType::info, "Export as image succeeded!", "All the tracks of the group have been exported as images into the directory DIRNAME.", {{"DIRNAME", file.getFullPathName()}});
        }
    }
    else if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
    {
        juce::FileChooser fc(juce::translate("Export as image"), {}, ((format == Format::JPEG) ? "*.jpeg;*.jpg" : "*.png"));
        if(!fc.browseForFileToSave(true))
        {
            return;
        }
        auto const file = fc.getResult();
        auto const result = exportTrack(identifier, file);
        if(result.failed())
        {
            AlertWindow::showMessage(AlertWindow::MessageType::warning, "Export as image failed!", result.getErrorMessage());
        }
        else
        {
            AlertWindow::showMessage(AlertWindow::MessageType::info, "Export as image succeeded!", "The track has been exported as an image into the file FILENAME.", {{"FILENAME", file.getFullPathName()}});
        }
    }
}

void Application::Exporter::exportToText(Format format, juce::String const& identifier, bool ignoreGrids)
{
    auto& documentAcsr = Instance::get().getDocumentAccessor();
    auto const formatName = juce::String(std::string(magic_enum::enum_name(format)));
    auto const formatExtension = formatName.toLowerCase();
    auto const formatWilcard = "*." + formatExtension;

    auto const fcTitle = juce::translate("Export as FORMATNAME").replace("FORMATNAME", formatName);
    auto const failedTitle = juce::translate("Export as FORMATNAME failed!").replace("FORMATNAME", formatName);
    auto const succeededTitle = juce::translate("Export as FORMATNAME succeeded!").replace("FORMATNAME", formatName);

    auto exportTrack = [&](juce::String const& trackIdentifier, juce::File const& file)
    {
        auto const documentHasTrack = Document::Tools::hasTrackAcsr(documentAcsr, trackIdentifier);
        anlStrongAssert(documentHasTrack);
        if(!documentHasTrack)
        {
            return juce::Result::fail("Track is invalid");
        }
        auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, trackIdentifier);
        if(ignoreGrids && Track::Tools::getDisplayType(trackAcsr) == Track::Tools::DisplayType::grid)
        {
            return juce::Result::ok();
        }

        switch(format)
        {
            case Format::JPEG:
                return juce::Result::fail("Unsupported format");
            case Format::PNG:
                return juce::Result::fail("Unsupported format");
            case Format::CSV:
                return Track::Exporter::toCsv(trackAcsr, file);
            case Format::XML:
                return Track::Exporter::toXml(trackAcsr, file);
            case Format::JSON:
                return Track::Exporter::toJson(trackAcsr, file);
        }
        return juce::Result::fail("Unsupported format");
    };

    auto exportGroupTracks = [&](juce::String const& groupIdentifier, juce::File const& file)
    {
        auto const documentHasGroup = Document::Tools::hasGroupAcsr(documentAcsr, groupIdentifier);
        anlStrongAssert(documentHasGroup);
        if(!documentHasGroup)
        {
            return juce::Result::fail("Group is invalid");
        }

        auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupIdentifier);
        auto const groupLayout = copy_with_erased_if(groupAcsr.getAttr<Group::AttrType::layout>(), [&](auto const& trackId)
                                                     {
                                                         return !Document::Tools::hasTrackAcsr(documentAcsr, trackId);
                                                     });
        auto const& groupName = juce::File::createLegalFileName(groupAcsr.getAttr<Group::AttrType::name>());
        for(auto const& trackIdentifier : groupLayout)
        {
            auto const documentHasTrack = Document::Tools::hasTrackAcsr(documentAcsr, trackIdentifier);
            anlStrongAssert(documentHasTrack);
            if(!documentHasTrack)
            {
                return juce::Result::fail("Track is invalid");
            }
            auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, trackIdentifier);
            auto const& trackName = juce::File::createLegalFileName(trackAcsr.getAttr<Track::AttrType::name>());
            auto const trackFile = file.getNonexistentChildFile(groupName + "_" + trackName, "." + formatExtension);
            auto const result = exportTrack(trackIdentifier, trackFile);
            if(result.failed())
            {
                return result;
            }
        }
        return juce::Result::ok();
    };

    if(identifier.isEmpty())
    {
        juce::FileChooser fc(fcTitle, {}, formatWilcard);
        if(!fc.browseForDirectory())
        {
            return;
        }
        juce::MouseCursor::showWaitCursor();
        auto const file = fc.getResult();
        auto const documentLayout = copy_with_erased_if(documentAcsr.getAttr<Document::AttrType::layout>(), [&](auto const& groupId)
                                                        {
                                                            return !Document::Tools::hasGroupAcsr(documentAcsr, groupId);
                                                        });

        for(auto const& groupIdentifier : documentLayout)
        {
            auto const result = exportGroupTracks(groupIdentifier, file);
            if(result.failed())
            {
                juce::MouseCursor::hideWaitCursor();
                AlertWindow::showMessage(AlertWindow::MessageType::warning, failedTitle, result.getErrorMessage());
                return;
            }
        }
        AlertWindow::showMessage(AlertWindow::MessageType::info, succeededTitle, "All the tracks of the document have been exported as FORMATNAME into the directory DIRNAME.", {{"FORMATNAME", formatName}, {"DIRNAME", file.getFullPathName()}});
        juce::MouseCursor::hideWaitCursor();
    }
    else if(Document::Tools::hasGroupAcsr(documentAcsr, identifier))
    {
        juce::FileChooser fc(juce::translate("Export as images"), {}, "");
        if(!fc.browseForDirectory())
        {
            return;
        }
        juce::MouseCursor::showWaitCursor();
        auto const file = fc.getResult();
        auto const result = exportGroupTracks(identifier, file);
        if(result.failed())
        {
            juce::MouseCursor::hideWaitCursor();
            AlertWindow::showMessage(AlertWindow::MessageType::warning, failedTitle, result.getErrorMessage());
            return;
        }
        juce::MouseCursor::hideWaitCursor();
        AlertWindow::showMessage(AlertWindow::MessageType::info, succeededTitle, "All the tracks of the group have been exported as FORMATNAME into the directory DIRNAME.", {{"FORMATNAME", formatName}, {"DIRNAME", file.getFullPathName()}});
    }
    else if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
    {
        juce::FileChooser fc(fcTitle, {}, formatWilcard);
        if(!fc.browseForFileToSave(true))
        {
            return;
        }
        juce::MouseCursor::showWaitCursor();
        auto const file = fc.getResult();
        auto const result = exportTrack(identifier, file);
        if(result.failed())
        {
            juce::MouseCursor::hideWaitCursor();
            AlertWindow::showMessage(AlertWindow::MessageType::warning, failedTitle, result.getErrorMessage());
            return;
        }
        juce::MouseCursor::hideWaitCursor();
        AlertWindow::showMessage(AlertWindow::MessageType::info, succeededTitle, "The track has been exported as FORMATNAME into the file FILENAME.", {{"FORMATNAME", formatName}, {"FILENAME", file.getFullPathName()}});
    }
}

ANALYSE_FILE_END
