#include "AnlDocumentExporter.h"
#include "../Group/AnlGroupExporter.h"
#include "../Track/AnlTrackExporter.h"
#include "../Track/AnlTrackTools.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

bool Document::Exporter::Options::operator==(Options const& rhd) const noexcept
{
    return format == rhd.format &&
           useGroupOverview == rhd.useGroupOverview &&
           useAutoSize == rhd.useAutoSize &&
           imageWidth == rhd.imageWidth &&
           imageHeight == rhd.imageHeight &&
           includeHeaderRaw == rhd.includeHeaderRaw &&
           ignoreGridResults == rhd.ignoreGridResults &&
           columnSeparator == rhd.columnSeparator;
}

bool Document::Exporter::Options::operator!=(Options const& rhd) const noexcept
{
    return !(*this == rhd);
}

bool Document::Exporter::Options::useImageFormat() const noexcept
{
    return format == Format::jpeg || format == Format::png;
}

bool Document::Exporter::Options::useTextFormat() const noexcept
{
    return !useImageFormat();
}

juce::String Document::Exporter::Options::getFormatName() const
{
    return juce::String(std::string(magic_enum::enum_name(format)));
}

juce::String Document::Exporter::Options::getFormatExtension() const
{
    return getFormatName().toLowerCase();
}

juce::String Document::Exporter::Options::getFormatWilcard() const
{
    if(format == Format::jpeg)
    {
        return "*.jpeg;*.jpg";
    }
    return "*." + getFormatExtension();
}

char Document::Exporter::Options::getSeparatorChar() const
{
    switch(columnSeparator)
    {
        case ColumnSeparator::comma:
        {
            return ',';
        }
        case ColumnSeparator::space:
        {
            return ' ';
        }
        case ColumnSeparator::tab:
        {
            return '\t';
        }
        case ColumnSeparator::pipe:
        {
            return '|';
        }
        case ColumnSeparator::slash:
        {
            return '/';
        }
        case ColumnSeparator::colon:
        {
            return ':';
        }
        default:
        {
            return ',';
        }
    }
}

Document::Exporter::Panel::Panel(Accessor& accessor, GetSizeFn getSizeFor)
: mAccessor(accessor)
, mGetSizeForFn(getSizeFor)
, mPropertyItem("Item", "The item to export", "", std::vector<std::string>{""}, [this](size_t index)
                {
                    juce::ignoreUnused(index);
                    sanitizeProperties(true);
                })
, mPropertyFormat("Format", "Select the export format", "", std::vector<std::string>{"JPEG", "PNG", "CSV", "JSON"}, [this](size_t index)
                  {
                      auto options = mOptions;
                      options.format = magic_enum::enum_value<Document::Exporter::Options::Format>(index);
                      setOptions(options, juce::NotificationType::sendNotificationSync);
                  })
, mPropertyGroupMode("Preserve group overlay", "Preserve group overlay of tracks when exporting", [this](bool state)
                     {
                         auto options = mOptions;
                         options.useGroupOverview = state;
                         setOptions(options, juce::NotificationType::sendNotificationSync);
                     })
, mPropertyAutoSizeMode("Preserve item size", "Preserve the size of the track or the group", [this](bool state)
                        {
                            auto options = mOptions;
                            options.useAutoSize = state;
                            setOptions(options, juce::NotificationType::sendNotificationSync);
                        })
, mPropertyWidth("Image Width", "Set the width of the image", "pixel", juce::Range<float>{1.0f, 100000000}, 1.0f, [this](float value)
                 {
                     auto options = mOptions;
                     options.imageWidth = std::max(static_cast<int>(std::round(value)), 1);
                     setOptions(options, juce::NotificationType::sendNotificationSync);
                 })
, mPropertyHeight("Image Height", "Set the height of the image", "pixel", juce::Range<float>{1.0f, 100000000}, 1.0f, [this](float value)
                  {
                      auto options = mOptions;
                      options.imageHeight = std::max(static_cast<int>(std::round(value)), 1);
                      setOptions(options, juce::NotificationType::sendNotificationSync);
                  })
, mPropertyRawHeader("Header Row", "Include header row before the data rows", [this](bool state)
                     {
                         auto options = mOptions;
                         options.includeHeaderRaw = state;
                         setOptions(options, juce::NotificationType::sendNotificationSync);
                     })
