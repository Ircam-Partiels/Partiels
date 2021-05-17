#include "AnlApplicationExporter.h"
#include "../Document/AnlDocumentTools.h"
#include "../Group/AnlGroupExporter.h"
#include "../Track/AnlTrackExporter.h"
#include "../Track/AnlTrackTools.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Exporter::Exporter()
: FloatingWindowContainer("Exporter", *this)
, mPropertyItem("Item", "The item to export", "", std::vector<std::string>{""}, [this](size_t index)
                {
                    juce::ignoreUnused(index);
                    sanitizeProperties(true);
                })
, mPropertyFormat("Format", "Select the export format", "", std::vector<std::string>{"JPEG", "PNG", "CSV", "JSON"}, [](size_t index)
                  {
                      auto& acsr = Instance::get().getApplicationAccessor();
                      auto options = acsr.getAttr<AttrType::exportOptions>();
                      options.format = magic_enum::enum_value<ExportOptions::Format>(index);
                      acsr.setAttr<AttrType::exportOptions>(options, NotificationType::synchronous);
                  })
, mPropertyGroupMode("Preserve group overlay", "Preserve group overlay of tracks when exporting", [](bool state)
                     {
                         auto& acsr = Instance::get().getApplicationAccessor();
                         auto options = acsr.getAttr<AttrType::exportOptions>();
                         options.useGroupOverview = state;
                         acsr.setAttr<AttrType::exportOptions>(options, NotificationType::synchronous);
                     })
, mPropertyAutoSizeMode("Preserve item size", "Preserve the size of the track or the group", [](bool state)
                        {
                            auto& acsr = Instance::get().getApplicationAccessor();
                            auto options = acsr.getAttr<AttrType::exportOptions>();
                            options.useAutoSize = state;
                            acsr.setAttr<AttrType::exportOptions>(options, NotificationType::synchronous);
                        })
, mPropertyWidth("Image Width", "Set the width of the image", "pixel", juce::Range<float>{1.0f, 100000000}, 1.0f, [](float value)
                 {
                     auto& acsr = Instance::get().getApplicationAccessor();
                     auto options = acsr.getAttr<AttrType::exportOptions>();
                     options.imageWidth = std::max(static_cast<int>(std::round(value)), 1);
                     acsr.setAttr<AttrType::exportOptions>(options, NotificationType::synchronous);
                 })
, mPropertyHeight("Image Height", "Set the height of the image", "pixel", juce::Range<float>{1.0f, 100000000}, 1.0f, [](float value)
                  {
                      auto& acsr = Instance::get().getApplicationAccessor();
                      auto options = acsr.getAttr<AttrType::exportOptions>();
                      options.imageHeight = std::max(static_cast<int>(std::round(value)), 1);
                      acsr.setAttr<AttrType::exportOptions>(options, NotificationType::synchronous);
                  })
, mPropertyRawHeader("Header Row", "Include header row before the data rows", [](bool state)
                     {
                         auto& acsr = Instance::get().getApplicationAccessor();
                         auto options = acsr.getAttr<AttrType::exportOptions>();
                         options.includeHeaderRaw = state;
                         acsr.setAttr<AttrType::exportOptions>(options, NotificationType::synchronous);
                     })
, mPropertyRawSeparator("Column Separator", "The seperatror character between colummns", "", std::vector<std::string>{"Comma", "Space", "Tab", "Pipe", "Slash", "Colon"}, [](size_t index)
                        {
                            auto& acsr = Instance::get().getApplicationAccessor();
                            auto options = acsr.getAttr<AttrType::exportOptions>();
                            options.columnSeparator = magic_enum::enum_value<ExportOptions::ColumnSeparator>(index);
                            acsr.setAttr<AttrType::exportOptions>(options, NotificationType::synchronous);
                        })
