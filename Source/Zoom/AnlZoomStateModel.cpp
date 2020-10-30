#include "AnlZoomStateModel.h"
#include "../Tools/AnlStringParser.h"

ANALYSE_FILE_BEGIN

Zoom::State::Model::Model(juce::Range<double> gr, juce::Range<double> vr, double length)
: globalRange(gr)
, visibleRange(vr)
, minimumLength(length)
{
}

Zoom::State::Model Zoom::State::Model::fromXml(juce::XmlElement const& xml, Model defaultModel)
{
    anlWeakAssert(xml.hasTagName("Zoom::State::Model"));
    if(!xml.hasTagName("Zoom::State::Model"))
    {
        return {};
    }
    
    anlWeakAssert(xml.hasAttribute("globalRange::start"));
    anlWeakAssert(xml.hasAttribute("globalRange::end"));
    anlWeakAssert(xml.hasAttribute("visibleRange::start"));
    anlWeakAssert(xml.hasAttribute("visibleRange::end"));
    anlWeakAssert(xml.hasAttribute("minimumLength"));
    
    defaultModel.globalRange = Tools::StringParser::fromXml(xml, "globalRange", defaultModel.globalRange);
    defaultModel.visibleRange = Tools::StringParser::fromXml(xml, "visibleRange", defaultModel.visibleRange);
    defaultModel.minimumLength = xml.getDoubleAttribute("minimumLength", defaultModel.minimumLength);
    
    return defaultModel;
}

std::unique_ptr<juce::XmlElement> Zoom::State::Model::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement>("Zoom::State::Model");
    if(xml == nullptr)
    {
        return nullptr;
    }
    
    xml->setAttribute("globalRange", Tools::StringParser::toString(globalRange));
    xml->setAttribute("visibleRange", Tools::StringParser::toString(visibleRange));
    xml->setAttribute("minimumLength", minimumLength);
    return xml;
}

std::set<Zoom::State::Model::Attribute> Zoom::State::Model::getAttributeTypes()
{
    return {Attribute::globalRange, Attribute::visibleRange, Attribute::minimumLength};
}

bool Zoom::State::Model::operator!=(Model const& other) const
{
    auto equals = [](juce::Range<double> const& lhs, juce::Range<double> const& rhs)
    {
        static auto constexpr epsilon = std::numeric_limits<double>::epsilon();
        return std::abs(lhs.getStart() - rhs.getStart()) < epsilon && std::abs(lhs.getEnd() - rhs.getEnd()) < epsilon;
    };
    
    return !equals(globalRange, other.globalRange) || !equals(visibleRange, other.visibleRange) || std::abs(minimumLength - other.minimumLength) > std::numeric_limits<double>::epsilon();
}

bool Zoom::State::Model::operator==(Model const& other) const
{
    return !(*this != other);
}

void Zoom::State::Accessor::fromModel(Model const& model, juce::NotificationType const notification)
{
    using Attribute = Model::Attribute;
    std::set<Attribute> attributes;
    auto sanitizeRange = [&](juce::Range<double> const& input)
    {
        return input.withEnd(std::max(input.getStart() + mModel.minimumLength, input.getEnd()));
    };
    compareAndSet(attributes, Attribute::minimumLength, mModel.minimumLength, std::max(model.minimumLength, 0.0));
    compareAndSet(attributes, Attribute::globalRange, mModel.globalRange, sanitizeRange(model.globalRange));
    compareAndSet(attributes, Attribute::visibleRange, mModel.visibleRange, model.globalRange.constrainRange(sanitizeRange(model.visibleRange)));
    notifyListener(attributes, notification);
}

ANALYSE_FILE_END
