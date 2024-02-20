#include "AnlDocumentFileBased.h"
#include "../Track/AnlTrackExporter.h"
#include "AnlDocumentExporter.h"

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
    auto fileResult = parse(file);
    if(fileResult.index() == 1_z)
    {
        return *std::get_if<1>(&fileResult);
    }
    auto xml = std::move(*std::get_if<0>(&fileResult));

    auto const original = XmlParser::fromXml(*xml.get(), "path", file);
    auto const oldDirectory = getConsolidateDirectory(original);
    auto const newDirectory = getConsolidateDirectory(file);
    replacePath(*xml.get(), oldDirectory.getFullPathName(), newDirectory.getFullPathName());

    AlertWindow::Catcher catcher;
    mDirector.setAlertCatcher(&catcher);
    mAccessor.fromXml(*xml.get(), {"document"}, NotificationType::synchronous);
    [[maybe_unused]] auto const references = mDirector.sanitize(NotificationType::synchronous);
    mDirector.setAlertCatcher(nullptr);
    catcher.showAsync();

    auto var = std::make_unique<juce::DynamicObject>();
    if(var != nullptr)
    {
        auto const viewport = XmlParser::fromXml(*xml.get(), "viewport", juce::Point<int>());
        var->setProperty("x", viewport.getX());
        var->setProperty("y", viewport.getY());
        mAccessor.sendSignal(SignalType::viewport, var.release(), NotificationType::synchronous);
    }

    mSavedStateAccessor.copyFrom(mAccessor, NotificationType::synchronous);

    triggerAsyncUpdate();
    return juce::Result::ok();
}

juce::File Document::FileBased::getConsolidateDirectory(juce::File const& file)
{
    return file.getParentDirectory().getChildFile(file.getFileNameWithoutExtension() + "_" + "ConsolidatedFiles");
}

juce::Result Document::FileBased::saveDocument(juce::File const& file)
{
    mDirector.startAction();
    auto const trackResult = Exporter::consolidateTrackFiles(mAccessor, getConsolidateDirectory(file));
    if(trackResult.failed())
    {
        mDirector.endAction(ActionState::abort);
        return trackResult;
    }
    auto const saveResult = saveTo(mAccessor, file);
    if(saveResult.wasOk())
    {
        mDirector.endAction(ActionState::continueTransaction);
        mSavedStateAccessor.copyFrom(mAccessor, NotificationType::synchronous);
        triggerAsyncUpdate();
        return saveResult;
    }
    mDirector.endAction(ActionState::abort);
    triggerAsyncUpdate();
    return saveResult;
}

juce::Result Document::FileBased::consolidate()
{
    auto file = getFile();
    if(!file.existsAsFile())
    {
        return juce::Result::fail("The document file doesn't exist!");
    }

    auto trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
    if(std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                   {
                       auto const& processing = trackAcsr.get().template getAttr<Track::AttrType::processing>();
                       return std::get<0>(processing) || std::get<2>(processing);
                   }))
    {
        return juce::Result::fail("The analysis is running!");
    }

    mDirector.startAction();
    AlertWindow::Catcher catcher;
    mDirector.setAlertCatcher(&catcher);
    auto const directory = getConsolidateDirectory(file);

    auto const audioResult = Exporter::consolidateAudioFiles(mAccessor, directory);
    if(audioResult.failed())
    {
        mDirector.endAction(ActionState::abort);
        return audioResult;
    }
    mDirector.setAlertCatcher(nullptr);

    // Create a commmit for all tracks to for consolidation
    for(auto& trackAcsr : trackAcsrs)
    {
        auto trackFileInfo = trackAcsr.get().getAttr<Track::AttrType::file>();
        if(trackFileInfo.commit.isEmpty())
        {
            trackFileInfo.commit = juce::Uuid().toString();
            trackAcsr.get().setAttr<Track::AttrType::file>(trackFileInfo, NotificationType::synchronous);
        }
    }
    auto const trackResult = Exporter::consolidateTrackFiles(mAccessor, directory);
    if(trackResult.failed())
    {
        mDirector.endAction(ActionState::abort);
        return trackResult;
    }
    mDirector.endAction(ActionState::newTransaction, juce::translate("Consolidate Document"));

    auto const audioClearResult = Exporter::clearUnusedAudioFiles(mAccessor, directory);
    auto const trackClearResult = Exporter::clearUnusedTrackFiles(mAccessor, directory);
    if(audioClearResult.failed())
    {
        return audioClearResult;
    }
    if(trackClearResult.failed())
    {
        return trackClearResult;
    }

    return saveDocument(file);
}

juce::Result Document::FileBased::loadTemplate(juce::File const& file, bool adaptOnSampleRate)
{
    auto fileResult = parse(file);
    if(fileResult.index() == 1_z)
    {
        return *std::get_if<1>(&fileResult);
    }
    auto xml = std::move(*std::get_if<0>(&fileResult));

    AlertWindow::Catcher catcher;
    mDirector.setAlertCatcher(&catcher);

    loadTemplate(mAccessor, *xml.get(), adaptOnSampleRate);
    [[maybe_unused]] auto const references = mDirector.sanitize(NotificationType::synchronous);

    mDirector.setAlertCatcher(nullptr);
    mSavedStateAccessor.copyFrom(mAccessor, NotificationType::synchronous);
    triggerAsyncUpdate();

    for(auto const& message : catcher.getMessages())
    {
        auto const type = std::get<0>(message.first);
        auto const title = std::get<1>(message.first) + (message.second.size() > 1 ? "(" + juce::String(message.second.size()) + ")" : "");

        auto const options = juce::MessageBoxOptions()
                                 .withIconType(static_cast<juce::AlertWindow::AlertIconType>(type))
                                 .withTitle(title)
                                 .withMessage(message.second.joinIntoString("\n"))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    }

    return juce::Result::ok();
}

