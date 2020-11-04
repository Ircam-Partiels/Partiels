#include "AnlZoomStateModel.h"
#include "../Tools/AnlStringParser.h"

ANALYSE_FILE_BEGIN

Zoom::State::Model::Model(juce::Range<double> vr)
: range(vr)
{
}

Zoom::State::Model Zoom::State::Model::fromXml(juce::XmlElement const& xml, Model defaultModel)
{
    anlWeakAssert(xml.hasTagName("Zoom::State::Model"));
    if(!xml.hasTagName("Zoom::State::Model"))
    {
        return {};
    }
    
    anlWeakAssert(xml.hasAttribute("range"));
    
    defaultModel.range = Tools::StringParser::fromXml(xml, "range", defaultModel.range);
    
    return defaultModel;
}

std::unique_ptr<juce::XmlElement> Zoom::State::Model::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement>("Zoom::State::Model");
    if(xml == nullptr)
    {
        return nullptr;
    }
    
    xml->setAttribute("range", Tools::StringParser::toString(range));
    return xml;
}

bool Zoom::State::Model::operator!=(Model const& other) const
{
    auto equals = [](juce::Range<double> const& lhs, juce::Range<double> const& rhs)
    {
        static auto constexpr epsilon = std::numeric_limits<double>::epsilon();
        return std::abs(lhs.getStart() - rhs.getStart()) < epsilon && std::abs(lhs.getEnd() - rhs.getEnd()) < epsilon;
    };
    
    return !equals(range, other.range);
}

bool Zoom::State::Model::operator==(Model const& other) const
{
    return !(*this != other);
}

Zoom::State::Accessor::Accessor(Model& model, juce::Range<double> const& globalRange, double minimumLength)
: Tools::ModelAccessor<Accessor, Model, Model::Attribute>(model)
{
    setContraints(globalRange, minimumLength, juce::NotificationType::dontSendNotification);
}

void Zoom::State::Accessor::fromModel(Model const& model, juce::NotificationType const notification)
{
    using Attribute = Model::Attribute;
    std::set<Attribute> attributes;
    auto sanitizeRange = [&](juce::Range<double> const& input)
    {
        if(mGlobalRange.isEmpty())
        {
            return input.withEnd(std::max(input.getStart() + mMinimumLength, input.getEnd()));
        }
        return mGlobalRange.constrainRange(input.withEnd(std::max(input.getStart() + mMinimumLength, input.getEnd())));
    };
    compareAndSet(attributes, Attribute::range, mModel.range, sanitizeRange(model.range));
    notifyListener(attributes, notification);
}

void Zoom::State::Accessor::setContraints(juce::Range<double> const& globalRange, double minimumLength, juce::NotificationType const notification)
{
    mGlobalRange = globalRange;
    mMinimumLength = std::min(std::max(minimumLength, 0.0), mGlobalRange.getLength());
    fromModel(mModel, notification);
}

std::tuple<juce::Range<double>, double> Zoom::State::Accessor::getContraints() const
{
    return std::make_tuple(mGlobalRange, mMinimumLength);
}

ANALYSE_FILE_END