, mPropertyIgnoreGrids("Ignore Grid Tracks", "Ignore tracks with grid results", [](bool state)
                       {
                           auto& acsr = Instance::get().getApplicationAccessor();
                           auto options = acsr.getAttr<AttrType::exportOptions>();
                           options.ignoreGridResults = state;
                           acsr.setAttr<AttrType::exportOptions>(options, NotificationType::synchronous);
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
    addChildComponent(mPropertyRawHeader);
    addChildComponent(mPropertyRawSeparator);
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

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::windowState:
            case AttrType::recentlyOpenedFilesList:
            case AttrType::currentDocumentFile:
            case AttrType::colourMode:
            case AttrType::showInfoBubble:
                break;
            case AttrType::exportOptions:
            {
                auto const options = acsr.getAttr<AttrType::exportOptions>();

                auto constexpr notification = juce::NotificationType::dontSendNotification;
                mPropertyFormat.entry.setSelectedItemIndex(static_cast<int>(options.format), notification);
                mPropertyGroupMode.entry.setToggleState(options.useGroupOverview, notification);
                mPropertyAutoSizeMode.entry.setToggleState(options.useAutoSize, notification);
                mPropertyWidth.entry.setValue(static_cast<double>(options.imageWidth), notification);
                mPropertyHeight.entry.setValue(static_cast<double>(options.imageHeight), notification);
                mPropertyRawHeader.entry.setToggleState(options.includeHeaderRaw, notification);
                mPropertyRawSeparator.entry.setSelectedItemIndex(static_cast<int>(options.columnSeparator), notification);
                mPropertyIgnoreGrids.entry.setToggleState(options.ignoreGridResults, notification);

                mPropertyGroupMode.setVisible(options.useImageFormat());
                mPropertyAutoSizeMode.setVisible(options.useImageFormat());
                mPropertyWidth.setVisible(options.useImageFormat());
                mPropertyHeight.setVisible(options.useImageFormat());
                mPropertyRawHeader.setVisible(options.format == ExportOptions::Format::csv);
                mPropertyRawSeparator.setVisible(options.format == ExportOptions::Format::csv);
                mPropertyIgnoreGrids.setVisible(options.useTextFormat());
                sanitizeProperties(false);
                resized();
            }
            break;
        }
    };

    auto& documentAcsr = Instance::get().getDocumentAccessor();
    documentAcsr.addListener(mDocumentListener, NotificationType::synchronous);
    auto& acsr = Instance::get().getApplicationAccessor();
    acsr.addListener(mListener, NotificationType::synchronous);
    setSize(300, 200);
}

Application::Exporter::~Exporter()
{
    auto& acsr = Instance::get().getApplicationAccessor();
    acsr.removeListener(mListener);
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
    setBounds(mPropertyRawHeader);
    setBounds(mPropertyRawSeparator);
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

void Application::Exporter::sanitizeProperties(bool updateModel)
{
    auto& acsr = Instance::get().getApplicationAccessor();
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
        if(updateModel)
        {
            auto options = acsr.getAttr<AttrType::exportOptions>();
            options.useGroupOverview = false;
            acsr.setAttr<AttrType::exportOptions>(options, NotificationType::synchronous);
        }
    }

    auto const autoSize = acsr.getAttr<AttrType::exportOptions>().useAutoSize;
    mPropertyWidth.setEnabled(!autoSize);
    mPropertyHeight.setEnabled(!autoSize);
    if(updateModel && autoSize && !itemIsDocument)
    {
        auto const size = getSizeFor(identifier);
        auto options = acsr.getAttr<AttrType::exportOptions>();
        options.imageWidth = std::get<0>(size);
        options.imageHeight = std::get<1>(size);
        acsr.setAttr<AttrType::exportOptions>(options, NotificationType::synchronous);
    }

    auto const itemId = mPropertyItem.entry.getSelectedId();
    mPropertyIgnoreGrids.setEnabled(itemId % groupItemFactor == 0);
}

