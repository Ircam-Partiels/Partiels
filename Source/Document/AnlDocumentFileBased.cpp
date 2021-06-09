#include "AnlDocumentFileBased.h"

ANALYSE_FILE_BEGIN

Document::AttrContainer const& Document::FileBased::getDefaultContainer()
{
    static AttrContainer const document{{{}}, {}, {}};
    return document;
}

Document::FileBased::FileBased(Accessor& accessor, Director& director, juce::String const& fileExtension, juce::String const& fileWildCard, juce::String const& openFileDialogTitle, juce::String const& saveFileDialogTitle)
: juce::FileBasedDocument(fileExtension, fileWildCard, juce::translate(openFileDialogTitle), juce::translate(saveFileDialogTitle))
, mAccessor(accessor)
, mDirector(director)
, mFileExtension(fileExtension)
{
    mSavedStateAccessor.copyFrom(mAccessor, NotificationType::synchronous);
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        triggerAsyncUpdate();
    };

    mListener.onAccessorInserted = [&](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::tracks:
            {
                auto& trackAcsr = mAccessor.getAcsr<AcsrType::tracks>(index);
                mTrackAccessors.insert(mTrackAccessors.begin() + static_cast<long>(index), std::ref(trackAcsr));
                trackAcsr.addListener(mTrackListener, NotificationType::synchronous);
            }
            break;
            case AcsrType::groups:
            {
                auto& groupAcsr = mAccessor.getAcsr<AcsrType::groups>(index);
                mGroupAccessors.insert(mGroupAccessors.begin() + static_cast<long>(index), std::ref(groupAcsr));
                groupAcsr.addListener(mGroupListener, NotificationType::synchronous);
            }
            break;
            case AcsrType::timeZoom:
                break;
            case AcsrType::transport:
                break;
        }
    };

    mListener.onAccessorErased = [&](Accessor const& acsr, AcsrType type, size_t index)
    {
        triggerAsyncUpdate();
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::tracks:
            {
                mTrackAccessors[index].get().removeListener(mTrackListener);
                mTrackAccessors.erase(mTrackAccessors.begin() + static_cast<long>(index));
            }
            break;
            case AcsrType::groups:
            {
                mGroupAccessors[index].get().removeListener(mGroupListener);
                mGroupAccessors.erase(mGroupAccessors.begin() + static_cast<long>(index));
            }
            break;
            case AcsrType::timeZoom:
                break;
            case AcsrType::transport:
                break;
        }
    };

    mTrackListener.onAttrChanged = [&](Track::Accessor const& acsr, Track::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        triggerAsyncUpdate();
    };

    mGroupListener.onAttrChanged = [&](Group::Accessor const& acsr, Group::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        triggerAsyncUpdate();
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::FileBased::~FileBased()
{
    for(auto& trackAcsr : mTrackAccessors)
    {
        trackAcsr.get().removeListener(mTrackListener);
    }
    for(auto& groupAcsr : mGroupAccessors)
    {
        groupAcsr.get().removeListener(mGroupListener);
    }
    mAccessor.removeListener(mListener);
}

juce::String Document::FileBased::getDocumentTitle()
{
    auto const file = getFile();
    if(file.existsAsFile())
    {
        return file.getFileNameWithoutExtension();
    }
    auto const files = mAccessor.getAttr<AttrType::reader>();
    if(!files.empty())
    {
        return files.front().file.getFileNameWithoutExtension();
    }
    return "";
}

juce::Result Document::FileBased::loadDocument(juce::File const& file)
{
    auto xml = juce::XmlDocument::parse(file);
    if(xml == nullptr || !xml->hasTagName("document"))
    {
        return juce::Result::fail(juce::translate("The file FLNM cannot be parsed!").replace("FLNM", file.getFileName()));
    }
    auto const viewport = XmlParser::fromXml(*xml.get(), "viewport", juce::Point<int>());
    mAccessor.fromXml(*xml.get(), {"document"}, NotificationType::synchronous);
    mDirector.sanitize(NotificationType::synchronous);
    auto var = std::make_unique<juce::DynamicObject>();
    if(var != nullptr)
    {
        var->setProperty("x", viewport.getX());
        var->setProperty("y", viewport.getY());
        mAccessor.sendSignal(SignalType::viewport, var.release(), NotificationType::synchronous);
    }
    mSavedStateAccessor.copyFrom(mAccessor, NotificationType::synchronous);
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
    mSavedStateAccessor.copyFrom(mAccessor, NotificationType::synchronous);
    triggerAsyncUpdate();
    return juce::Result::ok();
}

juce::Result Document::FileBased::loadBackup(juce::File const& file)
{
    auto xml = juce::XmlDocument::parse(file);
    if(xml == nullptr || !xml->hasTagName("document"))
    {
        return juce::Result::fail(juce::translate("The file FLNM cannot be parsed!").replace("FLNM", file.getFileName()));
    }
    if(xml->hasAttribute("origin"))
    {
        setFile(juce::File(xml->getStringAttribute("origin")));
    }
    else
    {
        setFile({});
    }
    mAccessor.copyFrom({getDefaultContainer()}, NotificationType::synchronous);
    auto const viewport = XmlParser::fromXml(*xml.get(), "viewport", juce::Point<int>());
    mAccessor.fromXml(*xml.get(), {"document"}, NotificationType::synchronous);
    mDirector.sanitize(NotificationType::synchronous);
    auto var = std::make_unique<juce::DynamicObject>();
    if(var != nullptr)
    {
        var->setProperty("x", viewport.getX());
        var->setProperty("y", viewport.getY());
        mAccessor.sendSignal(SignalType::viewport, var.release(), NotificationType::synchronous);
    }
    triggerAsyncUpdate();
    return juce::Result::ok();
}

juce::Result Document::FileBased::saveBackup(juce::File const& file)
{
    auto xml = mAccessor.toXml("document");
    if(xml == nullptr)
    {
        return juce::Result::fail(juce::translate("The document cannot be parsed!"));
    }
    if(getFile().existsAsFile())
    {
        xml->setAttribute("origin", getFile().getFullPathName());
    }
    if(!xml->writeTo(file))
    {
        return juce::Result::fail(juce::translate("The document cannot written to the file FLNM!").replace("FLNM", file.getFileName()));
    }
    return juce::Result::ok();
}

juce::File Document::FileBased::getLastDocumentOpened()
{
    auto const file = getFile();
    if(file.existsAsFile())
    {
        return file;
    }
    auto const files = mAccessor.getAttr<AttrType::reader>();
    if(!files.empty())
    {
        return files.front().file.withFileExtension(mFileExtension);
    }
    return mLastFile.getParentDirectory();
}

void Document::FileBased::setLastDocumentOpened(juce::File const& file)
{
    mLastFile = file;
}

void Document::FileBased::handleAsyncUpdate()
{
    changed();
    sendChangeMessage();
}

void Document::FileBased::changed()
{
    if(getFile() == juce::File{})
    {
        auto const state = mAccessor.isEquivalentTo(getDefaultContainer());
        setChangedFlag(!state);
    }
    else
    {
        auto const state = mAccessor.isEquivalentTo(mSavedStateAccessor);
        setChangedFlag(!state);
    }
}

ANALYSE_FILE_END
