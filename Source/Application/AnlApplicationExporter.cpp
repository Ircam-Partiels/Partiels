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
                    auto const itemId = mPropertyItem.entry.getSelectedId();
                    mPropertyIgnoreGrids.setEnabled(itemId % groupItemFactor == 0);
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
    addChildComponent(mPropertyIgnoreGrids);
    addAndMakeVisible(mPropertyExport);
    addAndMakeVisible(mLoadingCircle);

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

    if(mProcess.valid())
    {
        mProcess.get();
    }
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
    mLoadingCircle.setBounds(bounds.removeFromTop(22).withSizeKeepingCentre(22, 22));
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
    auto const& documentAcsr = Instance::get().getDocumentAccessor();
    auto const identifier = getSelectedIdentifier();
    auto const format = magic_enum::enum_value<Format>(static_cast<size_t>(mPropertyFormat.entry.getSelectedItemIndex()));
    if(format == Format::JPEG || format == Format::PNG)
    {
        auto const width = static_cast<int>(mPropertyWidth.entry.getValue());
        auto const height = static_cast<int>(mPropertyHeight.entry.getValue());
        auto const autoSize = mPropertyAutoSizeMode.entry.getToggleState();
        auto const groupMode = mPropertyGroupMode.entry.getToggleState();

        auto const useDirectory = identifier.isEmpty() || (!groupMode && Document::Tools::hasGroupAcsr(documentAcsr, identifier));

        juce::FileChooser fc(juce::translate("Export as image"), {}, ((format == Format::JPEG) ? "*.jpeg;*.jpg" : "*.png"));
        auto const fcresult = useDirectory ? fc.browseForDirectory() : fc.browseForFileToSave(true);
        if(!fcresult)
        {
            return;
        }

        anlStrongAssert(!mProcess.valid());
        if(mProcess.valid())
        {
            return;
        }

        mLoadingCircle.setActive(true);
        juce::MouseCursor::showWaitCursor();
        setEnabled(false);
        mFloatingWindow.setCanBeClosedByUser(false);
        
        mProcess = std::async([=, this, file = fc.getResult()]() -> std::tuple<AlertWindow::MessageType, juce::String, juce::String>
                              {
                                  auto const result = exportToImage(file, format, identifier, autoSize, width, height, groupMode);
                                  triggerAsyncUpdate();
                                  if(result.failed())
                                  {
                                      return std::make_tuple(AlertWindow::MessageType::warning, juce::translate("Export as image failed!"), result.getErrorMessage());
                                  }
                                  return std::make_tuple(AlertWindow::MessageType::info, juce::translate("Export as image succeeded!"), juce::translate("The analyses have been exported as image(s) to FILENAME.").replace("FILENAME", file.getFullPathName()));
                              });
    }
    else
    {
        auto const ignoreGrids = mPropertyIgnoreGrids.entry.getToggleState();
        auto const formatName = juce::String(std::string(magic_enum::enum_name(format)));
        auto const formatWilcard = "*." + formatName.toLowerCase();

        auto const useDirectory = identifier.isEmpty() || Document::Tools::hasGroupAcsr(documentAcsr, identifier);

        juce::FileChooser fc(juce::translate("Export as FORMATNAME").replace("FORMATNAME", formatName), {}, formatWilcard);
        auto const fcresult = useDirectory ? fc.browseForDirectory() : fc.browseForFileToSave(true);
        if(!fcresult)
        {
            return;
        }

        anlStrongAssert(!mProcess.valid());
        if(mProcess.valid())
        {
            return;
        }

        mLoadingCircle.setActive(true);
        juce::MouseCursor::showWaitCursor();
        setEnabled(false);
        mFloatingWindow.setCanBeClosedByUser(false);
        
        mProcess = std::async([=, this, file = fc.getResult()]() -> std::tuple<AlertWindow::MessageType, juce::String, juce::String>
                              {
                                  juce::Thread::setCurrentThreadName("Exporter");
                                  auto const result = exportToText(file, format, identifier, ignoreGrids);
                                  triggerAsyncUpdate();
                                  if(result.failed())
                                  {
                                      return std::make_tuple(AlertWindow::MessageType::warning, juce::translate("Export as FORMATNAME failed!").replace("FORMATNAME", formatName), result.getErrorMessage());
                                  }
                                  return std::make_tuple(AlertWindow::MessageType::info, juce::translate("Export as FORMATNAME succeeded!").replace("FORMATNAME", formatName), juce::translate("The analyses have been exported as FORMATNAME to FILENAME.").replace("FORMATNAME", formatName).replace("FILENAME", file.getFullPathName()));
                              });
    }
}

