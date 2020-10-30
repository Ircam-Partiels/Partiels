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
    anlWeakAssert(xml.hasAttribute("currentDocumentFile"));
    
    auto santizeList = [&](std::vector<juce::File> const& files)
    {
        std::vector<juce::File> copy = files;
        copy.erase(std::unique(copy.begin(), copy.end()), copy.end());
        copy.erase(std::remove_if(copy.begin(), copy.end(), [](auto const& file)
        {
            return !file.existsAsFile();
        }), copy.end());
        return copy;
    };
    
    defaultModel.windowState = xml.getStringAttribute("windowState", defaultModel.windowState);
    defaultModel.recentlyOpenedFilesList = Tools::StringParser::fromXml(xml, "recentlyOpenedFilesList", santizeList(defaultModel.recentlyOpenedFilesList));
    defaultModel.currentDocumentFile = Tools::StringParser::fromXml(xml, "currentDocumentFile", defaultModel.currentDocumentFile);
    
    return defaultModel;
}

std::set<Application::Model::Attribute> Application::Model::getAttributeTypes()
{
    return {Attribute::windowState, Attribute::recentlyOpenedFilesList, Attribute::currentDocumentFile};
}

std::unique_ptr<juce::XmlElement> Application::Model::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement>("Anl::Application::Model");
    if(xml == nullptr)
    {
        return nullptr;
    }
    
    xml->setAttribute("windowState", windowState);
    xml->setAttribute("recentlyOpenedFilesList", Tools::StringParser::toString(recentlyOpenedFilesList));
    xml->setAttribute("currentDocumentFile", Tools::StringParser::toString(currentDocumentFile));
    
    return xml;
}

void Application::Accessor::fromModel(Model const& model, juce::NotificationType const notification)
{
    using Attribute = Model::Attribute;
    std::set<Attribute> attributes;
    
    auto santizeList = [&](std::vector<juce::File> const& files)
    {
        std::vector<juce::File> copy = files;
        copy.erase(std::unique(copy.begin(), copy.end()), copy.end());
        copy.erase(std::remove_if(copy.begin(), copy.end(), [](auto const& file)
        {
            return !file.existsAsFile();
        }), copy.end());
        return copy;
    };
    
    compareAndSet(attributes, Attribute::windowState, mModel.windowState, model.windowState);
    compareAndSet(attributes, Attribute::recentlyOpenedFilesList, mModel.recentlyOpenedFilesList, santizeList(model.recentlyOpenedFilesList));
    compareAndSet(attributes, Attribute::currentDocumentFile, mModel.currentDocumentFile, model.currentDocumentFile);
    notifyListener(attributes, notification);
}

ANALYSE_FILE_END
