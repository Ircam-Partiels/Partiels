#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

Application::Model Application::Model::fromXml(juce::XmlElement const& xml, Model defaultModel)
{
    anlWeakAssert(xml.hasTagName("Anl::Application::Model"));
    if(!xml.hasTagName("Anl::Application::Model"))
    {
        return {};
    }
    
    anlWeakAssert(xml.hasAttribute("windowState"));
    anlWeakAssert(xml.hasAttribute("recentlyOpenedFilesList"));
    anlWeakAssert(xml.hasAttribute("currentOpenedFilesList"));
    anlWeakAssert(xml.hasAttribute("currentDocumentFile"));
    
    defaultModel.windowState = xml.getStringAttribute("windowState", defaultModel.windowState);
    defaultModel.recentlyOpenedFilesList = fromString(xml.getStringAttribute("recentlyOpenedFilesList"));
    sanitize(defaultModel.recentlyOpenedFilesList);
    defaultModel.currentOpenedFilesList = fromString(xml.getStringAttribute("currentOpenedFilesList"));
    sanitize(defaultModel.currentOpenedFilesList);
    defaultModel.currentDocumentFile = juce::File(xml.getStringAttribute("currentDocumentFile", defaultModel.currentDocumentFile.getFullPathName()));
    
    return defaultModel;
}

std::set<Application::Model::Attribute> Application::Model::getAttributeTypes()
{
    return {Attribute::windowState, Attribute::recentlyOpenedFilesList, Attribute::currentOpenedFilesList, Attribute::currentDocumentFile};
}

std::unique_ptr<juce::XmlElement> Application::Model::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement>("Anl::Application::Model");
    if(xml == nullptr)
    {
        return nullptr;
    }
    
    xml->setAttribute("windowState", windowState);
    xml->setAttribute("recentlyOpenedFilesList", toString(recentlyOpenedFilesList));
    xml->setAttribute("currentOpenedFilesList", toString(currentOpenedFilesList));
    xml->setAttribute("currentDocumentFile", currentDocumentFile.getFullPathName());
    
    return xml;
}

void Application::Accessor::fromModel(Model const& model, juce::NotificationType const notification)
{
    using Attribute = Model::Attribute;
    std::set<Attribute> attributes;
    MODEL_ACCESSOR_COMPARE_AND_SET(windowState, attributes)
    MODEL_ACCESSOR_COMPARE_AND_SET(recentlyOpenedFilesList, attributes)
    MODEL_ACCESSOR_COMPARE_AND_SET(currentOpenedFilesList, attributes)
    MODEL_ACCESSOR_COMPARE_AND_SET(currentDocumentFile, attributes)
    notifyListener(attributes, {}, notification);
}

std::vector<juce::File> Application::Model::fromString(juce::String const& filesAsString)
{
    juce::StringArray filePaths;
    filePaths.addLines(filesAsString);
    std::vector<juce::File> files;
    files.reserve(static_cast<size_t>(filePaths.size()));
    for(auto const& filePath : filePaths)
    {
        files.push_back({filePath});
    }
    return files;
}

juce::String Application::Model::toString(std::vector<juce::File> const& files)
{
    juce::StringArray filePaths;
    filePaths.ensureStorageAllocated(static_cast<int>(files.size()));
    for(auto const& file : files)
    {
        filePaths.add(file.getFullPathName());
    }
    return filePaths.joinIntoString("\n");
}

std::vector<juce::File> Application::Model::sanitize(std::vector<juce::File> const& files)
{
    std::vector<juce::File> copy = files;
    copy.erase(std::unique(copy.begin(), copy.end()), copy.end());
    copy.erase(std::remove_if(copy.begin(), copy.end(), [](auto const& file)
    {
        return !file.existsAsFile();
    }), copy.end());
    return copy;
}

ANALYSE_FILE_END
