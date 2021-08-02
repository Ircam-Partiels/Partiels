#include "AnlDocumentFileBased.h"

ANALYSE_FILE_BEGIN

Document::Accessor const& Document::FileBased::getDefaultAccessor()
{
    return mDefaultDdocument;
}

Document::FileBased::FileBased(Director& director, juce::String const& fileExtension, juce::String const& fileWildCard, juce::String const& openFileDialogTitle, juce::String const& saveFileDialogTitle)
: juce::FileBasedDocument(fileExtension, fileWildCard, juce::translate(openFileDialogTitle), juce::translate(saveFileDialogTitle))
, mDirector(director)
, mAccessor(mDirector.getAccessor())
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
    if(xml->getIntAttribute("AnlModelVersion", 0) > ProjectInfo::versionNumber)
    {
        return juce::Result::fail("The file FLNM has been created with a newer version of Partiels. Update the version of the application to load the file.");
    }

    auto const viewport = XmlParser::fromXml(*xml.get(), "viewport", juce::Point<int>());
    auto const original = XmlParser::fromXml(*xml.get(), "path", file);
    auto const oldParent = original.getParentDirectory().getFullPathName();
    auto const newParent = file.getParentDirectory().getFullPathName();
    if(oldParent != newParent)
    {
        std::function<void(juce::XmlElement&)> replacePath = [&](juce::XmlElement& element)
        {
            for(int i = 0; i < element.getNumAttributes(); ++i)
            {
                auto const& currentValue = element.getAttributeValue(i);
                if(currentValue.contains(oldParent))
                {
                    element.setAttribute(element.getAttributeName(i), currentValue.replace(oldParent, newParent));
                }
            }
            for(auto* child : element.getChildIterator())
            {
                if(child != nullptr)
                {
                    replacePath(*child);
                }
            }
        };
        replacePath(*xml.get());
    }
    AlertWindow::Catcher catcher;
    mDirector.setAlertCatcher(&catcher);
    mAccessor.fromXml(*xml.get(), {"document"}, NotificationType::synchronous);
    mDirector.sanitize(NotificationType::synchronous);
    mDirector.setAlertCatcher(nullptr);
    for(auto const& message : catcher.getMessages())
    {
        auto const type = std::get<0>(message.first);
        auto const title = std::get<1>(message.first) + (message.second.size() > 1 ? "(" + juce::String(message.second.size()) + ")" : "");
        AlertWindow::showMessage(type, title, message.second.joinIntoString("\n"));
    }

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
    auto const previousPath = mAccessor.getAttr<AttrType::path>();
    mAccessor.setAttr<AttrType::path>(file, NotificationType::synchronous);
    auto xml = mAccessor.toXml("document");
    if(xml == nullptr)
    {
        mAccessor.setAttr<AttrType::path>(previousPath, NotificationType::synchronous);
        return juce::Result::fail(juce::translate("The document cannot be parsed!"));
    }
    if(!xml->writeTo(file))
    {
        mAccessor.setAttr<AttrType::path>(previousPath, NotificationType::synchronous);
        return juce::Result::fail(juce::translate("The document cannot written to the file FLNM!").replace("FLNM", file.getFileName()));
    }
    mSavedStateAccessor.copyFrom(mAccessor, NotificationType::synchronous);
    triggerAsyncUpdate();
    return juce::Result::ok();
}

juce::Result Document::FileBased::consolidate()
{
    auto file = getFile();
    if(!file.existsAsFile())
    {
        return juce::Result::fail("The document file doesn't exist!");
    }
    auto const subDirectory = file.getParentDirectory().getChildFile(file.getFileNameWithoutExtension() + "_" + "ConsolidatedFiles");
    auto result = subDirectory.createDirectory();
    if(result.failed())
    {
        return result;
    }
    mDirector.startAction();
    result = mDirector.consolidate(subDirectory);
    if(result.failed())
    {
        mDirector.endAction(ActionState::abort);
        return result;
    }
    result = saveDocument(file);
    if(result.failed())
    {
        mDirector.endAction(ActionState::abort);
        return result;
    }
    mDirector.endAction(ActionState::newTransaction, "Consolidate Document");
    return juce::Result::ok();
}