void Application::Exporter::handleAsyncUpdate()
{
    mFloatingWindow.setCanBeClosedByUser(true);
    mLoadingCircle.setActive(false);
    juce::MouseCursor::hideWaitCursor();
    setEnabled(true);
    anlStrongAssert(mProcess.valid());
    if(!mProcess.valid())
    {
        return;
    }
    auto const result = mProcess.get();
    AlertWindow::showMessage(std::get<0>(result), std::get<1>(result), std::get<2>(result));
}

juce::Result Application::Exporter::exportToImage(juce::File const file, Format format, juce::String const& identifier, bool autoSize, int width, int height, bool groupMode)
{
    anlStrongAssert(format == Format::JPEG || format == Format::PNG);
    if(format != Format::JPEG && format != Format::PNG)
    {
        return juce::Result::fail("Invalid format");
    }
    if(file == juce::File())
    {
        return juce::Result::fail("Invalid file");
    }
    auto const extension = juce::String(std::string(magic_enum::enum_name(format))).toLowerCase();
    auto& documentAcsr = Instance::get().getDocumentAccessor();
    auto exportTrack = [&](juce::String const& trackIdentifier, juce::File const& trackFile)
    {
        juce::MessageManager::Lock lock;
        if(!lock.tryEnter())
        {
            return juce::Result::fail("Invalid threaded threadsafe access");
        }
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
        auto& timeZoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
        lock.exit();

        return Track::Exporter::toImage(trackAcsr, timeZoomAcsr, trackFile, trackWidth, trackheight);
    };

    auto exportGroup = [&](juce::String const& groupIdentifier, juce::File const& groupFile)
    {
        juce::MessageManager::Lock lock;
        if(!lock.tryEnter())
        {
            return juce::Result::fail("Invalid threaded threadsafe access");
        }
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
        auto& timeZoomAcsr = documentAcsr.getAcsr<Document::AcsrType::timeZoom>();
        lock.exit();

        return Group::Exporter::toImage(groupAcsr, timeZoomAcsr, groupFile, groupWidth, groupheight);
    };

    auto exportGroupTracks = [&](juce::String const& groupIdentifier, juce::File const& groupFolder)
    {
        juce::MessageManager::Lock lock;
        if(!lock.tryEnter())
        {
            return juce::Result::fail("Invalid threaded threadsafe access");
        }
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
        auto const groupName = juce::File::createLegalFileName(groupAcsr.getAttr<Group::AttrType::name>());
        lock.exit();

        for(auto const& trackIdentifier : groupLayout)
        {
            if(!lock.tryEnter())
            {
                return juce::Result::fail("Invalid threaded threadsafe access");
            }
            auto const documentHasTrack = Document::Tools::hasTrackAcsr(documentAcsr, trackIdentifier);
            anlStrongAssert(documentHasTrack);
            if(!documentHasTrack)
            {
                return juce::Result::fail("Track is invalid");
            }
            auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, trackIdentifier);
            auto const trackName = juce::File::createLegalFileName(trackAcsr.getAttr<Track::AttrType::name>());
            lock.exit();

            auto const trackFile = groupFolder.getNonexistentChildFile(groupName + "_" + trackName, "." + extension);
            auto const result = exportTrack(trackIdentifier, trackFile);
            if(result.failed())
            {
                return result;
            }
        }
        return juce::Result::ok();
    };

    auto exportDocumentGroups = [&](juce::File const& documentFolder)
    {
        juce::MessageManager::Lock lock;
        if(!lock.tryEnter())
        {
            return juce::Result::fail("Invalid threaded threadsafe access");
        }
        auto const documentLayout = copy_with_erased_if(documentAcsr.getAttr<Document::AttrType::layout>(), [&](auto const& groupId)
                                                        {
                                                            return !Document::Tools::hasGroupAcsr(documentAcsr, groupId);
                                                        });
        lock.exit();

        for(auto const& groupIdentifier : documentLayout)
        {
            if(!lock.tryEnter())
            {
                return juce::Result::fail("Invalid threaded threadsafe access");
            }
            auto const documentHasGroup = Document::Tools::hasGroupAcsr(documentAcsr, groupIdentifier);
            anlStrongAssert(documentHasGroup);
            if(!documentHasGroup)
            {
                return juce::Result::fail("Group is invalid");
            }

            auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupIdentifier);
            auto const groupName = juce::File::createLegalFileName(groupAcsr.getAttr<Group::AttrType::name>());
            lock.exit();

            auto const groupFile = documentFolder.getNonexistentChildFile(groupName, "." + extension);
            auto const result = exportGroup(groupIdentifier, groupFile);
            if(result.failed())
            {
                return result;
            }
        }
        return juce::Result::ok();
    };

    juce::MessageManager::Lock lock;
    if(!lock.tryEnter())
    {
        return juce::Result::fail("Invalid threaded threadsafe access");
    }

    if(identifier.isEmpty())
    {
        if(groupMode)
        {
            lock.exit();
            return exportDocumentGroups(file);
        }
        else
        {
            auto const documentLayout = copy_with_erased_if(documentAcsr.getAttr<Document::AttrType::layout>(), [&](auto const& groupId)
                                                            {
                                                                return !Document::Tools::hasGroupAcsr(documentAcsr, groupId);
                                                            });
            lock.exit();

            for(auto const& groupIdentifier : documentLayout)
            {
                auto const result = exportGroupTracks(groupIdentifier, file);
                if(result.failed())
                {
                    return result;
                }
            }
            return juce::Result::ok();
        }
    }
    else if(Document::Tools::hasGroupAcsr(documentAcsr, identifier))
    {
        lock.exit();
        return groupMode ? exportGroup(identifier, file) : exportGroupTracks(identifier, file);
    }
    else if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
    {
        lock.exit();
        return exportTrack(identifier, file);
    }
    return juce::Result::fail("Invalid identifier");
}

