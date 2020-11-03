#include "AnlStringParser.h"

ANALYSE_FILE_BEGIN

template <> unsigned long Tools::StringParser::fromString(juce::String const& string)
{
    return std::stoul(string.toStdString());
}

template <> juce::String Tools::StringParser::toString(unsigned long const& value)
{
    return juce::String(std::to_string(value));
}

template <> juce::File Tools::StringParser::fromString<juce::File>(juce::String const& string)
{
    return juce::File(string);
}

template <> juce::String Tools::StringParser::toString<juce::File>(juce::File const& value)
{
    return value.getFullPathName();
}

template <> juce::Range<double> Tools::StringParser::fromString<juce::Range<double>>(juce::String const& string)
{
    juce::StringArray stringArray;
    stringArray.addLines(string);
    anlStrongAssert(stringArray.size() == 2);
    auto const start = stringArray.size() >= 1 ? stringArray.getReference(0).getDoubleValue() : 0.0;
    auto const end = stringArray.size() >= 2 ? stringArray.getReference(1).getDoubleValue() : 0.0;
    return {start, end};
}

template <> juce::String Tools::StringParser::toString<juce::Range<double>>(juce::Range<double> const& value)
{
    juce::StringArray stringArray;
    stringArray.ensureStorageAllocated(2);
    stringArray.add(juce::String(value.getStart()));
    stringArray.add(juce::String(value.getEnd()));
    return stringArray.joinIntoString("\n");
}

template <> std::vector<juce::File> Tools::StringParser::fromString<std::vector<juce::File>>(juce::String const& string)
{
    juce::StringArray filePaths;
    filePaths.addLines(string);
    std::vector<juce::File> files;
    files.reserve(static_cast<size_t>(filePaths.size()));
    for(auto const& filePath : filePaths)
    {
        files.push_back({filePath});
    }
    return files;
}

template <> juce::String Tools::StringParser::toString<std::vector<juce::File>>(std::vector<juce::File> const& value)
{
    juce::StringArray filePaths;
    filePaths.ensureStorageAllocated(static_cast<int>(value.size()));
    for(auto const& file : value)
    {
        filePaths.add(file.getFullPathName());
    }
    return filePaths.joinIntoString("\n");
}

std::vector<std::reference_wrapper<juce::XmlElement>> Tools::XmlUtils::getChilds(juce::XmlElement const& xml, juce::StringRef const& tag, juce::StringRef const& newTag)
{
    std::vector<std::reference_wrapper<juce::XmlElement>> childs;
    childs.reserve(static_cast<size_t>(xml.getNumChildElements()));
    for(auto* child = xml.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        if(child != nullptr && (tag.isEmpty() || tag == child->getTagName()))
        {
            if(newTag.isNotEmpty())
            {
                child->setTagName(newTag);                
            }
            childs.push_back(*child);
        }
    }
    return childs;
}

ANALYSE_FILE_END