juce::Result Document::FileBased::loadBackup(juce::File const& file)
{
    auto fileResult = parse(file);
    if(fileResult.index() == 1_z)
    {
        return *std::get_if<1>(&fileResult);
    }
    auto const xml = std::move(*std::get_if<0>(&fileResult));

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
    mAccessor.fromXml(*xml.get(), {"document"}, NotificationType::synchronous);
    [[maybe_unused]] auto const references = mDirector.sanitize(NotificationType::synchronous);
    mDirector.setAlertCatcher(nullptr);
    catcher.showAsync();

    auto var = std::make_unique<juce::DynamicObject>();
    if(var != nullptr)
    {
        auto const viewport = XmlParser::fromXml(*xml.get(), "viewport", juce::Point<int>());
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
    auto const currentFile = getFile();
    if(currentFile.existsAsFile())
    {
        xml->setAttribute("origin", currentFile.getFullPathName());
    }
    if(!xml->writeTo(file))
    {
        return juce::Result::fail(juce::translate("The document cannot be written to the file FLNM!").replace("FLNM", file.getFileName()));
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

std::variant<std::unique_ptr<juce::XmlElement>, juce::Result> Document::FileBased::parse(juce::File const& file)
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
    return xml;
}

juce::Result Document::FileBased::saveTo(Accessor& accessor, juce::File const& file)
{
    auto const previousPath = accessor.getAttr<AttrType::path>();
    accessor.setAttr<AttrType::path>(file, NotificationType::synchronous);
    auto xml = accessor.toXml("document");
    if(xml == nullptr)
    {
        accessor.setAttr<AttrType::path>(previousPath, NotificationType::synchronous);
        return juce::Result::fail(juce::translate("The document cannot be parsed!"));
    }
    if(!xml->writeTo(file))
    {
        accessor.setAttr<AttrType::path>(previousPath, NotificationType::synchronous);
        return juce::Result::fail(juce::translate("The document cannot be written to the file FLNM!").replace("FLNM", file.getFileName()));
    }
    return juce::Result::ok();
}

void Document::FileBased::loadTemplate(Accessor& accessor, juce::XmlElement const& xml, bool adaptOnSampleRate)
{
    auto const viewport = XmlParser::fromXml(xml, "viewport", juce::Point<int>());

    Accessor tempAcsr;
    tempAcsr.fromXml(xml, {"document"}, NotificationType::synchronous);
    tempAcsr.setAttr<AttrType::reader>(accessor.getAttr<AttrType::reader>(), NotificationType::synchronous);
    tempAcsr.setAttr<AttrType::path>(accessor.getAttr<AttrType::path>(), NotificationType::synchronous);
    auto const tempSampleRate = tempAcsr.getAttr<AttrType::samplerate>();
    auto const currentSampleRate = accessor.getAttr<AttrType::samplerate>();
    auto const ratio = tempSampleRate > 0.0 ? currentSampleRate / tempSampleRate : 1.0;
    for(auto const& acsr : tempAcsr.getAcsrs<AcsrType::tracks>())
    {
        auto trackFileInfo = acsr.get().getAttr<Track::AttrType::file>();
        if(trackFileInfo.commit.isNotEmpty())
        {
            trackFileInfo.commit.clear();
            acsr.get().setAttr<Track::AttrType::file>(trackFileInfo, NotificationType::synchronous);
        }
        if(adaptOnSampleRate)
        {
            auto state = acsr.get().getAttr<Track::AttrType::state>();
            if(acsr.get().getAttr<Track::AttrType::description>().defaultState.blockSize == 0_z)
            {
                state.blockSize = static_cast<size_t>(std::round(static_cast<double>(state.blockSize) * ratio));
            }
            if(acsr.get().getAttr<Track::AttrType::description>().defaultState.stepSize != 0_z)
            {
                state.stepSize = static_cast<size_t>(std::round(static_cast<double>(state.stepSize) * ratio));
            }
            acsr.get().setAttr<Track::AttrType::state>(state, NotificationType::synchronous);
        }
    }

    accessor.copyFrom(tempAcsr, NotificationType::synchronous);

    auto var = std::make_unique<juce::DynamicObject>();
    if(var != nullptr)
    {
        var->setProperty("x", viewport.getX());
        var->setProperty("y", viewport.getY());
        accessor.sendSignal(SignalType::viewport, var.release(), NotificationType::synchronous);
    }
}

void Document::FileBased::replacePath(juce::XmlElement& element, juce::String const& oldPath, juce::String const& newPath)
{
    if(oldPath == newPath)
    {
        return;
    }
    for(int i = 0; i < element.getNumAttributes(); ++i)
    {
        auto const& currentValue = element.getAttributeValue(i);
        if(currentValue.contains(oldPath))
        {
            element.setAttribute(element.getAttributeName(i), currentValue.replace(oldPath, newPath));
        }
    }
    for(auto* child : element.getChildIterator())
    {
        if(child != nullptr)
        {
            replacePath(*child, oldPath, newPath);
        }
    }
}

ANALYSE_FILE_END