, mPropertyRawSeparator("Column Separator", "The seperatror character between colummns", "", std::vector<std::string>{"Comma", "Space", "Tab", "Pipe", "Slash", "Colon"}, [this](size_t index)
                        {
                            auto options = mOptions;
                            options.columnSeparator = magic_enum::enum_value<Document::Exporter::Options::ColumnSeparator>(index);
                            setOptions(options, juce::NotificationType::sendNotificationSync);
                        })
, mPropertyIgnoreGrids("Ignore Grid Tracks", "Ignore tracks with grid results", [this](bool state)
                       {
                           auto options = mOptions;
                           options.ignoreGridResults = state;
                           setOptions(options, juce::NotificationType::sendNotificationSync);
                       })
, mDocumentLayoutNotifier(typeid(*this).name(), mAccessor, [this]()
                          {
                              updateItems();
                          })
{
    addAndMakeVisible(mPropertyItem);
    addAndMakeVisible(mPropertyFormat);
    addAndMakeVisible(mPropertyGroupMode);
    if(mGetSizeForFn != nullptr)
    {
        addAndMakeVisible(mPropertyAutoSizeMode);
        addAndMakeVisible(mPropertyWidth);
        addAndMakeVisible(mPropertyHeight);
    }
    addChildComponent(mPropertyRawHeader);
    addChildComponent(mPropertyRawSeparator);
    addChildComponent(mPropertyIgnoreGrids);

    setSize(300, 200);
}

void Document::Exporter::Panel::resized()
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
    setSize(bounds.getWidth(), bounds.getY() + 2);
}

