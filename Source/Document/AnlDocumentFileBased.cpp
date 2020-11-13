#include "AnlDocumentFileBased.h"

ANALYSE_FILE_BEGIN


Document::FileBased::FileBased(Accessor& accessor, juce::String const& fileExtension, juce::String const& fileWildCard, juce::String const& openFileDialogTitle, juce::String const& saveFileDialogTitle)
: juce::FileBasedDocument(fileExtension, fileWildCard, openFileDialogTitle, saveFileDialogTitle)
, mAccessor(accessor)
{
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        triggerAsyncUpdate();
    };
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::FileBased::~FileBased()
{
    mAccessor.removeListener(mListener);
}

juce::String Document::FileBased::getDocumentTitle()
{
    return getFile().existsAsFile() ? getFile().getFileNameWithoutExtension() : (mAccessor.getValue<AttrType::file>().existsAsFile() ? mAccessor.getValue<AttrType::file>().getFileNameWithoutExtension() : "");
}

juce::Result Document::FileBased::loadDocument(juce::File const& file)
{
    auto xml = juce::XmlDocument::parse(file);
    if(xml == nullptr)
    {
        return juce::Result::fail(juce::translate("The file FLNM cannot be parsed!").replace("FLNM", file.getFileName()));
    }
    mAccessor.fromXml(*xml.get(), {"document"}, NotificationType::synchronous);
    mSavedState = mAccessor.getModel();
    triggerAsyncUpdate();
    return juce::Result::ok();
}

juce::Result Document::FileBased::saveDocument(juce::File const& file)
{
    auto xml = mAccessor.toXml("document");
    if(xml == nullptr)
    {
        return juce::Result::fail(juce::translate("The document cannot be parsed!"));
    }
    if(!xml->writeTo(file))
    {
        return juce::Result::fail(juce::translate("The document cannot written to the file FLNM!").replace("FLNM", file.getFileName()));
    }
    mSavedState = mAccessor.getModel();
    triggerAsyncUpdate();
    return juce::Result::ok();
}

juce::File Document::FileBased::getLastDocumentOpened()
{
    return getFile().existsAsFile() ? getFile() : (mAccessor.getValue<AttrType::file>().existsAsFile() ? mAccessor.getValue<AttrType::file>().getFullPathName() : mLastFile.getParentDirectory());
}

void Document::FileBased::setLastDocumentOpened(juce::File const& file)
{
    mLastFile = file;
}

void Document::FileBased::handleAsyncUpdate()
{
    setChangedFlag(!mAccessor.isEquivalentTo(mSavedState));
}

ANALYSE_FILE_END