juce::Result Application::Exporter::exportToText(juce::File const file, Format format, juce::String const& identifier, bool ignoreGrids)
{
    auto& documentAcsr = Instance::get().getDocumentAccessor();
    auto const formatExtension = juce::String(std::string(magic_enum::enum_name(format))).toLowerCase();
    if(file == juce::File())
    {
        return juce::Result::fail("Invalid file");
    }

    auto exportTrack = [&](juce::String const& trackIdentifier, juce::File const& trackFile)
    {
        juce::MessageManager::Lock lock;
        if(!lock.tryEnter())
        {
            return juce::Result::fail("Invalid threaded access to model");
        }
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
        lock.exit();

        switch(format)
        {
            case Format::JPEG:
                return juce::Result::fail("Unsupported format");
            case Format::PNG:
                return juce::Result::fail("Unsupported format");
            case Format::CSV:
                return Track::Exporter::toCsv(trackAcsr, trackFile);
            case Format::JSON:
                return Track::Exporter::toJson(trackAcsr, trackFile);
        }
        return juce::Result::fail("Unsupported format");
    };

    auto exportGroupTracks = [&](juce::String const& groupIdentifier, juce::File const& groupFolder)
    {
        juce::MessageManager::Lock lock;
        if(!lock.tryEnter())
        {
            return juce::Result::fail("Invalid threaded threadsafe access");
        }
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
        auto const groupName = juce::File::createLegalFileName(groupAcsr.getAttr<Group::AttrType::name>());
        lock.exit();

        for(auto const& trackIdentifier : groupLayout)
        {
            if(!lock.tryEnter())
            {
                return juce::Result::fail("Invalid threaded threadsafe access");
            }
            auto const documentHasTrack = Document::Tools::hasTrackAcsr(documentAcsr, trackIdentifier);
            anlStrongAssert(documentHasTrack);
            if(!documentHasTrack)
            {
                return juce::Result::fail("Track is invalid");
            }
            auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, trackIdentifier);
            auto const trackName = juce::File::createLegalFileName(trackAcsr.getAttr<Track::AttrType::name>());
            lock.exit();

            auto const trackFile = groupFolder.getNonexistentChildFile(groupName + "_" + trackName, "." + formatExtension);
            auto const result = exportTrack(trackIdentifier, trackFile);
            if(result.failed())
            {
                return result;
            }
        }
        return juce::Result::ok();
    };

    juce::MessageManager::Lock lock;
    if(!lock.tryEnter())
    {
        return juce::Result::fail("Invalid threaded threadsafe access");
    }

    if(identifier.isEmpty())
    {
        auto const documentLayout = copy_with_erased_if(documentAcsr.getAttr<Document::AttrType::layout>(), [&](auto const& groupId)
                                                        {
                                                            return !Document::Tools::hasGroupAcsr(documentAcsr, groupId);
                                                        });

        lock.exit();

        for(auto const& groupIdentifier : documentLayout)
        {
            auto const result = exportGroupTracks(groupIdentifier, file);
            if(result.failed())
            {
                return result;
            }
        }
        return juce::Result::ok();
    }
    else if(Document::Tools::hasGroupAcsr(documentAcsr, identifier))
    {
        lock.exit();
        return exportGroupTracks(identifier, file);
    }
    else if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
    {
        lock.exit();
        return exportTrack(identifier, file);
    }
    return juce::Result::fail("Invalid identifier");
}

ANALYSE_FILE_END