juce::String Document::Exporter::Panel::getSelectedIdentifier() const
{
    auto const itemId = mPropertyItem.entry.getSelectedId();
    if(itemId % documentItemFactor == 0)
    {
        return {};
    }

    auto const documentLayout = copy_with_erased_if(mAccessor.getAttr<AttrType::layout>(), [&](auto const& groupId)
                                                    {
                                                        return !Document::Tools::hasGroupAcsr(mAccessor, groupId);
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
    anlWeakAssert(Document::Tools::hasGroupAcsr(mAccessor, groupId));
    if(!Document::Tools::hasGroupAcsr(mAccessor, groupId))
    {
        return {};
    }

    auto const& groupAcsr = Document::Tools::getGroupAcsr(mAccessor, groupId);
    auto const groupLayout = copy_with_erased_if(groupAcsr.getAttr<Group::AttrType::layout>(), [&](auto const& trackId)
                                                 {
                                                     return !Document::Tools::hasTrackAcsr(mAccessor, trackId);
                                                 });
    auto const trackIndex = static_cast<size_t>((itemId % groupItemFactor) - 1);
    anlWeakAssert(trackIndex < groupLayout.size());
    if(trackIndex >= groupLayout.size())
    {
        return {};
    }
    return groupLayout[trackIndex];
}

void Document::Exporter::Panel::updateItems()
{
    auto const selectedId = getSelectedIdentifier();

    auto& entryBox = mPropertyItem.entry;
    entryBox.clear(juce::NotificationType::dontSendNotification);
    entryBox.addItem(juce::translate("All (Document)"), documentItemFactor);
    auto const documentLayout = copy_with_erased_if(mAccessor.getAttr<Document::AttrType::layout>(), [&](auto const& groupId)
                                                    {
                                                        return !Document::Tools::hasGroupAcsr(mAccessor, groupId);
                                                    });

    for(size_t documentLayoutIndex = 0; documentLayoutIndex < documentLayout.size(); ++documentLayoutIndex)
    {
        auto const& groupId = documentLayout[documentLayoutIndex];
        auto const groupItemId = groupItemFactor * static_cast<int>(documentLayoutIndex + 1_z) + documentItemFactor;
        anlWeakAssert(Document::Tools::hasGroupAcsr(mAccessor, groupId));
        if(Document::Tools::hasGroupAcsr(mAccessor, groupId))
        {
            auto const& groupAcsr = Document::Tools::getGroupAcsr(mAccessor, groupId);
            auto const& groupName = groupAcsr.getAttr<Group::AttrType::name>();
            entryBox.addItem(juce::translate("GROUPNAME (Group)").replace("GROUPNAME", groupName), groupItemId);
            if(groupId == selectedId)
            {
                entryBox.setSelectedId(groupItemId);
            }

            auto const groupLayout = copy_with_erased_if(groupAcsr.getAttr<Group::AttrType::layout>(), [&](auto const& trackId)
                                                         {
                                                             return !Document::Tools::hasTrackAcsr(mAccessor, trackId);
                                                         });

            for(size_t groupLayoutIndex = 0; groupLayoutIndex < groupLayout.size(); ++groupLayoutIndex)
            {
                auto const& trackId = groupLayout[groupLayoutIndex];
                auto const trackItemId = groupItemId + static_cast<int>(groupLayoutIndex + 1_z);
                anlWeakAssert(Document::Tools::hasTrackAcsr(mAccessor, trackId));
                if(Document::Tools::hasTrackAcsr(mAccessor, trackId))
                {
                    auto const& trackAcsr = Document::Tools::getTrackAcsr(mAccessor, trackId);
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

void Document::Exporter::Panel::sanitizeProperties(bool updateModel)
{
    auto const identifier = getSelectedIdentifier();
    auto const itemIsDocument = identifier.isEmpty();
    auto const itemIsGroup = Document::Tools::hasGroupAcsr(mAccessor, identifier);
    auto const itemIsTrack = Document::Tools::hasTrackAcsr(mAccessor, identifier);
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
            auto options = mOptions;
            options.useGroupOverview = false;
            setOptions(options, juce::NotificationType::sendNotificationSync);
        }
    }

    auto const autoSize = mOptions.useAutoSize;
    mPropertyWidth.setEnabled(!autoSize);
    mPropertyHeight.setEnabled(!autoSize);
    if(mGetSizeForFn != nullptr && updateModel && autoSize && !itemIsDocument)
    {
        auto const size = mGetSizeForFn(identifier);
        auto options = mOptions;
        options.imageWidth = std::get<0>(size);
        options.imageHeight = std::get<1>(size);
        setOptions(options, juce::NotificationType::sendNotificationSync);
    }

    auto const itemId = mPropertyItem.entry.getSelectedId();
    mPropertyIgnoreGrids.setEnabled(itemId % groupItemFactor == 0);
}

Document::Exporter::Options const& Document::Exporter::Panel::getOptions() const
{
    return mOptions;
}

void Document::Exporter::Panel::setOptions(Options const& options, juce::NotificationType notification)
{
    if(mOptions == options)
    {
        return;
    }
    mOptions = options;
    auto constexpr silent = juce::NotificationType::dontSendNotification;
    mPropertyFormat.entry.setSelectedItemIndex(static_cast<int>(options.format), silent);
    mPropertyGroupMode.entry.setToggleState(options.useGroupOverview, silent);
    mPropertyAutoSizeMode.entry.setToggleState(options.useAutoSize, silent);
    mPropertyWidth.entry.setValue(static_cast<double>(options.imageWidth), silent);
    mPropertyHeight.entry.setValue(static_cast<double>(options.imageHeight), silent);
    mPropertyRawHeader.entry.setToggleState(options.includeHeaderRaw, silent);
    mPropertyRawSeparator.entry.setSelectedItemIndex(static_cast<int>(options.columnSeparator), silent);
    mPropertyIgnoreGrids.entry.setToggleState(options.ignoreGridResults, silent);

    mPropertyGroupMode.setVisible(options.useImageFormat());
    mPropertyAutoSizeMode.setVisible(mGetSizeForFn != nullptr && options.useImageFormat());
    mPropertyWidth.setVisible(mGetSizeForFn != nullptr && options.useImageFormat());
    mPropertyHeight.setVisible(mGetSizeForFn != nullptr && options.useImageFormat());
    mPropertyRawHeader.setVisible(options.format == Document::Exporter::Options::Format::csv);
    mPropertyRawSeparator.setVisible(options.format == Document::Exporter::Options::Format::csv);
    mPropertyIgnoreGrids.setVisible(options.useTextFormat());
    sanitizeProperties(false);
    resized();

    if(notification == juce::NotificationType::dontSendNotification)
    {
        return;
    }
    else if(notification == juce::NotificationType::sendNotificationAsync)
    {
        triggerAsyncUpdate();
    }
    else
    {
        handleAsyncUpdate();
    }
}

void Document::Exporter::Panel::handleAsyncUpdate()
{
    if(onOptionsChanged != nullptr)
    {
        onOptionsChanged();
    }
}

juce::Result Document::Exporter::toFile(Accessor& accessor, juce::File const file, juce::String const filePrefix, juce::String const& identifier, Options const& options, std::atomic<bool> const& shouldAbort, GetSizeFn getSizeFor)
{
    if(file == juce::File())
    {
        return juce::Result::fail("Invalid file");
    }

    if(options.useImageFormat())
    {
        auto exportTrack = [&](juce::String const& trackIdentifier, juce::File const& trackFile)
        {
            juce::MessageManager::Lock lock;
            if(!lock.tryEnter())
            {
                return juce::Result::fail("Invalid threaded access");
            }
            auto const documentHasTrack = Tools::hasTrackAcsr(accessor, trackIdentifier);
            anlStrongAssert(documentHasTrack);
            if(!documentHasTrack)
            {
                return juce::Result::fail("Track is invalid");
            }

            anlStrongAssert(!options.useAutoSize || getSizeFor != nullptr);
            if(options.useAutoSize && getSizeFor == nullptr)
            {
                return juce::Result::fail("Invalid size method");
            }
            auto const size = options.useAutoSize ? getSizeFor(trackIdentifier) : std::make_pair(options.imageWidth, options.imageHeight);
            auto& trackAcsr = Tools::getTrackAcsr(accessor, trackIdentifier);
            auto& timeZoomAcsr = accessor.getAcsr<AcsrType::timeZoom>();
            auto const fileUsed = trackFile.isDirectory() ? trackFile.getNonexistentChildFile(filePrefix + trackAcsr.getAttr<Track::AttrType::name>(), "." + options.getFormatExtension()) : trackFile.getSiblingFile(filePrefix + trackFile.getFileName());
            lock.exit();

            return Track::Exporter::toImage(trackAcsr, timeZoomAcsr, fileUsed, std::get<0>(size), std::get<1>(size), shouldAbort);
        };

        auto exportGroup = [&](juce::String const& groupIdentifier, juce::File const& groupFile)
        {
            juce::MessageManager::Lock lock;
            if(!lock.tryEnter())
            {
                return juce::Result::fail("Invalid threaded access");
            }
            auto const documentHasGroup = Tools::hasGroupAcsr(accessor, groupIdentifier);
            anlStrongAssert(documentHasGroup);
            if(!documentHasGroup)
            {
                return juce::Result::fail("Group is invalid");
            }
            anlStrongAssert(!options.useAutoSize || getSizeFor != nullptr);
            if(options.useAutoSize && getSizeFor == nullptr)
            {
                return juce::Result::fail("Invalid size method");
            }
            auto const size = options.useAutoSize ? getSizeFor(groupIdentifier) : std::make_pair(options.imageWidth, options.imageHeight);
            auto& groupAcsr = Tools::getGroupAcsr(accessor, groupIdentifier);
            auto& timeZoomAcsr = accessor.getAcsr<AcsrType::timeZoom>();
            auto const fileUsed = groupFile.isDirectory() ? groupFile.getNonexistentChildFile(filePrefix + groupAcsr.getAttr<Group::AttrType::name>(), "." + options.getFormatExtension()) : groupFile.getSiblingFile(filePrefix + groupFile.getFileName());
            lock.exit();

            return Group::Exporter::toImage(groupAcsr, timeZoomAcsr, fileUsed, std::get<0>(size), std::get<1>(size), shouldAbort);
        };

        auto exportGroupTracks = [&](juce::String const& groupIdentifier, juce::File const& groupFolder)
        {
            juce::MessageManager::Lock lock;
            if(!lock.tryEnter())
            {
                return juce::Result::fail("Invalid threaded access");
            }
            auto const documentHasGroup = Tools::hasGroupAcsr(accessor, groupIdentifier);
            anlStrongAssert(documentHasGroup);
            if(!documentHasGroup)
            {
                return juce::Result::fail("Group is invalid");
            }

            auto const& groupAcsr = Tools::getGroupAcsr(accessor, groupIdentifier);
            auto const groupLayout = copy_with_erased_if(groupAcsr.getAttr<Group::AttrType::layout>(), [&](auto const& trackId)
                                                         {
                                                             return !Tools::hasTrackAcsr(accessor, trackId);
                                                         });
            auto const groupName = juce::File::createLegalFileName(groupAcsr.getAttr<Group::AttrType::name>());
            lock.exit();

            for(auto const& trackIdentifier : groupLayout)
            {
                if(!lock.tryEnter())
                {
                    return juce::Result::fail("Invalid threaded access");
                }
                auto const documentHasTrack = Tools::hasTrackAcsr(accessor, trackIdentifier);
                anlStrongAssert(documentHasTrack);
                if(!documentHasTrack)
                {
                    return juce::Result::fail("Track is invalid");
                }
                auto const& trackAcsr = Tools::getTrackAcsr(accessor, trackIdentifier);
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
                return juce::Result::fail("Invalid threaded access");
            }
            auto const documentLayout = copy_with_erased_if(accessor.getAttr<AttrType::layout>(), [&](auto const& groupId)
                                                            {
                                                                return !Tools::hasGroupAcsr(accessor, groupId);
                                                            });
            lock.exit();

            for(auto const& groupIdentifier : documentLayout)
            {
                if(!lock.tryEnter())
                {
                    return juce::Result::fail("Invalid threaded access");
                }
                auto const documentHasGroup = Tools::hasGroupAcsr(accessor, groupIdentifier);
                anlStrongAssert(documentHasGroup);
                if(!documentHasGroup)
                {
                    return juce::Result::fail("Group is invalid");
                }

                auto const& groupAcsr = Tools::getGroupAcsr(accessor, groupIdentifier);
                auto const groupName = juce::File::createLegalFileName(groupAcsr.getAttr<Group::AttrType::name>());
                lock.exit();

                auto const groupFile = documentFolder.getNonexistentChildFile(groupName, "." + options.getFormatExtension());
                auto const result = exportGroup(groupIdentifier, documentFolder);
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
            return juce::Result::fail("Invalid threaded access");
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
                auto const documentLayout = copy_with_erased_if(accessor.getAttr<AttrType::layout>(), [&](auto const& groupId)
                                                                {
                                                                    return !Tools::hasGroupAcsr(accessor, groupId);
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
        else if(Tools::hasGroupAcsr(accessor, identifier))
        {
            lock.exit();
            return options.useGroupOverview ? exportGroup(identifier, file) : exportGroupTracks(identifier, file);
        }
        else if(Tools::hasTrackAcsr(accessor, identifier))
        {
            lock.exit();
            return exportTrack(identifier, file);
        }
        return juce::Result::fail("Invalid identifier");
    }

    anlStrongAssert(options.useTextFormat());
    if(!options.useTextFormat())
    {
        return juce::Result::fail("Invalid format");
    }

    auto exportTrack = [&](juce::String const& trackIdentifier, juce::File const& trackFile)
    {
        juce::MessageManager::Lock lock;
        if(!lock.tryEnter())
        {
            return juce::Result::fail("Invalid threaded access to model");
        }
        auto const documentHasTrack = Tools::hasTrackAcsr(accessor, trackIdentifier);
        anlStrongAssert(documentHasTrack);
        if(!documentHasTrack)
        {
            return juce::Result::fail("Track is invalid");
        }
        auto& trackAcsr = Tools::getTrackAcsr(accessor, trackIdentifier);
        if(identifier != trackIdentifier && options.ignoreGridResults && Track::Tools::getDisplayType(trackAcsr) == Track::Tools::DisplayType::columns)
        {
            return juce::Result::ok();
        }
        auto const fileUsed = trackFile.isDirectory() ? trackFile.getNonexistentChildFile(filePrefix + trackAcsr.getAttr<Track::AttrType::name>(), "." + options.getFormatExtension()) : trackFile.getSiblingFile(filePrefix + trackFile.getFileName());
        lock.exit();

        switch(options.format)
        {
            case Options::Format::jpeg:
                return juce::Result::fail("Unsupported format");
            case Options::Format::png:
                return juce::Result::fail("Unsupported format");
            case Options::Format::csv:
                return Track::Exporter::toCsv(trackAcsr, fileUsed, options.includeHeaderRaw, options.getSeparatorChar(), shouldAbort);
            case Options::Format::json:
                return Track::Exporter::toJson(trackAcsr, fileUsed, shouldAbort);
        }
        return juce::Result::fail("Unsupported format");
    };

    auto exportGroupTracks = [&](juce::String const& groupIdentifier, juce::File const& groupFolder)
    {
        juce::MessageManager::Lock lock;
        if(!lock.tryEnter())
        {
            return juce::Result::fail("Invalid threaded access");
        }
        auto const documentHasGroup = Tools::hasGroupAcsr(accessor, groupIdentifier);
        anlStrongAssert(documentHasGroup);
        if(!documentHasGroup)
        {
            return juce::Result::fail("Group is invalid");
        }
        auto const& groupAcsr = Tools::getGroupAcsr(accessor, groupIdentifier);
        auto const groupLayout = copy_with_erased_if(groupAcsr.getAttr<Group::AttrType::layout>(), [&](auto const& trackId)
                                                     {
                                                         return !Tools::hasTrackAcsr(accessor, trackId);
                                                     });
        auto const groupName = juce::File::createLegalFileName(groupAcsr.getAttr<Group::AttrType::name>());
        lock.exit();

        for(auto const& trackIdentifier : groupLayout)
        {
            if(!lock.tryEnter())
            {
                return juce::Result::fail("Invalid threaded access");
            }
            auto const documentHasTrack = Tools::hasTrackAcsr(accessor, trackIdentifier);
            anlStrongAssert(documentHasTrack);
            if(!documentHasTrack)
            {
                return juce::Result::fail("Track is invalid");
            }
            auto const& trackAcsr = Tools::getTrackAcsr(accessor, trackIdentifier);
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
        return juce::Result::fail("Invalid threaded access");
    }

    if(identifier.isEmpty())
    {
        auto const documentLayout = copy_with_erased_if(accessor.getAttr<AttrType::layout>(), [&](auto const& groupId)
                                                        {
                                                            return !Tools::hasGroupAcsr(accessor, groupId);
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
    else if(Tools::hasGroupAcsr(accessor, identifier))
    {
        lock.exit();
        return exportGroupTracks(identifier, file);
    }
    else if(Tools::hasTrackAcsr(accessor, identifier))
    {
        lock.exit();
        return exportTrack(identifier, file);
    }
    return juce::Result::fail("Invalid identifier");
}

template <>
void XmlParser::toXml<Document::Exporter::Options>(juce::XmlElement& xml, juce::Identifier const& attributeName, Document::Exporter::Options const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "format", value.format);
        toXml(*child, "useGroupOverview", value.useGroupOverview);
        toXml(*child, "useAutoSize", value.useAutoSize);
        toXml(*child, "imageWidth", value.imageWidth);
        toXml(*child, "imageHeight", value.imageHeight);
        toXml(*child, "includeHeaderRaw", value.includeHeaderRaw);
        toXml(*child, "ignoreGridResults", value.ignoreGridResults);
        toXml(*child, "columnSeparator", value.columnSeparator);
        xml.addChildElement(child.release());
    }
}

template <>
auto XmlParser::fromXml<Document::Exporter::Options>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Document::Exporter::Options const& defaultValue)
    -> Document::Exporter::Options
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Document::Exporter::Options value;
    value.format = fromXml(*child, "format", defaultValue.format);
    value.useGroupOverview = fromXml(*child, "useGroupOverview", defaultValue.useGroupOverview);
    value.useAutoSize = fromXml(*child, "useAutoSize", defaultValue.useAutoSize);
    value.imageWidth = fromXml(*child, "imageWidth", defaultValue.imageWidth);
    value.imageHeight = fromXml(*child, "imageHeight", defaultValue.imageHeight);
    value.includeHeaderRaw = fromXml(*child, "includeHeaderRaw", defaultValue.includeHeaderRaw);
    value.ignoreGridResults = fromXml(*child, "ignoreGridResults", defaultValue.ignoreGridResults);
    value.columnSeparator = fromXml(*child, "columnSeparator", defaultValue.columnSeparator);
    return value;
}

ANALYSE_FILE_END
