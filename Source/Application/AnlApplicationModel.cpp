#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

Application::Model Application::Model::fromXml(juce::XmlElement const& xml)
{
    ilfWeakAssert(xml.hasTagName("Anl::Application::Model"));
    if(!xml.hasTagName("Anl::Application::Model"))
    {
        return {};
    }
    
    ilfWeakAssert(xml.hasAttribute("windowState"));
    ilfWeakAssert(xml.hasAttribute("recentlyOpenedFilesList"));
    ilfWeakAssert(xml.hasAttribute("currentOpenedFilesList"));
    ilfWeakAssert(xml.hasAttribute("currentDocumentFile"));
    
    Model model;
    model.windowState = xml.getStringAttribute("windowState", model.windowState);
    model.recentlyOpenedFilesList = fromString(xml.getStringAttribute("recentlyOpenedFilesList"));
    sanitize(model.recentlyOpenedFilesList);
    model.currentOpenedFilesList = fromString(xml.getStringAttribute("currentOpenedFilesList"));
    sanitize(model.currentOpenedFilesList);
    model.currentDocumentFile = juce::File(xml.getStringAttribute("currentDocumentFile", model.currentDocumentFile.getFullPathName()));
    
    return model;
}

std::unique_ptr<juce::XmlElement> Application::Model::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement>("As::Application::Model");
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

void Application::Model::sanitize(std::vector<juce::File>& files)
{
    files.erase(std::unique(files.begin(), files.end()), files.end());
    files.erase(std::remove_if(files.begin(), files.end(), [](auto const& file)
    {
        return !file.existsAsFile();
    }), files.end());
}

ANALYSE_FILE_END
