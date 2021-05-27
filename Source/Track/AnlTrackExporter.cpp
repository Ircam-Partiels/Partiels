#include "AnlTrackExporter.h"
#include "../Transport/AnlTransportModel.h"
#include "AnlTrackPlot.h"
#include "AnlTrackTools.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#include <json/json.hpp>
#pragma GCC diagnostic pop
#include <fstream>

ANALYSE_FILE_BEGIN

juce::Result Track::Exporter::fromPreset(Accessor& accessor, juce::File const& file)
{
    auto xml = juce::XmlDocument::parse(file);
    if(xml == nullptr)
    {
        return juce::Result::fail(juce::translate("The file FLNAME cannot be parsed to preset format.").replace("FLNAME", file.getFullPathName()));
    }
    if(!xml->hasTagName("preset"))
    {
        return juce::Result::fail(juce::translate("The preset file FLNAME is not valid.").replace("FLNAME", file.getFullPathName()));
    }

    auto const key = XmlParser::fromXml(*xml.get(), "key", Plugin::Key());
    if(key != accessor.getAttr<AttrType::key>())
    {
        return juce::Result::fail(juce::translate("The plugin key of the preset file FLNAME doesn't correspond to track.").replace("FLNAME", file.getFullPathName()));
    }
    accessor.setAttr<AttrType::state>(XmlParser::fromXml(*xml.get(), "state", accessor.getAttr<AttrType::state>()), NotificationType::synchronous);
    return juce::Result::ok();
}

juce::Result Track::Exporter::toPreset(Accessor const& accessor, juce::File const& file)
{
    auto const name = accessor.getAttr<AttrType::name>();

    auto const title = juce::translate("Export as preset failed!");
    auto xml = std::make_unique<juce::XmlElement>("preset");
    anlWeakAssert(xml != nullptr);
    if(xml == nullptr)
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as a preset because the track cannot be parsed to preset format.").replace("ANLNAME", name));
    }

    XmlParser::toXml(*xml.get(), "key", accessor.getAttr<AttrType::key>());
    XmlParser::toXml(*xml.get(), "state", accessor.getAttr<AttrType::state>());

    if(!xml->writeTo(file))
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as a preset because the file FLNAME cannot be written.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }
    return juce::Result::ok();
}

juce::Result Track::Exporter::toImage(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, juce::File const& file, int width, int height)
{
    juce::MessageManager::Lock lock;
    if(!lock.tryEnter())
    {
        return juce::Result::fail("Invalid threaded access to model");
    }
    auto const name = accessor.getAttr<AttrType::name>();
    lock.exit();

    if(width <= 0 || height <= 0)
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as image because image size is not valid.").replace("ANLNAME", accessor.getAttr<AttrType::name>()));
    }
    juce::TemporaryFile temp(file);

    auto* imageFormat = juce::ImageFileFormat::findImageFormatForFileExtension(temp.getFile());
    if(imageFormat == nullptr)
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as image because the format of the file FLNAME is not supported.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }

    auto getImage = [&]()
    {
        if(!lock.tryEnter())
        {
            return juce::Image{};
        }
        juce::Image image(juce::Image::PixelFormat::ARGB, width, height, true);
        juce::Graphics g(image);
        g.fillAll(accessor.getAttr<AttrType::colours>().background);
        Plot::paint(accessor, g, {0, 0, width, height}, timeZoomAccessor);
        return image;
    };

    auto const image = getImage();
    if(image.isValid())
    {
        juce::FileOutputStream stream(temp.getFile());
        if(!stream.openedOk())
        {
            return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as image because the output stream of the file FLNAME cannot be opened.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
        }

        if(!imageFormat->writeImageToStream(image, stream))
        {
            return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as image because the output stream of the file FLNAME cannot be written.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
        }
    }
    else
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as image because the image connott be created.").replace("ANLNAME", name));
    }

    if(!temp.overwriteTargetFileWithTemporary())
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be written to the file FLNAME. Ensure you have the right access to this file.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }
    return juce::Result::ok();
}

