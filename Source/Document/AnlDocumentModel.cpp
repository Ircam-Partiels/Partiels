#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

Document::Model::Model(Model const& other)
: file(other.file)
, isLooping(other.isLooping)
//, zoomStateTime(other.zoomStateTime.mData)
{
    analyzers.resize(other.analyzers.size());
    std::transform(other.analyzers.cbegin(), other.analyzers.cend(), analyzers.begin(), [](auto const& analyzer)
    {
        anlStrongAssert(analyzer != nullptr);
        auto newAnalyzer = std::make_unique<Analyzer::Accessor>();
        if(newAnalyzer)
        {
            newAnalyzer->fromModel(analyzer->getModel(), NotificationType::synchronous);
        }
        return newAnalyzer;
    });
    
    
}

Document::Model::Model(Model&& other)
: file(std::move(other.file))
, isLooping(std::move(other.isLooping))
//, zoomStateTime(std::move(other.zoomStateTime.mData))
, analyzers(std::move(other.analyzers))
{
    
}

bool Document::Model::operator==(Model const& other) const
{
    return file == other.file &&
    isLooping == other.isLooping &&
    std::equal(analyzers.cbegin(), analyzers.cend(), other.analyzers.cbegin(), [](auto const& lhs, auto const& rhs)
    {
        return lhs != nullptr && rhs != nullptr && lhs->isEquivalentTo(rhs->getModel());
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
    anlWeakAssert(xml.hasAttribute("gain"));
    
    defaultModel.file = Tools::StringParser::fromXml(xml, "file", defaultModel.file);
    defaultModel.isLooping = xml.getBoolAttribute("isLooping", defaultModel.isLooping);
    defaultModel.gain = xml.getDoubleAttribute("gain", defaultModel.gain);
    auto const childs = Tools::XmlUtils::getChilds(xml, "analyzers", "analyzers");
    auto& analyzers = defaultModel.analyzers;
    analyzers.resize(childs.size());
    for(size_t i = 0; i < analyzers.size(); ++i)
    {
        if(analyzers[i] == nullptr)
        {
            analyzers[i] = std::make_unique<Analyzer::Accessor>();
        }
        analyzers[i]->fromXml(childs[i], "analyzers", NotificationType::synchronous);
    }
    auto* child = xml.getChildByName("Zoom::State::Time");
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        defaultModel.zoomStateTime.fromXml(*child, "Zoom::State::Time", NotificationType::synchronous);
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
    xml->setAttribute("gain", gain);
    for(auto const& child : analyzers)
    {
        xml->addChildElement(child->toXml("analyzers").release());
    }
    xml->addChildElement(zoomStateTime.toXml("Zoom::State::Time").release());
    return xml;
}

void Document::Accessor::fromModel(Model const& model, NotificationType const notification)
{
    using Attribute = Model::Attribute;
    std::set<Attribute> attributes;
    compareAndSet(attributes, Attribute::file, mModel.file, model.file);
    compareAndSet(attributes, Attribute::isLooping, mModel.isLooping, model.isLooping);
    compareAndSet(attributes, Attribute::gain, mModel.gain, model.gain);
    
    compareAndSet(attributes, Attribute::isPlaybackStarted, mModel.isPlaybackStarted, model.isPlaybackStarted);
    compareAndSet(attributes, Attribute::playheadPosition, mModel.playheadPosition, model.playheadPosition);
    
    //auto data = compareAndSet(attributes, Attribute::analyzers, mAnalyzerAccessors, mModel.analyzers, model.analyzers, notification);
    notifyListener(attributes, notification);
    mModel.zoomStateTime.fromModel(model.zoomStateTime.getModel(), notification);
    JUCE_COMPILER_WARNING("fix asynchronous support of data deletion")
}

Analyzer::Accessor& Document::Accessor::getAnalyzerAccessor(size_t index)
{
    return *mModel.analyzers[index].get();
}

Zoom::State::Accessor& Document::Accessor::getZoomStateTimeAccessor()
{
    return mModel.zoomStateTime;
}

ANALYSE_FILE_END
