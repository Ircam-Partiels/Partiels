#include "AnlDocumentExporter.h"
#include "../Group/AnlGroupExporter.h"
#include "../Track/AnlTrackExporter.h"
#include "../Track/AnlTrackTools.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

juce::Result Document::Exporter::toFile(Accessor& accessor, juce::File const file, juce::String const& identifier, Options const& options, std::function<std::pair<int, int>(juce::String const& identifier)> getSizeFor)
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
                return juce::Result::fail("Invalid threaded threadsafe access");
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
            lock.exit();

            return Track::Exporter::toImage(trackAcsr, timeZoomAcsr, trackFile, std::get<0>(size), std::get<1>(size));
        };

        auto exportGroup = [&](juce::String const& groupIdentifier, juce::File const& groupFile)
        {
            juce::MessageManager::Lock lock;
            if(!lock.tryEnter())
            {
                return juce::Result::fail("Invalid threaded threadsafe access");
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
            lock.exit();

            return Group::Exporter::toImage(groupAcsr, timeZoomAcsr, groupFile, std::get<0>(size), std::get<1>(size));
        };

        auto exportGroupTracks = [&](juce::String const& groupIdentifier, juce::File const& groupFolder)
        {
            juce::MessageManager::Lock lock;
            if(!lock.tryEnter())
            {
                return juce::Result::fail("Invalid threaded threadsafe access");
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
                    return juce::Result::fail("Invalid threaded threadsafe access");
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
                return juce::Result::fail("Invalid threaded threadsafe access");
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
                    return juce::Result::fail("Invalid threaded threadsafe access");
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
        lock.exit();

        switch(options.format)
        {
            case Options::Format::jpeg:
                return juce::Result::fail("Unsupported format");
            case Options::Format::png:
                return juce::Result::fail("Unsupported format");
            case Options::Format::csv:
                return Track::Exporter::toCsv(trackAcsr, trackFile, options.includeHeaderRaw, options.getSeparatorChar());
            case Options::Format::json:
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
                return juce::Result::fail("Invalid threaded threadsafe access");
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
        return juce::Result::fail("Invalid threaded threadsafe access");
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