juce::Result Track::Exporter::toCsv(Accessor const& accessor, juce::File const& file, bool includeHeader, char separator)
{
    juce::MessageManager::Lock lock;
    if(!lock.tryEnter())
    {
        return juce::Result::fail("Invalid threaded access to model");
    }
    auto const results = accessor.getAttr<AttrType::results>();
    auto const name = accessor.getAttr<AttrType::name>();
    lock.exit();

    if(results.isEmpty())
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be exported as CSV because the results are not valid.").replace("ANLNAME", name));
    }

    juce::TemporaryFile temp(file);

    {
        juce::FileOutputStream stream(temp.getFile());
        if(!stream.openedOk())
        {
            return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be exported as CSV because the output stream of the file FLNAME cannot be opened.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
        }

        auto state = false;
        auto addLine = [&]()
        {
            state = false;
            stream << '\n';
        };

        auto addColumn = [&](juce::StringRef const& text)
        {
            if(state)
            {
                stream << separator;
            }
            stream << text;
            state = true;
            return true;
        };

        auto const markers = results.getMarkers();
        auto const points = results.getPoints();
        auto const columns = results.getColumns();
        if(markers != nullptr)
        {
            if(includeHeader)
            {
                addColumn("ROW");
                for(size_t i = 0; i < markers->size(); ++i)
                {
                    addColumn("TIME" + juce::String(i));
                    addColumn("DURATION" + juce::String(i));
                    addColumn("LABEL " + juce::String(i));
                }
                addLine();
            }

            auto const maxChannel = std::max_element(markers->cbegin(), markers->cend(), [](auto const& lhs, auto const& rhs)
                                                     {
                                                         return lhs.size() < rhs.size();
                                                     });
            auto const numValues = maxChannel != markers->cend() ? maxChannel->size() : 0_z;
            for(size_t i = 0; i < numValues; ++i)
            {
                addColumn(juce::String(i));
                for(auto const& channelResults : *markers)
                {
                    if(channelResults.size() > i)
                    {
                        addColumn(juce::String(std::get<0>(channelResults[i])));
                        addColumn(juce::String(std::get<1>(channelResults[i])));
                        addColumn(std::get<2>(channelResults[i]));
                    }
                    else
                    {
                        addColumn("\"\"");
                        addColumn("\"\"");
                        addColumn("\"\"");
                    }
                }
                addLine();
            }
        }
        else if(points != nullptr)
        {
            if(includeHeader)
            {
                addColumn("ROW");
                for(size_t i = 0; i < points->size(); ++i)
                {
                    addColumn("TIME" + juce::String(i));
                    addColumn("DURATION" + juce::String(i));
                    addColumn("VALUE " + juce::String(i));
                }
                addLine();
            }

            auto const maxChannel = std::max_element(points->cbegin(), points->cend(), [](auto const& lhs, auto const& rhs)
                                                     {
                                                         return lhs.size() < rhs.size();
                                                     });
            auto const numValues = maxChannel != points->cend() ? maxChannel->size() : 0_z;
            for(size_t i = 0; i < numValues; ++i)
            {
                addColumn(juce::String(i));
                for(auto const& channelResults : *points)
                {
                    if(channelResults.size() > i)
                    {
                        addColumn(juce::String(std::get<0>(channelResults[i])));
                        addColumn(juce::String(std::get<1>(channelResults[i])));
                        addColumn(!std::get<2>(channelResults[i]).has_value() ? "\"\"" : juce::String(*std::get<2>(channelResults[i])));
                    }
                    else
                    {
                        addColumn("\"\"");
                        addColumn("\"\"");
                        addColumn("\"\"");
                    }
                }
                addLine();
            }
        }
        else if(columns != nullptr)
        {
            if(includeHeader)
            {
                addColumn("ROW");
                for(size_t i = 0; i < columns->size(); ++i)
                {
                    addColumn("TIME" + juce::String(i));
                    addColumn("DURATION" + juce::String(i));
                    auto const& channelResults = columns->at(i);
                    for(size_t j = 0; j < std::get<2>(channelResults.front()).size(); ++j)
                    {
                        addColumn("BIN " + juce::String(i) + juce::String(j));
                    }
                }
                addLine();
            }

            auto const maxChannel = std::max_element(columns->cbegin(), columns->cend(), [](auto const& lhs, auto const& rhs)
                                                     {
                                                         return lhs.size() < rhs.size();
                                                     });
            auto const numValues = maxChannel != columns->cend() ? maxChannel->size() : 0_z;
            for(size_t i = 0; i < numValues; ++i)
            {
                addColumn(juce::String(i));
                for(auto const& channelResults : *columns)
                {
                    if(channelResults.size() > i)
                    {
                        addColumn(juce::String(std::get<0>(channelResults[i])));
                        addColumn(juce::String(std::get<1>(channelResults[i])));
                        for(auto const& value : std::get<2>(channelResults[i]))
                        {
                            addColumn(juce::String(value));
                        }
                    }
                    else
                    {
                        addColumn("\"\"");
                        addColumn("\"\"");
                    }
                }
                addLine();
            }
        }
    }

    if(!temp.overwriteTargetFileWithTemporary())
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be written to the file FLNAME. Ensure you have the right access to this file.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }
    return juce::Result::ok();
}

