#include "AnlApplicationExporter.h"
#include "../Document/AnlDocumentTools.h"
#include "../Group/AnlGroupExporter.h"
#include "../Track/AnlTrackExporter.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::ImageExporter::ImageExporter()
: FloatingWindowContainer("Image Exporter", *this)
, mPropertyItem("Item", "The item to export.", "", std::vector<std::string>{}, [this](size_t index)
                {
                    juce::ignoreUnused(index);
                    sanitizeProperties();
                })
, mPropertyGroupMode("Preserve group overlay", "Preserve group overlay of tracks when exporting", [this](bool state)
                     {
                         juce::ignoreUnused(state);
                         sanitizeProperties();
                     })
, mPropertyAutoSizeMode("Preserve item size", "Preserve the size of the track or the group", [this](bool state)
                        {
                            juce::ignoreUnused(state);
                            sanitizeProperties();
                        })
, mPropertyWidth("Image Width", "Set the width of the image", "pixel", juce::Range<float>{1.0f, 100000000}, 1.0f, [](float value)
                 {
                     juce::ignoreUnused(value);
                 })
, mPropertyHeight("Image Height", "Set the height of the image", "pixel", juce::Range<float>{1.0f, 100000000}, 1.0f, [](float value)
                  {
                      juce::ignoreUnused(value);
                  })
, mPropertyFormat("Image Format", "Set the format of the image", "", std::vector<std::string>{"JPEG", "PNG"}, [](size_t index)
                  {
                      juce::ignoreUnused(index);
                  })
, mPropertyExport("Export", "Export to image", [this]()
                  {
                      exportToFile();
                  })
{
    addAndMakeVisible(mPropertyItem);
    addAndMakeVisible(mPropertyGroupMode);
    addAndMakeVisible(mPropertyAutoSizeMode);
    addAndMakeVisible(mPropertyWidth);
    addAndMakeVisible(mPropertyHeight);
    addAndMakeVisible(mPropertyFormat);
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

    auto& documentAcsr = Instance::get().getDocumentAccessor();
    documentAcsr.addListener(mDocumentListener, NotificationType::synchronous);
    setSize(300, 200);
}

Application::ImageExporter::~ImageExporter()
{
    auto& documentAcsr = Instance::get().getDocumentAccessor();
    documentAcsr.removeListener(mDocumentListener);
}

void Application::ImageExporter::resized()
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
    setBounds(mPropertyGroupMode);
    setBounds(mPropertyAutoSizeMode);
    setBounds(mPropertyWidth);
    setBounds(mPropertyHeight);
    setBounds(mPropertyFormat);
    setBounds(mPropertyExport);
    setSize(bounds.getWidth(), bounds.getY() + 2);
}

void Application::ImageExporter::show(juce::Point<int> const& pt)
{
    FloatingWindowContainer::show(pt);
    mFloatingWindow.runModalLoop();
}

std::pair<int, int> Application::ImageExporter::getSizeFor(juce::String const& identifier)
{
    auto* window = Instance::get().getWindow();
    if(!identifier.isEmpty() && window != nullptr)
    {
        auto const bounds = window->getPlotBounds(identifier);
        anlWeakAssert(!bounds.isEmpty());
        if(!bounds.isEmpty())
        {
            auto const* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect(window->getScreenBounds());
            auto const scale = display != nullptr ? display->scale : 1.0;
            return {static_cast<int>(std::round(static_cast<double>(bounds.getWidth()) * scale)), static_cast<int>(std::round(static_cast<double>(bounds.getHeight()) * scale))};
        }
    }
    return {0, 0};
}

juce::String Application::ImageExporter::getSelectedIdentifier() const
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

void Application::ImageExporter::updateItems()
{
    auto const selectedId = getSelectedIdentifier();

    auto& documentAcsr = Instance::get().getDocumentAccessor();
    auto& entryBox = mPropertyItem.entry;
    entryBox.clear(juce::NotificationType::dontSendNotification);
    entryBox.addItem(juce::translate("Document: All"), documentItemFactor);
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

void Application::ImageExporter::sanitizeProperties()
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

void Application::ImageExporter::exportToFile()
{
    auto& documentAcsr = Instance::get().getDocumentAccessor();
    auto const identifier = getSelectedIdentifier();
    auto const width = static_cast<int>(mPropertyWidth.entry.getValue());
    auto const height = static_cast<int>(mPropertyHeight.entry.getValue());
    auto const autoSize = mPropertyAutoSizeMode.entry.getToggleState();
    auto const groupMode = mPropertyGroupMode.entry.getToggleState();
    auto const extension = mPropertyFormat.entry.getSelectedItemIndex() == 0 ? juce::String("jpg") : juce::String("png");

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
            AlertWindow::showMessage(AlertWindow::MessageType::info, "Export to image succeeded!", "All the groups of the document have been exported as images into the directory DIRNAME.", {{"DIRNAME", file.getFullPathName()}});
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
            AlertWindow::showMessage(AlertWindow::MessageType::info, "Export to image succeeded!", "All the tracks of the document have been exported as images into the directory DIRNAME.", {{"DIRNAME", file.getFullPathName()}});
        }
    }
    else if(Document::Tools::hasGroupAcsr(documentAcsr, identifier))
    {
        if(groupMode)
        {
            juce::FileChooser fc(juce::translate("Export as image"), {}, ((extension == "jpg") ? "*.jpeg;*.jpg" : "*.png"));
            if(!fc.browseForFileToSave(true))
            {
                return;
            }
            auto const file = fc.getResult();
            auto const result = exportGroup(identifier, file);
            if(result.failed())
            {
                AlertWindow::showMessage(AlertWindow::MessageType::warning, "Export to image failed!", result.getErrorMessage());
            }
            else
            {
                AlertWindow::showMessage(AlertWindow::MessageType::info, "Export to image succeeded!", "The group has been exported as an image into the file FILENAME.", {{"FILENAME", file.getFullPathName()}});
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
            AlertWindow::showMessage(AlertWindow::MessageType::info, "Export to image succeeded!", "All the tracks of the group have been exported as images into the directory DIRNAME.", {{"DIRNAME", file.getFullPathName()}});
        }
    }
    else if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
    {
        juce::FileChooser fc(juce::translate("Export as image"), {}, ((extension == "jpg") ? "*.jpeg;*.jpg" : "*.png"));
        if(!fc.browseForFileToSave(true))
        {
            return;
        }
        auto const file = fc.getResult();
        auto const result = exportTrack(identifier, file);
        if(result.failed())
        {
            AlertWindow::showMessage(AlertWindow::MessageType::warning, "Export to image failed!", result.getErrorMessage());
        }
        else
        {
            AlertWindow::showMessage(AlertWindow::MessageType::info, "Export to image succeeded!", "The track has been exported as an image into the file FILENAME.", {{"FILENAME", file.getFullPathName()}});
        }
    }
}

ANALYSE_FILE_END
