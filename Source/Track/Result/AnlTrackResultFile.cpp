#include "AnlTrackResultFile.h"

ANALYSE_FILE_BEGIN

Track::Result::FileInfo::FileInfo(juce::File const& f)
: file(f)
{
}

bool Track::Result::FileInfo::operator==(FileInfo const& rhs) const noexcept
{
    return file == rhs.file && commit == rhs.commit;
}

bool Track::Result::FileInfo::operator!=(FileInfo const& rhs) const noexcept
{
    return !(*this == rhs);
}

bool Track::Result::FileInfo::isEmpty() const noexcept
{
    return file == juce::File{} && commit.isEmpty();
}

Track::Result::FileDescription::FileDescription(juce::File const& f)
: file(f)
, extension(file.getFileExtension())
{
}

char Track::Result::FileDescription::toChar(ColumnSeparator const& separator)
{
    switch(separator)
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

Track::Result::FileDescription::ColumnSeparator Track::Result::FileDescription::toColumnSeparator(char const& separator)
{
    switch(separator)
    {
        case ',':
        {
            return ColumnSeparator::comma;
        }
        case ' ':
        {
            return ColumnSeparator::space;
        }
        case '\t':
        {
            return ColumnSeparator::tab;
        }
        case '|':
        {
            return ColumnSeparator::pipe;
        }
        case '/':
        {
            return ColumnSeparator::slash;
        }
        case ':':
        {
            return ColumnSeparator::colon;
        }
        default:
        {
            return ColumnSeparator::comma;
        }
    }
}

bool Track::Result::FileDescription::operator==(FileDescription const& rhs) const noexcept
{
    return file == rhs.file &&
           format == rhs.format &&
           extension == rhs.extension &&
           includeHeaderRow == rhs.includeHeaderRow &&
           columnSeparator == rhs.columnSeparator &&
           disableLabelEscaping == rhs.disableLabelEscaping &&
           reaperType == rhs.reaperType &&
           includeDescription == rhs.includeDescription &&
           sdifFrameSignature == rhs.sdifFrameSignature &&
           sdifMatrixSignature == rhs.sdifMatrixSignature;
}

bool Track::Result::FileDescription::operator!=(FileDescription const& rhs) const noexcept
{
    return !(*this == rhs);
}

bool Track::Result::FileDescription::isEmpty() const noexcept
{
    return file == juce::File{};
}

template <>
void XmlParser::toXml<Track::Result::FileInfo>(juce::XmlElement& xml, juce::Identifier const& attributeName, Track::Result::FileInfo const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "path", value.file);
        toXml(*child, "commit", value.commit);
        xml.addChildElement(child.release());
    }
}

template <>
auto XmlParser::fromXml<Track::Result::FileInfo>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Track::Result::FileInfo const& defaultValue)
    -> Track::Result::FileInfo
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    auto value = defaultValue;
    value.file = fromXml(*child, "path", defaultValue.file);
    value.commit = fromXml(*child, "commit", defaultValue.commit);
    return value;
}

template <>
void XmlParser::toXml<Track::Result::FileDescription>(juce::XmlElement& xml, juce::Identifier const& attributeName, Track::Result::FileDescription const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "path", value.file);
        toXml(*child, "format", value.format);
        toXml(*child, "extension", value.extension);
        toXml(*child, "includeHeaderRow", value.includeHeaderRow);
        toXml(*child, "reaperType", value.reaperType);
        toXml(*child, "columnSeparator", value.columnSeparator);
        toXml(*child, "disableLabelEscaping", value.disableLabelEscaping);
        toXml(*child, "includeDescription", value.includeDescription);
        toXml(*child, "sdifFrameSignature", value.sdifFrameSignature);
        toXml(*child, "sdifMatrixSignature", value.sdifMatrixSignature);
        xml.addChildElement(child.release());
    }
}

template <>
auto XmlParser::fromXml<Track::Result::FileDescription>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Track::Result::FileDescription const& defaultValue)
    -> Track::Result::FileDescription
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Track::Result::FileDescription value;
    value.file = fromXml(*child, "path", defaultValue.file);
    value.format = fromXml(*child, "format", defaultValue.format);
    value.extension = fromXml(*child, "extension", defaultValue.extension);
    value.includeHeaderRow = fromXml(*child, "includeHeaderRow", defaultValue.includeHeaderRow);
    value.reaperType = fromXml(*child, "reaperType", defaultValue.reaperType);
    value.columnSeparator = fromXml(*child, "columnSeparator", defaultValue.columnSeparator);
    value.disableLabelEscaping = fromXml(*child, "disableLabelEscaping", defaultValue.disableLabelEscaping);
    value.includeDescription = fromXml(*child, "includeDescription", defaultValue.includeDescription);
    value.sdifFrameSignature = fromXml(*child, "sdifFrameSignature", defaultValue.sdifFrameSignature);
    value.sdifMatrixSignature = fromXml(*child, "sdifMatrixSignature", defaultValue.sdifMatrixSignature);
    return value;
}

void Track::Result::to_json(nlohmann::json& j, FileInfo const& file)
{
    j["path"] = file.file;
    j["commit"] = file.commit;
}

void Track::Result::from_json(nlohmann::json const& j, FileInfo& file)
{
    file.file = j.value("path", file.file);
    file.commit = j.value("commit", file.commit);
}

void Track::Result::to_json(nlohmann::json& j, FileDescription const& file)
{
    j["path"] = file.file;
    j["format"] = std::string(magic_enum::enum_name(file.format));
    j["extension"] = file.extension;
    j["includeHeaderRow"] = file.includeHeaderRow;
    j["columnSeparator"] = std::string(magic_enum::enum_name(file.columnSeparator));
    j["disableLabelEscaping"] = file.disableLabelEscaping;
    j["reaperType"] = std::string(magic_enum::enum_name(file.reaperType));
    j["includeDescription"] = file.includeDescription;
    j["sdifFrameSignature"] = file.sdifFrameSignature;
    j["sdifMatrixSignature"] = file.sdifMatrixSignature;
}

void Track::Result::from_json(nlohmann::json const& j, FileDescription& file)
{
    file.file = j.value("path", file.file);
    file.format = magic_enum::enum_cast<FileDescription::Format>(j.value("format", std::string(magic_enum::enum_name(file.format)))).value_or(file.format);
    file.extension = j.value("extension", file.extension);
    file.includeHeaderRow = j.value("includeHeaderRow", file.includeHeaderRow);
    file.columnSeparator = magic_enum::enum_cast<FileDescription::ColumnSeparator>(j.value("columnSeparator", std::string(magic_enum::enum_name(file.columnSeparator)))).value_or(file.columnSeparator);
    file.disableLabelEscaping = j.value("disableLabelEscaping", file.disableLabelEscaping);
    file.reaperType = magic_enum::enum_cast<FileDescription::ReaperType>(j.value("reaperType", std::string(magic_enum::enum_name(file.reaperType)))).value_or(file.reaperType);
    file.includeDescription = j.value("includeDescription", file.includeDescription);
    file.sdifFrameSignature = j.value("sdifFrameSignature", file.sdifFrameSignature);
    file.sdifMatrixSignature = j.value("sdifMatrixSignature", file.sdifMatrixSignature);
}

ANALYSE_FILE_END