juce::Result Document::FileBased::loadTemplate(juce::File const& file, bool adaptOnSampleRate)
{
    auto xml = juce::XmlDocument::parse(file);
    if(xml == nullptr || !xml->hasTagName("document"))
    {
        return juce::Result::fail(juce::translate("The file FLNM cannot be parsed!").replace("FLNM", file.getFileName()));
    }
    if(xml->getIntAttribute("AnlModelVersion", 0) > ProjectInfo::versionNumber)
    {
        return juce::Result::fail("The file FLNM has been created with a newer version of Partiels. Update the version of the application to load the file.");
    }

    auto const viewport = XmlParser::fromXml(*xml.get(), "viewport", juce::Point<int>());

    Accessor tempAcsr;
    tempAcsr.fromXml(*xml.get(), {"document"}, NotificationType::synchronous);
    tempAcsr.setAttr<AttrType::reader>(mAccessor.getAttr<AttrType::reader>(), NotificationType::synchronous);
    tempAcsr.setAttr<AttrType::path>(mAccessor.getAttr<AttrType::path>(), NotificationType::synchronous);
    auto const tempSampleRate = tempAcsr.getAttr<AttrType::samplerate>();
    auto const currentSampleRate = mAccessor.getAttr<AttrType::samplerate>();
    auto const ratio = tempSampleRate > 0.0 ? currentSampleRate / tempSampleRate : 1.0;
    for(auto const acsr : tempAcsr.getAcsrs<AcsrType::tracks>())
    {
        auto const resultFile = acsr.get().getAttr<Track::AttrType::file>();
        if(resultFile.hasFileExtension("dat"))
        {
            acsr.get().setAttr<Track::AttrType::file>(juce::File{}, NotificationType::synchronous);
        }
        if(adaptOnSampleRate)
        {
            auto state = acsr.get().getAttr<Track::AttrType::state>();
            state.blockSize = static_cast<size_t>(std::round(static_cast<double>(state.blockSize) * ratio));
            state.stepSize = static_cast<size_t>(std::round(static_cast<double>(state.stepSize) * ratio));
            acsr.get().setAttr<Track::AttrType::state>(state, NotificationType::synchronous);
        }
    }

    AlertWindow::Catcher catcher;
    mDirector.setAlertCatcher(&catcher);
    mAccessor.copyFrom(tempAcsr, NotificationType::synchronous);
    mDirector.sanitize(NotificationType::synchronous);
    mDirector.setAlertCatcher(nullptr);

    for(auto const& message : catcher.getMessages())
    {
        auto const type = std::get<0>(message.first);
        auto const title = std::get<1>(message.first) + (message.second.size() > 1 ? "(" + juce::String(message.second.size()) + ")" : "");
        AlertWindow::showMessage(type, title, message.second.joinIntoString("\n"));
    }

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

juce::Result Document::FileBased::loadBackup(juce::File const& file)
{
    auto xml = juce::XmlDocument::parse(file);
    if(xml == nullptr || !xml->hasTagName("document"))
    {
        return juce::Result::fail(juce::translate("The file FLNM cannot be parsed!").replace("FLNM", file.getFileName()));
    }
    if(xml->getIntAttribute("AnlModelVersion", 0) > ProjectInfo::versionNumber)
    {
        return juce::Result::fail("The file FLNM has been created with a newer version of Partiels. Update the version of the application to load the file.");
    }

    if(xml->hasAttribute("origin"))
    {
        setFile(juce::File(xml->getStringAttribute("origin")));
    }
    else
    {
        setFile({});
    }

    AlertWindow::Catcher catcher;
    mDirector.setAlertCatcher(&catcher);
    mAccessor.copyFrom(getDefaultAccessor(), NotificationType::synchronous);
    auto const viewport = XmlParser::fromXml(*xml.get(), "viewport", juce::Point<int>());
    mAccessor.fromXml(*xml.get(), {"document"}, NotificationType::synchronous);
    mDirector.sanitize(NotificationType::synchronous);
    mDirector.setAlertCatcher(nullptr);
    for(auto const& message : catcher.getMessages())
    {
        auto const type = std::get<0>(message.first);
        auto const title = std::get<1>(message.first) + (message.second.size() > 1 ? "(" + juce::String(message.second.size()) + ")" : "");
        AlertWindow::showMessage(type, title, message.second.joinIntoString("\n"));
    }

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
    mAccessor.setAttr<AttrType::path>(getFile(), NotificationType::synchronous);
    if(getFile() == juce::File{})
    {
        auto const state = mAccessor.isEquivalentTo(getDefaultAccessor());
        setChangedFlag(!state);
    }
    else
    {
        auto const state = mAccessor.isEquivalentTo(mSavedStateAccessor);
        setChangedFlag(!state);
    }
}

ANALYSE_FILE_END
