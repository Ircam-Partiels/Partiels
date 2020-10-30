#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

Document::Model::Model(Model const& other)
: file(other.file)
, isLooping(other.isLooping)
{
    analyzers.resize(other.analyzers.size());
    std::transform(other.analyzers.cbegin(), other.analyzers.cend(), analyzers.begin(), [](auto const& analyzer)
    {
        anlStrongAssert(analyzer != nullptr);
        return analyzer != nullptr ? std::make_unique<Analyzer::Model>(*analyzer.get()) : std::make_unique<Analyzer::Model>();
    });
}

Document::Model::Model(Model&& other)
: file(std::move(other.file))
, analyzers(std::move(other.analyzers))
, isLooping(std::move(other.isLooping))
{
    
}

bool Document::Model::operator==(Model const& other) const
{
    return file == other.file
    && isLooping == other.isLooping
    && std::equal(analyzers.cbegin(), analyzers.cend(), other.analyzers.cbegin(), [](auto const& lhs, auto const& rhs)
    {
        return lhs != nullptr && rhs != nullptr && *(lhs.get()) == *(rhs.get());
    });
}

bool Document::Model::operator!=(Model const& other) const
{
    return (*this == other) == false;
}

Document::Model Document::Model::fromXml(juce::XmlElement const& xml, Model defaultModel)
{
    anlWeakAssert(xml.hasTagName("Anl::Document::Model"));
    if(!xml.hasTagName("Anl::Document::Model"))
    {
        return {};
    }
    
    anlWeakAssert(xml.hasAttribute("file"));
    anlWeakAssert(xml.hasAttribute("isLooping"));
    
    defaultModel.file = Tools::StringParser::fromXml(xml, "file", defaultModel.file);
    defaultModel.isLooping = xml.getBoolAttribute("isLooping", defaultModel.isLooping);
    auto const childs = Tools::XmlUtils::getChilds(xml, "Analyzer");
    auto& analyzers = defaultModel.analyzers;
    analyzers.resize(childs.size());
    for(size_t i = 0; i < analyzers.size(); ++i)
    {
        if(analyzers[i] == nullptr)
        {
            analyzers[i] = std::make_unique<Analyzer::Model>(Analyzer::Model::fromXml(childs[i]));
        }
        else
        {
            *(analyzers[i].get()) = Analyzer::Model::fromXml(childs[i]);
        }
    }
    return defaultModel;
}

std::unique_ptr<juce::XmlElement> Document::Model::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement>("Anl::Document::Model");
    if(xml == nullptr)
    {
        return nullptr;
    }
    
    xml->setAttribute("file", Tools::StringParser::toString(file));
    xml->setAttribute("isLooping", isLooping);
    Tools::XmlUtils::addChilds(*xml, analyzers, "Analyzer");
    return xml;
}

std::set<Document::Model::Attribute> Document::Model::getAttributeTypes()
{
    return {Attribute::file, Attribute::analyzers, Attribute::isLooping};
}

void Document::Accessor::fromModel(Model const& model, juce::NotificationType const notification)
{
    using Attribute = Model::Attribute;
    std::set<Attribute> attributes;
    compareAndSet(attributes, Attribute::isLooping, mModel.isLooping, model.isLooping);
    compareAndSet(attributes, Attribute::file, mModel.file, model.file);
    auto data = compareAndSet(attributes, Attribute::analyzers, mAnalyzerAccessors, mModel.analyzers, model.analyzers, notification);
    notifyListener(attributes, notification);
    JUCE_COMPILER_WARNING("zaza")
//    if(notification == juce::NotificationType::sendNotificationAsync)
//    {
//        juce::MessageManager::callAsync([ptr = std::make_shared<decltype(data)::element_type>(data)>(data.release())]
//        {
//            juce::ignoreUnused(ptr);
//        });
//    }
}

Analyzer::Accessor& Document::Accessor::getAnalyzerAccessor(size_t index)
{
    return *mAnalyzerAccessors[index].get();
}

ANALYSE_FILE_END
