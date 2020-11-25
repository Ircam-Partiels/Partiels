
#include "ilf_LayoutManagerStrechableContainerController.h"

ILF_WARNING_BEGIN
ILF_NAMESPACE_BEGIN

LayoutManager::Strechable::Container::Controller::Controller(Model& model)
: mModel(model)
{
    
}

LayoutManager::Strechable::Container::Model const& LayoutManager::Strechable::Container::Controller::getModel() const
{
    return mModel;
}

std::vector<int> LayoutManager::Strechable::Container::Controller::getSizes() const
{
    return mModel.sizes;
}

void LayoutManager::Strechable::Container::Controller::setSizes(std::vector<int> sizes, juce::NotificationType const notification)
{
    if(mModel.sizes != sizes)
    {
        mModel.sizes = sizes;
        notifyListeners(Listener::AttributeType::sizes, {sizes.data(), sizes.size()}, notification);
    }
}

void LayoutManager::Strechable::Container::Controller::fromModel(Model const& model, juce::NotificationType const notification)
{
    setSizes(model.sizes, notification);
}

void LayoutManager::Strechable::Container::Controller::fromXml(juce::XmlElement const& xml, juce::NotificationType const notification)
{
    fromModel(Model::fromXml(xml), notification);
}

std::unique_ptr<juce::XmlElement> LayoutManager::Strechable::Container::Controller::toXml() const
{
    return mModel.toXml();
}

void LayoutManager::Strechable::Container::Controller::addListener(Listener* listener, juce::NotificationType const notification)
{
    mListeners.add(listener);
    if(listener && notification != juce::dontSendNotification)
    {
        ilfStrongAssert(notification != juce::sendNotificationAsync);
        listener->layoutManagerStrechableContainerAttributeChanged(this, Listener::AttributeType::sizes, {mModel.sizes.data(), mModel.sizes.size()});
    }
}

void LayoutManager::Strechable::Container::Controller::removeListener(Listener* listener)
{
    mListeners.remove(listener);
}

void LayoutManager::Strechable::Container::Controller::notifyListeners(Listener::AttributeType type, juce::var const& value, juce::NotificationType const notification)
{
    if(notification != juce::NotificationType::dontSendNotification)
    {
        ilfStrongAssert(notification != juce::sendNotificationAsync);
        mListeners.call(&Listener::layoutManagerStrechableContainerAttributeChanged, this, type, value);
    }
}

ILF_NAMESPACE_END
ILF_WARNING_END
