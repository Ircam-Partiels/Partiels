#include "AnlDocumentFileBased.h"

ANALYSE_FILE_BEGIN


Document::FileBased::FileBased(Accessor& accessor, juce::String const& fileExtension, juce::String const& fileWildCard, juce::String const& openFileDialogTitle, juce::String const& saveFileDialogTitle)
: juce::FileBasedDocument(fileExtension, fileWildCard, openFileDialogTitle, saveFileDialogTitle)
, mAccessor(accessor)
{
    mListener.onChanged = [&](Accessor const& acsr, Attribute attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        triggerAsyncUpdate();
    };
    mAccessor.addListener(mListener, juce::NotificationType::dontSendNotification);
}

Document::FileBased::~FileBased()
{
    mAccessor.removeListener(mListener);
}

juce::String Document::FileBased::getDocumentTitle()
{
    return getFile().existsAsFile() ? getFile().getFileNameWithoutExtension() : (mAccessor.getModel().file.existsAsFile() ? mAccessor.getModel().file.getFileNameWithoutExtension() : "");
}

juce::Result Document::FileBased::loadDocument(juce::File const& file)
{
    auto xml = juce::XmlDocument::parse(file);
    if(xml == nullptr)
    {
        return juce::Result::fail(juce::translate("The file FLNM cannot be parsed!").replace("FLNM", file.getFileName()));
    }
    mAccessor.fromXml(*xml.get(), {}, juce::NotificationType::sendNotificationSync);
    mSavedXml = std::move(xml);
    triggerAsyncUpdate();
    return juce::Result::ok();
}

juce::Result Document::FileBased::saveDocument(juce::File const& file)
{
    auto xml = mAccessor.getModel().toXml();
    if(xml == nullptr)
    {
        return juce::Result::fail(juce::translate("The document cannot be parsed!"));
    }
    if(!xml->writeTo(file))
    {
        return juce::Result::fail(juce::translate("The document cannot written to the file FLNM!").replace("FLNM", file.getFileName()));
    }
    mSavedXml = std::move(xml);
    triggerAsyncUpdate();
    return juce::Result::ok();
}

juce::File Document::FileBased::getLastDocumentOpened()
{
    return getFile().existsAsFile() ? getFile() : (mAccessor.getModel().file.existsAsFile() ? mAccessor.getModel().file.getFullPathName() : mLastFile.getParentDirectory());
}

void Document::FileBased::setLastDocumentOpened(juce::File const& file)
{
    mLastFile = file;
}

void Document::FileBased::handleAsyncUpdate()
{
    auto getChangedState = [&]()
    {
        if(mSavedXml == nullptr && mAccessor.getModel() == Model())
        {
            return false;
        }
        auto xml = mAccessor.getModel().toXml();
        if(mSavedXml != nullptr && xml != nullptr)
        {
            return !mSavedXml->isEquivalentTo(xml.get(), true);
        }
        return true;
    };
    setChangedFlag(getChangedState());
}

ANALYSE_FILE_END
