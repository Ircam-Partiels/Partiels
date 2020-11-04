#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

Document::Model::Model(Model const& other)
: file(other.file)
, isLooping(other.isLooping)
, zoomStateTime(other.zoomStateTime)
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
, isLooping(std::move(other.isLooping))
, zoomStateTime(std::move(other.zoomStateTime))
, analyzers(std::move(other.analyzers))
{
    
}

bool Document::Model::operator==(Model const& other) const
{
    return file == other.file &&
    isLooping == other.isLooping &&
    std::equal(analyzers.cbegin(), analyzers.cend(), other.analyzers.cbegin(), [](auto const& lhs, auto const& rhs)
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
    anlWeakAssert(xml.hasAttribute("gain"));
    
    defaultModel.file = Tools::StringParser::fromXml(xml, "file", defaultModel.file);
    defaultModel.isLooping = xml.getBoolAttribute("isLooping", defaultModel.isLooping);
    defaultModel.gain = xml.getDoubleAttribute("gain", defaultModel.gain);
    auto const childs = Tools::XmlUtils::getChilds(xml, "Analyzer", "Anl::Analyzer::Model");
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
    auto* child = xml.getChildByName("Zoom::State::Time");
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        child->setTagName("Zoom::State::Model");
        defaultModel.zoomStateTime = Zoom::State::Model::fromXml(*child, defaultModel.zoomStateTime);
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
    Tools::XmlUtils::addChilds(*xml, analyzers, "Analyzer");
    auto child = zoomStateTime.toXml();
    anlStrongAssert(child != nullptr);
    if(child != nullptr)
    {
        child->setTagName("Zoom::State::Time");
        xml->addChildElement(child.release());
    }
    return xml;
}

void Document::Accessor::fromModel(Model const& model, juce::NotificationType const notification)
{
    using Attribute = Model::Attribute;
    std::set<Attribute> attributes;
    compareAndSet(attributes, Attribute::file, mModel.file, model.file);
    compareAndSet(attributes, Attribute::isLooping, mModel.isLooping, model.isLooping);
    compareAndSet(attributes, Attribute::gain, mModel.gain, model.gain);
    
    compareAndSet(attributes, Attribute::isPlaybackStarted, mModel.isPlaybackStarted, model.isPlaybackStarted);
    compareAndSet(attributes, Attribute::playheadPosition, mModel.playheadPosition, model.playheadPosition);
    
    auto data = compareAndSet(attributes, Attribute::analyzers, mAnalyzerAccessors, mModel.analyzers, model.analyzers, notification);
    notifyListener(attributes, notification);
    mZoomStateTimeAccessor.fromModel(model.zoomStateTime, notification);
    JUCE_COMPILER_WARNING("fix asynchronous support of data deletion")
}

Analyzer::Accessor& Document::Accessor::getAnalyzerAccessor(size_t index)
{
    return *mAnalyzerAccessors[index].get();
}

Zoom::State::Accessor& Document::Accessor::getZoomStateTimeAccessor()
{
    return mZoomStateTimeAccessor;
}

ANALYSE_FILE_END