void Application::Exporter::exportToFile()
{
    auto const& acsr = Instance::get().getApplicationAccessor();
    auto const& options = acsr.getAttr<AttrType::exportOptions>();
    auto const& documentAcsr = Instance::get().getDocumentAccessor();
    auto const identifier = getSelectedIdentifier();

    juce::FileChooser fc(juce::translate("Export as FORMATNAME").replace("FORMATNAME", options.getFormatName()), {}, options.getFormatWilcard());
    auto const useDirectory = identifier.isEmpty() || (Document::Tools::hasGroupAcsr(documentAcsr, identifier) && (!options.useGroupOverview || options.useTextFormat()));
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
                              auto const result = exportTo(file, identifier, options);
                              triggerAsyncUpdate();
                              if(result.failed())
                              {
                                  return std::make_tuple(AlertWindow::MessageType::warning, juce::translate("Export as FORMATNAME failed!").replace("FORMATNAME", options.getFormatName()), result.getErrorMessage());
                              }
                              return std::make_tuple(AlertWindow::MessageType::info, juce::translate("Export as FORMATNAME succeeded!").replace("FORMATNAME", options.getFormatName()), juce::translate("The analyses have been exported as FORMATNAME to FILENAME.").replace("FORMATNAME", options.getFormatName()).replace("FILENAME", file.getFullPathName()));
                          });
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

juce::Result Application::Exporter::exportToImage(juce::File const file, juce::String const& identifier, ExportOptions const& options)
{
    anlStrongAssert(options.useImageFormat());
    if(!options.useImageFormat())
    {
        return juce::Result::fail("Invalid format");
    }
    if(file == juce::File())
    {
        return juce::Result::fail("Invalid file");
    }

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
        auto const trackWidth = options.useAutoSize ? std::get<0>(size) : options.imageWidth;
        auto const trackheight = options.useAutoSize ? std::get<1>(size) : options.imageHeight;
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
        auto const groupWidth = options.useAutoSize ? std::get<0>(size) : options.imageWidth;
        auto const groupheight = options.useAutoSize ? std::get<1>(size) : options.imageHeight;
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

            auto const trackFile = groupFolder.getNonexistentChildFile(groupName + "_" + trackName, "." + options.getFormatExtension());
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

            auto const groupFile = documentFolder.getNonexistentChildFile(groupName, "." + options.getFormatExtension());
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
        if(options.useGroupOverview)
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
        return options.useGroupOverview ? exportGroup(identifier, file) : exportGroupTracks(identifier, file);
    }
    else if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
    {
        lock.exit();
        return exportTrack(identifier, file);
    }
    return juce::Result::fail("Invalid identifier");
}

juce::Result Application::Exporter::exportToText(juce::File const file, juce::String const& identifier, ExportOptions const& options)
{
    anlStrongAssert(options.useTextFormat());
    if(!options.useTextFormat())
    {
        return juce::Result::fail("Invalid format");
    }
    if(file == juce::File())
    {
        return juce::Result::fail("Invalid file");
    }

    auto& documentAcsr = Instance::get().getDocumentAccessor();
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
        if(options.ignoreGridResults && Track::Tools::getDisplayType(trackAcsr) == Track::Tools::DisplayType::grid)
        {
            return juce::Result::ok();
        }
        lock.exit();

        switch(options.format)
        {
            case ExportOptions::Format::jpeg:
                return juce::Result::fail("Unsupported format");
            case ExportOptions::Format::png:
                return juce::Result::fail("Unsupported format");
            case ExportOptions::Format::csv:
                return Track::Exporter::toCsv(trackAcsr, trackFile, options.includeHeaderRaw, options.getSeparatorChar());
            case ExportOptions::Format::json:
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

            auto const trackFile = groupFolder.getNonexistentChildFile(groupName + "_" + trackName, "." + options.getFormatExtension());
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

juce::Result Application::Exporter::exportTo(juce::File const file, juce::String const& identifier, ExportOptions const& options)
{
    if(options.useTextFormat())
    {
        return exportToText(file, identifier, options);
    }
    return exportToImage(file, identifier, options);
}

ANALYSE_FILE_END
