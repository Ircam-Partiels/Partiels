
#include "AnlApplicationController.h"

ANALYSE_FILE_BEGIN

Application::Controller::Controller(Model& model)
: mModel(model)
{
}

Application::Model const& Application::Controller::getModel() const
{
    return mModel;
}

juce::String Application::Controller::getWindowState() const
{
    return mModel.windowState;
}

std::vector<juce::File> const& Application::Controller::getRecentlyOpenedFilesList() const
{
    return mModel.recentlyOpenedFilesList;
}

std::vector<juce::File> const& Application::Controller::getCurrentOpenedFilesList() const
{
    return mModel.currentOpenedFilesList;
}

juce::File Application::Controller::getCurrentDocumentFile() const
{
    return mModel.currentDocumentFile;
}

void Application::Controller::setWindowState(juce::String const& state, juce::NotificationType const notification)
{
    if(mModel.windowState != state)
    {
        mModel.windowState = state;
        notifyListeners(Listener::AttributeType::windowState, {mModel.windowState}, notification);
    }
}

void Application::Controller::setRecentlyOpenedFilesList(std::vector<juce::File> files, juce::NotificationType const notification)
{
    Model::sanitize(files);
    if(mModel.recentlyOpenedFilesList != files)
    {
        mModel.recentlyOpenedFilesList = files;
        notifyListeners(Listener::AttributeType::recentlyOpenedFilesList, Model::toString(files), notification);
    }
}

void Application::Controller::setCurrentOpenedFilesList(std::vector<juce::File> files, juce::NotificationType const notification)
{
    Model::sanitize(files);
    if(mModel.currentOpenedFilesList != files)
    {
        mModel.currentOpenedFilesList = files;
        notifyListeners(Listener::AttributeType::currentOpenedFilesList, Model::toString(files), notification);
    }
}

void Application::Controller::setCurrentDocumentFile(juce::File const& file, juce::NotificationType const notification)
{
    if(mModel.currentDocumentFile != file)
    {
        mModel.currentDocumentFile = file;
        notifyListeners(Listener::AttributeType::currentDocumentFile, {file.getFullPathName()}, notification);
    }
}

void Application::Controller::fromModel(Application::Model const& model, juce::NotificationType const notification)
{
    setWindowState(model.windowState, notification);
    setRecentlyOpenedFilesList(model.recentlyOpenedFilesList, notification);
    setCurrentOpenedFilesList(model.currentOpenedFilesList, notification);
    setCurrentDocumentFile(model.currentDocumentFile, notification);
}

void Application::Controller::fromXml(juce::XmlElement const& xml, juce::NotificationType const notification)
{
    fromModel(Model::fromXml(xml), notification);
}

std::unique_ptr<juce::XmlElement> Application::Controller::toXml() const
{
    return mModel.toXml();
}

void Application::Controller::addListener(Listener* listener, juce::NotificationType const notification)
{
    mListeners.add(listener);
    if(listener && notification != juce::dontSendNotification)
    {
        ilfStrongAssert(notification != juce::sendNotificationAsync);
        listener->applicationAttributeChanged(this, Listener::AttributeType::recentlyOpenedFilesList, Model::toString(mModel.recentlyOpenedFilesList));
        listener->applicationAttributeChanged(this, Listener::AttributeType::currentOpenedFilesList, Model::toString(mModel.currentOpenedFilesList));
        listener->applicationAttributeChanged(this, Listener::AttributeType::currentDocumentFile, {mModel.currentDocumentFile.getFullPathName()});
        listener->applicationAttributeChanged(this, Listener::AttributeType::windowState, {mModel.windowState});
    }
}

void Application::Controller::removeListener(Listener* listener)
{
    mListeners.remove(listener);
}

void Application::Controller::notifyListeners(Listener::AttributeType type, juce::var const& value, juce::NotificationType const notification)
{
    ilfStrongAssert(notification != juce::NotificationType::sendNotificationAsync);
    if(notification != juce::NotificationType::dontSendNotification)
    {
        mListeners.call(&Listener::applicationAttributeChanged, this, type, value);
    }
}


ANALYSE_FILE_END