juce::Result Track::Exporter::toJson(Accessor const& accessor, juce::File const& file)
{
    juce::MessageManager::Lock lock;
    if(!lock.tryEnter())
    {
        return juce::Result::fail("Invalid threaded access to model");
    }
    auto const results = accessor.getAttr<AttrType::results>();
    auto const name = accessor.getAttr<AttrType::name>();
    lock.exit();

    if(results.isEmpty())
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be exported as JSON because the results are not valid.").replace("ANLNAME", name).replace("ANLNAME", accessor.getAttr<AttrType::name>()));
    }

    nlohmann::json json;

    auto const markers = results.getMarkers();
    auto const points = results.getPoints();
    auto const columns = results.getColumns();
    if(markers != nullptr)
    {
        for(auto const& channelResults : *markers)
        {
            nlohmann::json cjson;
            for(auto const& result : channelResults)
            {
                nlohmann::json vjson;
                vjson["time"] = std::get<0>(result);
                if(std::get<1>(result) > 0.0)
                {
                    vjson["duration"] = std::get<1>(result);
                }
                if(!std::get<2>(result).empty())
                {
                    vjson["label"] = std::get<2>(result);
                }
                cjson.emplace_back(std::move(vjson));
            }
            json.emplace_back(std::move(cjson));
        }
    }
    else if(points != nullptr)
    {
        for(auto const& channelResults : *points)
        {
            nlohmann::json cjson;
            for(auto const& result : channelResults)
            {
                nlohmann::json vjson;
                vjson["time"] = std::get<0>(result);
                if(std::get<1>(result) > 0.0)
                {
                    vjson["duration"] = std::get<1>(result);
                }
                if(std::get<2>(result).has_value())
                {
                    vjson["value"] = *std::get<2>(result);
                }
                cjson.emplace_back(std::move(vjson));
            }
            json.emplace_back(std::move(cjson));
        }
    }
    else if(columns != nullptr)
    {
        for(auto const& channelResults : *columns)
        {
            nlohmann::json cjson;
            for(auto const& result : channelResults)
            {
                nlohmann::json vjson;
                vjson["time"] = std::get<0>(result);
                if(std::get<1>(result) > 0.0)
                {
                    vjson["duration"] = std::get<1>(result);
                }
                vjson["values"] = std::get<2>(result);
                cjson.emplace_back(std::move(vjson));
            }
            json.emplace_back(std::move(cjson));
        }
    }

    juce::TemporaryFile temp(file);
    std::ofstream stream(temp.getFile().getFullPathName().toStdString());
    if(!stream.is_open() || !stream.good())
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as JSON because the output stream of the file FLNAME cannot be opened.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }
    stream << json << std::endl;
    stream.close();
    if(!temp.overwriteTargetFileWithTemporary())
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be written to the file FLNAME. Ensure you have the right access to this file.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }
    return juce::Result::ok();
}

ANALYSE_FILE_END
