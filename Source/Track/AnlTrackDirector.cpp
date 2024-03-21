#include "AnlTrackDirector.h"
#include "../Plugin/AnlPluginProcessor.h"
#include "../Plugin/AnlPluginTools.h"
#include "AnlTrackExporter.h"
#include "AnlTrackProcessor.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Director::Director(Accessor& accessor, juce::UndoManager& undoManager, HierarchyManager& hierarchyManager, std::unique_ptr<juce::AudioFormatReader> audioFormatReader)
: mAccessor(accessor)
, mUndoManager(undoManager)
, mHierarchyManager(hierarchyManager)
, mAudioFormatReader(std::move(audioFormatReader))
{
    mSharedZoomListener.onAttrChanged = [=, this](Zoom::Accessor const& sharedZoomAcsr, Zoom::AttrType attribute)
    {
        std::unique_lock<std::mutex> lock(mSharedZoomMutex, std::try_to_lock);
        if(!lock.owns_lock())
        {
            return;
        }
        auto const updateZoom = [&](Zoom::Accessor& zoomAcsr)
        {
            if(zoomAcsr.getAttr<Zoom::AttrType::globalRange>().isEmpty())
            {
                return;
            }
            auto const range = Zoom::Tools::getScaledVisibleRange(sharedZoomAcsr, zoomAcsr.getAttr<Zoom::AttrType::globalRange>());
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
        };
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::visibleRange:
            {
                auto const frameType = Tools::getFrameType(mAccessor);
                if(frameType.has_value())
                {
                    switch(frameType.value())
                    {
                        case FrameType::label:
                            break;
                        case FrameType::value:
                            updateZoom(mAccessor.getAcsr<AcsrType::valueZoom>());
                            break;
                        case FrameType::vector:
                            updateZoom(mAccessor.getAcsr<AcsrType::binZoom>());
                            break;
                    }
                }
            }
            break;
            case Zoom::AttrType::minimumLength:
            case Zoom::AttrType::anchor:
                break;
        }
    };

    accessor.onAttrUpdated = [this](AttrType attr, NotificationType notification)
    {
        switch(attr)
        {
            case AttrType::identifier:
            {
                if(onIdentifierUpdated != nullptr)
                {
                    onIdentifierUpdated(notification);
                }
            }
            break;
            case AttrType::name:
            {
                if(onNameUpdated != nullptr)
                {
                    onIdentifierUpdated(notification);
                }
            }
            break;
            case AttrType::key:
            {
                if(mAccessor.getAttr<AttrType::description>() == Plugin::Description{})
                {
                    auto const getDescription = [&]()
                    {
                        try
                        {
                            auto const sampleRate = mAudioFormatReader != nullptr ? mAudioFormatReader->sampleRate : 48000.0;
                            return PluginList::Scanner::loadDescription(mAccessor.getAttr<AttrType::key>(), sampleRate);
                        }
                        catch(...)
                        {
                        }
                        return Plugin::Description();
                    };

                    auto const description = getDescription();
                    if(description != Plugin::Description{})
                    {
                        mAccessor.setAttr<AttrType::description>(description, notification);
                        auto newState = description.defaultState;
                        if(newState.blockSize == 0_z)
                        {
                            newState.blockSize = 64_z;
                        }
                        if(newState.stepSize == 0_z)
                        {
                            newState.stepSize = newState.blockSize;
                        }
                        mAccessor.setAttr<AttrType::state>(newState, notification);
                    }
                }
                else
                {
                    runAnalysis(notification);
                }
            }
            break;
            case AttrType::input:
            {
                if(onInputUpdated != nullptr)
                {
                    onInputUpdated(notification);
                }
                runAnalysis(notification);
            }
            break;
            case AttrType::state:
            {
                runAnalysis(notification);
            }
            break;
            case AttrType::description:
            {
                sanitizeZooms(notification);
            }
            break;
            case AttrType::file:
            {
                auto const file = getEffectiveFile();
                FileWatcher::clearAllFiles();
                saveBackup();
                FileWatcher::addFile(file);
                auto const& results = mAccessor.getAttr<AttrType::results>();
                auto const access = results.getReadAccess();
                if(!static_cast<bool>(access) || !results.isEmpty())
                {
                    break;
                }
                if(file != juce::File{})
                {
                    if(!file.existsAsFile())
                    {
                        fileHasBeenRemoved(file);
                    }
                    else
                    {
                        runLoading();
                    }
                }
                else
                {
                    runAnalysis(notification);
                }
            }
            break;
            case AttrType::results:
            {
                sanitizeZooms(notification);
                auto const& results = mAccessor.getAttr<AttrType::results>();
                auto const access = results.getReadAccess();
                if(static_cast<bool>(access))
                {
                    auto const numChannels = results.getNumChannels();
                    if(numChannels.has_value())
                    {
                        auto channelsLayout = mAccessor.getAttr<AttrType::channelsLayout>();
                        channelsLayout.resize(*numChannels, true);
                        mAccessor.setAttr<AttrType::channelsLayout>(channelsLayout, NotificationType::synchronous);
                    }
                }

                if(onResultsUpdated != nullptr)
                {
                    onResultsUpdated(notification);
                }
            }
            break;
            case AttrType::edit:
            case AttrType::graphics:
            case AttrType::height:
                break;
            case AttrType::colours:
            {
                if(!mAccessor.getAttr<AttrType::hasPluginColourMap>())
                {
                    runRendering();
                }
            }
            break;
            case AttrType::channelsLayout:
            {
                if(onChannelsLayoutUpdated != nullptr)
                {
                    onChannelsLayoutUpdated(notification);
                }
            }
            break;
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            {
                auto sharedZoomAcsr = mAccessor.getAttr<AttrType::zoomAcsr>();
                if(mSharedZoomAccessor.has_value())
                {
                    mSharedZoomAccessor.value().get().removeListener(mSharedZoomListener);
                }
                mSharedZoomAccessor = sharedZoomAcsr;
                if(mSharedZoomAccessor.has_value() && mAccessor.getAttr<AttrType::zoomLink>())
                {
                    mSharedZoomAccessor.value().get().addListener(mSharedZoomListener, NotificationType::synchronous);
                }
            }
            break;
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::focused:
            case AttrType::grid:
            case AttrType::font:
            case AttrType::unit:
            case AttrType::showInGroup:
            case AttrType::zoomValueMode:
            case AttrType::extraThresholds:
            case AttrType::hasPluginColourMap:
                break;
        }
    };

    auto const updateLinkedZoom = [this](NotificationType notification)
    {
        if(!mAccessor.getAttr<AttrType::zoomLink>() || !mSharedZoomAccessor.has_value())
        {
            return;
        }
        std::unique_lock<std::mutex> lock(mSharedZoomMutex, std::try_to_lock);
        if(!lock.owns_lock())
        {
            return;
        }
        auto& sharedZoom = mSharedZoomAccessor.value().get();
        auto const updateZoom = [&](Zoom::Accessor& zoomAcsr)
        {
            if(zoomAcsr.getAttr<Zoom::AttrType::globalRange>().isEmpty() && zoomAcsr.getAttr<Zoom::AttrType::visibleRange>().isEmpty())
            {
                return;
            }
            auto const range = Zoom::Tools::getScaledVisibleRange(zoomAcsr, sharedZoom.getAttr<Zoom::AttrType::globalRange>());
            sharedZoom.setAttr<Zoom::AttrType::visibleRange>(range, notification);
        };
        auto const frameType = Tools::getFrameType(mAccessor);
        if(frameType.has_value())
        {
            switch(frameType.value())
            {
                case Track::FrameType::label:
                    break;
                case Track::FrameType::value:
                    updateZoom(mAccessor.getAcsr<AcsrType::valueZoom>());
                    break;
                case Track::FrameType::vector:
                    updateZoom(mAccessor.getAcsr<AcsrType::binZoom>());
                    break;
            }
        }
    };

    auto& valueZoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
    valueZoomAcsr.onAttrUpdated = [=, this](Zoom::AttrType attr, NotificationType notification)
    {
        switch(attr)
        {
            case Zoom::AttrType::globalRange:
            {
                switch(mAccessor.getAttr<AttrType::zoomValueMode>())
                {
                    case ZoomValueMode::plugin:
                    {
                        auto globalRange = Tools::getValueRange(mAccessor.getAttr<AttrType::description>());
                        if(!globalRange.has_value() || globalRange->isEmpty())
                        {
                            break;
                        }
                        setGlobalValueRange(globalRange.value(), NotificationType::synchronous);
                        break;
                    }
                    case ZoomValueMode::results:
                    {
                        auto globalRange = Tools::getResultRange(mAccessor);
                        if(!globalRange.has_value() || globalRange->isEmpty())
                        {
                            break;
                        }
                        setGlobalValueRange(globalRange.value(), NotificationType::synchronous);
                        break;
                    }
                    case ZoomValueMode::undefined:
                    case ZoomValueMode::custom:
                        break;
                }
                if(Tools::getFrameType(mAccessor) == Track::FrameType::value)
                {
                    updateLinkedZoom(notification);
                }
            }
            break;
            case Zoom::AttrType::visibleRange:
            {
                if(Tools::getFrameType(mAccessor) == Track::FrameType::value)
                {
                    updateLinkedZoom(notification);
                }
                else if(Tools::getFrameType(mAccessor) == Track::FrameType::vector)
                {
                    runRendering();
                }
            }
            break;
            case Zoom::AttrType::minimumLength:
            case Zoom::AttrType::anchor:
                break;
        }
    };
    valueZoomAcsr.onAttrUpdated(Zoom::AttrType::globalRange, NotificationType::synchronous);

    auto& binZoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
    binZoomAcsr.onAttrUpdated = [=, this](Zoom::AttrType attr, NotificationType notification)
    {
        switch(attr)
        {
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::visibleRange:
            {
                if(Tools::getFrameType(mAccessor) == Track::FrameType::vector)
                {
                    updateLinkedZoom(notification);
                }
            }
            break;
            case Zoom::AttrType::minimumLength:
            case Zoom::AttrType::anchor:
                break;
        }
    };

    mProcessor.onAnalysisEnded = [&](Results const& results)
    {
        mAccessor.setAttr<AttrType::results>(results, NotificationType::synchronous);
        runRendering();
    };

    mProcessor.onAnalysisAborted = [&]()
    {
        mAccessor.setAttr<AttrType::results>(Results{}, NotificationType::synchronous);
        mAccessor.setAttr<AttrType::graphics>(Images{}, NotificationType::synchronous);
    };

    mLoader.onLoadingSucceeded = [&](Results const& results)
    {
        auto const fileInfo = mAccessor.getAttr<AttrType::file>();
        if(!fileInfo.extra.is_null())
        {
            mAccessor.fromJson(fileInfo.extra, NotificationType::synchronous);
        }
        mAccessor.setAttr<AttrType::results>(results, NotificationType::synchronous);
        runRendering();
    };

    mLoader.onLoadingFailed = [&](juce::String const& message)
    {
        mAccessor.setAttr<AttrType::warnings>(WarningType::file, NotificationType::synchronous);
        askToReloadFile(message);
    };

    mLoader.onLoadingAborted = [&]()
    {
        mAccessor.setAttr<AttrType::results>(Results{}, NotificationType::synchronous);
        mAccessor.setAttr<AttrType::graphics>(Images{}, NotificationType::synchronous);
    };

    mGraphics.onRenderingUpdated = [&](Images images)
    {
        mAccessor.setAttr<AttrType::graphics>(images, NotificationType::synchronous);
    };

    mGraphics.onRenderingEnded = [&](Images images)
    {
        stopTimer();
        timerCallback();
        mAccessor.setAttr<AttrType::graphics>(images, NotificationType::synchronous);
    };

    mGraphics.onRenderingAborted = [&]()
    {
        stopTimer();
        timerCallback();
    };

    mHierarchyListener.onResultsChanged = [this]([[maybe_unused]] HierarchyManager const& manager, juce::String const& identifier)
    {
        if(mAccessor.getAttr<AttrType::input>() == identifier)
        {
            runAnalysis(NotificationType::synchronous);
        }
    };
    mHierarchyListener.onHierarchyChanged = [this]([[maybe_unused]] HierarchyManager const& manager)
    {
        auto const input = mAccessor.getAttr<AttrType::input>();
        if(input.isNotEmpty() && !mHierarchyManager.hasAccessor(input))
        {
            auto const results = mAccessor.getAttr<AttrType::results>();
            auto const access = results.getReadAccess();
            if(!static_cast<bool>(access) || !results.isEmpty())
            {
                runAnalysis(NotificationType::synchronous);
            }
        }
    };
    mHierarchyManager.addHierarchyListener(mHierarchyListener, NotificationType::synchronous);

    runAnalysis(NotificationType::synchronous);
}

Track::Director::~Director()
{
    mHierarchyManager.removeHierarchyListener(mHierarchyListener);
    setPluginTable(nullptr, nullptr);
    setLoaderSelector(nullptr, nullptr);
    if(mSharedZoomAccessor.has_value())
    {
        mSharedZoomAccessor.value().get().removeListener(mSharedZoomListener);
    }

    mGraphics.onRenderingAborted = nullptr;
    mGraphics.onRenderingEnded = nullptr;
    mProcessor.onAnalysisAborted = nullptr;
    mProcessor.onAnalysisEnded = nullptr;
    mAccessor.onAttrUpdated = nullptr;
    mAccessor.onAccessorInserted = nullptr;
    auto& valueZoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
    valueZoomAcsr.onAttrUpdated = nullptr;
    auto& binZoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
    binZoomAcsr.onAttrUpdated = nullptr;
}

Track::Accessor& Track::Director::getAccessor()
{
    return mAccessor;
}

juce::UndoManager& Track::Director::getUndoManager()
{
    return mUndoManager;
}

Track::HierarchyManager& Track::Director::getHierarchyManager()
{
    return mHierarchyManager;
}

std::function<Track::Accessor&()> Track::Director::getSafeAccessorFn()
{
    MiscWeakAssert(mSafeAccessorRetriever.getAccessorFn != nullptr);
    return mSafeAccessorRetriever.getAccessorFn;
}

std::function<Zoom::Accessor&()> Track::Director::getSafeTimeZoomAccessorFn()
{
    MiscWeakAssert(mSafeAccessorRetriever.getTimeZoomAccessorFn != nullptr);
    return mSafeAccessorRetriever.getTimeZoomAccessorFn;
}

std::function<Transport::Accessor&()> Track::Director::getSafeTransportZoomAccessorFn()
{
    MiscWeakAssert(mSafeAccessorRetriever.getTransportAccessorFn != nullptr);
    return mSafeAccessorRetriever.getTransportAccessorFn;
}

void Track::Director::setSafeAccessorRetriever(SafeAccessorRetriever const& sav)
{
    mSafeAccessorRetriever = sav;
}

bool Track::Director::hasChanged() const
{
    return !mAccessor.isEquivalentTo(mSavedState) || !mAccessor.getAcsr<AcsrType::valueZoom>().isEquivalentTo(mSavedState.getAcsr<AcsrType::valueZoom>()) || !mAccessor.getAcsr<AcsrType::binZoom>().isEquivalentTo(mSavedState.getAcsr<AcsrType::binZoom>());
}

void Track::Director::startAction()
{
    MiscDebug("Track", "Director::startAction");
    anlWeakAssert(mIsPerformingAction == false);
    if(!std::exchange(mIsPerformingAction, true))
    {
        mSavedState.copyFrom(mAccessor, NotificationType::synchronous);
    }
}

void Track::Director::endAction(ActionState state, juce::String const& name)
{
    MiscDebug("Track", "Director::endAction");
    anlWeakAssert(mIsPerformingAction == true);
    if(!hasChanged())
    {
        mIsPerformingAction = false;
        return;
    }

    class Action
    : public juce::UndoableAction
    {
    public:
        Action(std::function<Accessor&()> fn, Accessor const& undoAcsr)
        : mGetSafeAccessor(fn)
        {
            MiscWeakAssert(mGetSafeAccessor != nullptr);
            if(mGetSafeAccessor != nullptr)
            {
                mRedoAccessor.copyFrom(mGetSafeAccessor(), NotificationType::synchronous);
                mUndoAccessor.copyFrom(undoAcsr, NotificationType::synchronous);
            }
        }

        ~Action() override = default;

        bool perform() override
        {
            MiscWeakAssert(mGetSafeAccessor != nullptr);
            if(mGetSafeAccessor == nullptr)
            {
                return true;
            }
            auto& accessor = mGetSafeAccessor();
            accessor.copyFrom(mRedoAccessor, NotificationType::synchronous);
            return true;
        }

        bool undo() override
        {
            MiscWeakAssert(mGetSafeAccessor != nullptr);
            if(mGetSafeAccessor == nullptr)
            {
                return true;
            }
            auto& accessor = mGetSafeAccessor();
            // Clears results to force reload the file
            if(accessor.getAttr<AttrType::file>().isEmpty() && !mUndoAccessor.getAttr<AttrType::file>().isEmpty())
            {
                accessor.setAttr<AttrType::results>(Results{}, NotificationType::synchronous);
                accessor.setAttr<AttrType::warnings>(WarningType::none, NotificationType::synchronous);
            }
            accessor.copyFrom(mUndoAccessor, NotificationType::synchronous);
            return true;
        }

    private:
        std::function<Accessor&()> mGetSafeAccessor;
        Accessor mRedoAccessor;
        Accessor mUndoAccessor;
    };

    auto action = std::make_unique<Action>(getSafeAccessorFn(), mSavedState);
    if(action != nullptr)
    {
        switch(state)
        {
            case ActionState::abort:
            {
                action->undo();
            }
            break;
            case ActionState::newTransaction:
            {
                mUndoManager.beginNewTransaction(name);
                mUndoManager.perform(action.release());
            }
            break;
            case ActionState::continueTransaction:
            {
                mUndoManager.perform(action.release());
            }
            break;
        }
    }
    mSavedState.copyFrom(mAccessor, NotificationType::synchronous);
    mIsPerformingAction = false;
}

void Track::Director::setGlobalValueRange(juce::Range<double> const& range, NotificationType const notification)
{
    auto const pluginRange = Tools::getValueRange(mAccessor.getAttr<AttrType::description>());
    auto const resultsRange = Tools::getResultRange(mAccessor);
    if(range.isEmpty())
    {
        mAccessor.setAttr<AttrType::zoomValueMode>(ZoomValueMode::undefined, notification);
    }
    else if(pluginRange.has_value() && range == pluginRange.value())
    {
        mAccessor.setAttr<AttrType::zoomValueMode>(ZoomValueMode::plugin, notification);
    }
    else if(resultsRange.has_value() && range == resultsRange.value())
    {
        mAccessor.setAttr<AttrType::zoomValueMode>(ZoomValueMode::results, notification);
    }
    else
    {
        mAccessor.setAttr<AttrType::zoomValueMode>(ZoomValueMode::custom, notification);
    }
    mAccessor.getAcsr<AcsrType::valueZoom>().setAttr<Zoom::AttrType::globalRange>(range, notification);
}

void Track::Director::setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, NotificationType const notification)
{
    anlStrongAssert(audioFormatReader == nullptr || audioFormatReader != mAudioFormatReader);
    if(audioFormatReader == mAudioFormatReader)
    {
        return;
    }

    std::swap(mAudioFormatReader, audioFormatReader);
    runAnalysis(notification);
}

void Track::Director::setBackupDirectory(juce::File const& directory)
{
    if(mBackupDirectory != directory)
    {
        deleteBackup();
        mBackupDirectory = directory;
        saveBackup();
    }
}

void Track::Director::setAlertCatcher(AlertWindow::Catcher* catcher)
{
    mAlertCatcher = catcher;
}

void Track::Director::setPluginTable(PluginList::Table* table, std::function<void(bool)> showHideFn)
{
    mPluginTable = table;
    mPluginTableShowHideFn = showHideFn;
}

void Track::Director::setLoaderSelector(Loader::ArgumentSelector* selector, std::function<void(bool)> showHideFn)
{
    mLoaderSelector = selector;
    mLoaderSelectorShowHideFn = showHideFn;
}

void Track::Director::runAnalysis(NotificationType const notification)
{
    if(!mAccessor.getAttr<AttrType::file>().isEmpty())
    {
        return;
    }
    mGraphics.stopRendering();
    if(mAudioFormatReader == nullptr)
    {
        mProcessor.stopAnalysis();
        return;
    }

    auto const& key = mAccessor.getAttr<AttrType::key>();
    if(key.identifier.empty() || key.feature.empty())
    {
        return;
    }

    auto const input = mAccessor.getAttr<AttrType::input>();
    if(input.isNotEmpty() && !mHierarchyManager.hasAccessor(input))
    {
        return;
    }

    auto inputResults = input.isNotEmpty() ? mHierarchyManager.getAccessor(input).getAttr<AttrType::results>() : Results{};
    try
    {
        auto const description = mProcessor.runAnalysis(mAccessor, *mAudioFormatReader.get(), inputResults);
        if(description.has_value())
        {
            anlDebug("Track", "analysis launched");
            mAccessor.setAttr<AttrType::description>(description.value(), notification);
            mAccessor.setAttr<AttrType::warnings>(WarningType::none, notification);
            startTimer(50);
            timerCallback();
        }
    }
    catch(Plugin::LoadingError& e)
    {
        mAccessor.setAttr<AttrType::warnings>(WarningType::library, notification);
        askToReloadPlugin(e.what());
    }
    catch(Plugin::ParametersError& e)
    {
        mAccessor.setAttr<AttrType::warnings>(WarningType::state, notification);
        askToRestoreState(e.what());
    }
    catch(std::exception& e)
    {
        mAccessor.setAttr<AttrType::warnings>(WarningType::plugin, notification);
        warmAboutPlugin(e.what());
    }
    catch(...)
    {
        mAccessor.setAttr<AttrType::warnings>(WarningType::plugin, notification);
        warmAboutPlugin("The plugin cannot be allocated!");
    }
}

void Track::Director::runLoading()
{
    mGraphics.stopRendering();
    auto file = mAccessor.getAttr<AttrType::file>();
    file.file = getEffectiveFile();
    if(file.file != juce::File{})
    {
        mAccessor.setAttr<AttrType::warnings>(WarningType::none, NotificationType::synchronous);
        startTimer(50);
        timerCallback();
        mLoader.loadAnalysis(file);
    }
}

void Track::Director::runRendering()
{
    auto const createPlugin = [this]() -> std::unique_ptr<Ive::PluginWrapper>
    {
        auto const& key = mAccessor.getAttr<AttrType::key>();
        if(key.identifier.empty() || key.feature.empty())
        {
            return nullptr;
        }
        if(mAudioFormatReader == nullptr)
        {
            return nullptr;
        }
        auto const sampleRate = mAudioFormatReader != nullptr ? mAudioFormatReader->sampleRate : 48000.0;
        try
        {
            return Plugin::Tools::createPluginWrapper(key, sampleRate);
        }
        catch(...)
        {
        }
        return nullptr;
    };

    auto const result = mGraphics.runRendering(mAccessor, createPlugin());
    mAccessor.setAttr<AttrType::hasPluginColourMap>(result, NotificationType::synchronous);
    startTimer(50);
    timerCallback();
}

void Track::Director::sanitizeZooms(NotificationType const notification)
{
    auto const getVisibleRange = [this](Zoom::Accessor& zoomAcsr, Zoom::Range const& newRange, bool useZoomLink)
    {
        auto const hasZoomLink = mSharedZoomAccessor.has_value() && mAccessor.getAttr<AttrType::zoomLink>();
        auto const& zoomAcsrRef = hasZoomLink && useZoomLink ? mSharedZoomAccessor.value().get() : zoomAcsr;
        auto const scaledRange = Zoom::Tools::getScaledVisibleRange(zoomAcsrRef, newRange);
        return scaledRange.isEmpty() ? newRange : scaledRange;
    };

    auto const applyValueZoom = [&](bool useZoomLink)
    {
        auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
        auto const& output = mAccessor.getAttr<AttrType::description>().output;
        auto const minimumLength = output.isQuantized ? static_cast<double>(output.quantizeStep) : Zoom::epsilon();
        auto const globalRange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
        auto const pluginRange = Tools::getValueRange(mAccessor.getAttr<AttrType::description>());
        auto const zoomValueMode = mAccessor.getAttr<AttrType::zoomValueMode>();

        auto const shouldUpdate = [&](ZoomValueMode const expectedMode, std::optional<Zoom::Range> const range)
        {
            return (zoomValueMode == ZoomValueMode::undefined || zoomValueMode == expectedMode || globalRange.getLength() <= Zoom::epsilon()) && range.has_value() && !range->isEmpty();
        };

        if(shouldUpdate(ZoomValueMode::plugin, pluginRange))
        {
            auto const visibleRange = getVisibleRange(zoomAcsr, pluginRange.value(), useZoomLink);
            setGlobalValueRange(pluginRange.value(), NotificationType::synchronous);
            zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(minimumLength, notification);
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange, notification);
        }
        else
        {
            auto const& results = mAccessor.getAttr<AttrType::results>();
            auto const access = results.getReadAccess();
            auto const resultsRange = static_cast<bool>(access) ? results.getValueRange() : decltype(results.getValueRange()){};
            if(shouldUpdate(ZoomValueMode::results, resultsRange))
            {
                auto const visibleRange = getVisibleRange(zoomAcsr, resultsRange.value(), useZoomLink);
                setGlobalValueRange(resultsRange.value(), NotificationType::synchronous);
                zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(minimumLength, notification);
                zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange, notification);
            }
        }
    };

    auto const applyBinZoom = [&](bool useZoomLink)
    {
        auto& zoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
        auto const pluginRange = Tools::getBinRange(mAccessor.getAttr<AttrType::description>());
        auto const& results = mAccessor.getAttr<AttrType::results>();
        auto const access = results.getReadAccess();
        auto const numBins = static_cast<bool>(access) ? results.getNumBins() : decltype(results.getNumBins()){};
        if(pluginRange.has_value() && !pluginRange.value().isEmpty())
        {
            auto const visibleRange = getVisibleRange(zoomAcsr, pluginRange.value(), useZoomLink);
            zoomAcsr.setAttr<Zoom::AttrType::globalRange>(pluginRange.value(), notification);
            zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(1.0, notification);
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange, notification);
        }
        else if(numBins.has_value() && numBins.value() > 0_z)
        {
            Zoom::Range const binRange{0.0, static_cast<double>(numBins.value())};
            auto const visibleRange = getVisibleRange(zoomAcsr, binRange, useZoomLink);
            zoomAcsr.setAttr<Zoom::AttrType::globalRange>(binRange, notification);
            zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(1.0, notification);
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange, notification);
        }
    };

    auto const frameType = Tools::getFrameType(mAccessor);
    if(frameType.has_value())
    {
        switch(frameType.value())
        {
            case FrameType::label:
                break;
            case FrameType::value:
            {
                applyValueZoom(true);
            }
            break;
            case FrameType::vector:
            {
                applyValueZoom(false);
                applyBinZoom(true);
            }
            break;
        }
    }
}

void Track::Director::fileHasBeenRemoved(juce::File const& file)
{
    mAccessor.setAttr<AttrType::warnings>(WarningType::file, NotificationType::synchronous);
    if(mAlertCatcher != nullptr)
    {
        mAlertCatcher->postMessage(AlertWindow::MessageType::warning, juce::translate("Results file has been removed!"), juce::translate("The results file FLNAME of the track TKNAME has been removed.").replace("FLNAME", file.getFullPathName()).replace("TKNAME", mAccessor.getAttr<AttrType::name>()));
        return;
    }

    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::WarningIcon)
                             .withTitle(juce::translate("Results file has been removed!"))
                             .withMessage(juce::translate("The results file FLNAME of the track TKNAME has been removed. Would you like to restore it?").replace("FLNAME", file.getFullPathName()).replace("TKNAME", mAccessor.getAttr<AttrType::name>()))
                             .withButton(juce::translate("Restore"))
                             .withButton(juce::translate("Cancel"));

    juce::WeakReference<Director> weakReference(this);
    juce::AlertWindow::showAsync(options, [=, this](int result)
                                 {
                                     if(weakReference.get() == nullptr || result != 1)
                                     {
                                         return;
                                     }
                                     askForFile();
                                 });
}

void Track::Director::fileHasBeenRestored(juce::File const& file)
{
    if(mAlertCatcher != nullptr)
    {
        return;
    }

    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::WarningIcon)
                             .withTitle(juce::translate("Results file has been restored!"))
                             .withMessage(juce::translate("The results file FLNAME of the track TKNAME has been restored. Would you like to reload it?").replace("FLNAME", file.getFullPathName()).replace("TKNAME", mAccessor.getAttr<AttrType::name>()))
                             .withButton(juce::translate("Reload"))
                             .withButton(juce::translate("Cancel"));

    juce::WeakReference<Director> weakReference(this);
    juce::AlertWindow::showAsync(options, [=, this](int result)
                                 {
                                     if(weakReference.get() == nullptr || result != 1)
                                     {
                                         return;
                                     }
                                     runLoading();
                                 });
}

void Track::Director::fileHasBeenModified(juce::File const& file)
{
    if(mAlertCatcher != nullptr)
    {
        return;
    }

    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::WarningIcon)
                             .withTitle(juce::translate("Results file has been modified!"))
                             .withMessage(juce::translate("The results file FLNAME of the track TKNAME has been modified. Would you like to reload it?").replace("FLNAME", file.getFullPathName()).replace("TKNAME", mAccessor.getAttr<AttrType::name>()))
                             .withButton(juce::translate("Reload"))
                             .withButton(juce::translate("Cancel"));

    juce::WeakReference<Director> weakReference(this);
    juce::AlertWindow::showAsync(options, [=, this](int result)
                                 {
                                     if(weakReference.get() == nullptr || result != 1)
                                     {
                                         return;
                                     }
                                     runLoading();
                                 });
}

void Track::Director::timerCallback()
{
    auto const processorRunning = mProcessor.isRunning() || mLoader.isRunning();
    auto const processorProgress = mProcessor.isRunning() ? mProcessor.getAdvancement() : (mLoader.isRunning() ? mLoader.getAdvancement() : 0.0);
    auto const graphicsRunning = mGraphics.isRunning();
    auto const graphicsProgress = mGraphics.getAdvancement();
    if(!processorRunning && !graphicsRunning)
    {
        stopTimer();
    }
    mAccessor.setAttr<AttrType::processing>(std::make_tuple(processorRunning, processorProgress, graphicsRunning, graphicsProgress), NotificationType::synchronous);
}

void Track::Director::warmAboutPlugin(juce::String const& reason)
{
    auto const name = mAccessor.getAttr<AttrType::name>();
    auto const key = mAccessor.getAttr<AttrType::key>();
    auto const errorMessage = juce::translate("The plugin [KEYID - KEYFEATURE] of the track TKNAME cannot be loaded: REASON.")
                                  .replace("KEYID", key.identifier)
                                  .replace("KEYFEATURE", key.feature)
                                  .replace("TKNAME", name)
                                  .replace("REASON", reason);

    if(mAlertCatcher != nullptr)
    {
        mAlertCatcher->postMessage(AlertWindow::MessageType::warning, juce::translate("Plugin cannot be loaded!"), errorMessage);
        return;
    }

    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::WarningIcon)
                             .withTitle(juce::translate("Plugin cannot be loaded!"))
                             .withMessage(errorMessage)
                             .withButton(juce::translate("Ok"));
    juce::AlertWindow::showAsync(options, nullptr);
}

void Track::Director::askToReloadPlugin(juce::String const& reason)
{
    auto const name = mAccessor.getAttr<AttrType::name>();
    auto const key = mAccessor.getAttr<AttrType::key>();
    auto const errorMessage = juce::translate("The plugin [KEYID - KEYFEATURE] of the track TKNAME cannot be loaded: REASON.")
                                  .replace("KEYID", key.identifier)
                                  .replace("KEYFEATURE", key.feature)
                                  .replace("TKNAME", name)
                                  .replace("REASON", reason);

    if(mAlertCatcher != nullptr)
    {
        mAlertCatcher->postMessage(AlertWindow::MessageType::warning, juce::translate("Plugin cannot be loaded!"), errorMessage);
        return;
    }

    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::WarningIcon)
                             .withTitle(juce::translate("Plugin cannot be loaded!"))
                             .withMessage(errorMessage + " " + juce::translate("Would you like to select another plugin?"))
                             .withButton(juce::translate("Replace plugin"))
                             .withButton(juce::translate("Ignore"));

    juce::WeakReference<Director> weakReference(this);
    juce::AlertWindow::showAsync(options, [=, this](int windowResult)
                                 {
                                     if(weakReference.get() == nullptr || mPluginTable == nullptr || mPluginTableShowHideFn == nullptr)
                                     {
                                         return;
                                     }
                                     if(windowResult == 1)
                                     {
                                         mPluginTable->setMultipleSelectionEnabled(false);
                                         mPluginTable->onAddPlugins = [=, this](std::set<Plugin::Key> keys)
                                         {
                                             if(weakReference.get() == nullptr)
                                             {
                                                 return;
                                             }
                                             MiscWeakAssert(keys.size() == 1_z);
                                             if(keys.size() < 1_z)
                                             {
                                                 return;
                                             }
                                             if(mPluginTableShowHideFn != nullptr)
                                             {
                                                 mPluginTableShowHideFn(false);
                                             }
                                             startAction();
                                             mAccessor.setAttr<AttrType::key>(*keys.cbegin(), NotificationType::synchronous);
                                             endAction(ActionState::newTransaction, juce::translate("Change track's plugin"));
                                         };
                                         mPluginTableShowHideFn(true);
                                     }
                                 });
}

void Track::Director::askToRestoreState(juce::String const& reason)
{
    auto const name = mAccessor.getAttr<AttrType::name>();
    auto const key = mAccessor.getAttr<AttrType::key>();
    auto const errorMessage = juce::translate("The plugin [KEYID - KEYFEATURE] of the track TKNAME cannot be loaded: REASON.")
                                  .replace("KEYID", key.identifier)
                                  .replace("KEYFEATURE", key.feature)
                                  .replace("TKNAME", name)
                                  .replace("REASON", reason);

    if(mAlertCatcher != nullptr)
    {
        mAlertCatcher->postMessage(AlertWindow::MessageType::warning, juce::translate("Plugin cannot be loaded!"), errorMessage);
        return;
    }

    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::WarningIcon)
                             .withTitle(juce::translate("Plugin cannot be loaded!"))
                             .withMessage(errorMessage + " " + juce::translate("Would you like to reset to the default state?"))
                             .withButton(juce::translate("Restore default state"))
                             .withButton(juce::translate("Ignore"));

    juce::WeakReference<Director> weakReference(this);
    juce::AlertWindow::showAsync(options, [=, this](int windowResult)
                                 {
                                     if(weakReference.get() == nullptr)
                                     {
                                         return;
                                     }
                                     if(windowResult == 1)
                                     {
                                         startAction();
                                         mAccessor.setAttr<AttrType::state>(mAccessor.getAttr<AttrType::description>().defaultState, NotificationType::synchronous);
                                         endAction(ActionState::newTransaction, juce::translate("Reset track to default state"));
                                     }
                                 });
}

void Track::Director::askToRemoveFile()
{
    auto const file = mAccessor.getAttr<AttrType::file>();
    if(file.isEmpty())
    {
        return;
    }

    if(mAlertCatcher != nullptr)
    {
        return;
    }

    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::QuestionIcon)
                             .withTitle(juce::translate("Remove result files?"))
                             .withMessage(juce::translate("The analysis results are loaded from a file, have been modified, or consolidated to a file. Do you want to remove the file (it will restart the analysis if a plugin is selected)?"))
                             .withButton(juce::translate("Remove file"))
                             .withButton(juce::translate("Cancel"));
    juce::WeakReference<Director> weakReference(this);
    juce::AlertWindow::showAsync(options, [=, this](auto result)
                                 {
                                     if(result == 0 || weakReference.get() == nullptr)
                                     {
                                         return;
                                     }
                                     startAction();
                                     mAccessor.setAttr<AttrType::warnings>(WarningType::none, NotificationType::synchronous);
                                     mAccessor.setAttr<AttrType::results>(Results{}, NotificationType::synchronous);
                                     mAccessor.setAttr<AttrType::file>(FileInfo{}, NotificationType::synchronous);
                                     endAction(ActionState::newTransaction, juce::translate("Remove track's results file"));
                                 });
}

void Track::Director::askToReloadFile(juce::String const& reason)
{
    auto const file = mAccessor.getAttr<AttrType::file>().file;
    auto const name = mAccessor.getAttr<AttrType::name>();
    auto const errorMessage = juce::translate("The file FLNAME of the track TKNAME cannot be loaded: REASON.")
                                  .replace("FLNAME", file.getFullPathName())
                                  .replace("TKNAME", name)
                                  .replace("REASON", reason);

    if(mAlertCatcher != nullptr)
    {
        mAlertCatcher->postMessage(AlertWindow::MessageType::warning, "Loading file failed!", errorMessage);
        return;
    }

    auto const& key = mAccessor.getAttr<AttrType::key>();
    auto getMessageBoxOptions = [&]()
    {
        if(!key.identifier.empty() && !key.feature.empty())
        {
            return juce::MessageBoxOptions()
                .withIconType(juce::AlertWindow::WarningIcon)
                .withTitle(juce::translate("Loading file failed!"))
                .withMessage(errorMessage + " " + juce::translate("Would you like to replace the file, run the analysis or to ignore the file?"))
                .withButton(juce::translate("Replace"))
                .withButton(juce::translate("Analyze"))
                .withButton(juce::translate("Ignore"));
        }
        return juce::MessageBoxOptions()
            .withIconType(juce::AlertWindow::WarningIcon)
            .withTitle(juce::translate("Loading file failed!"))
            .withMessage(errorMessage + " " + juce::translate("Would you like to replace the file or to ignore the file?"))
            .withButton(juce::translate("Replace"))
            .withButton(juce::translate("Ignore"));
    };

    juce::WeakReference<Director> safePointer(this);
    juce::AlertWindow::showAsync(getMessageBoxOptions(), [=, this](int result)
                                 {
                                     if(safePointer.get() == nullptr)
                                     {
                                         return;
                                     }
                                     if(result == 1)
                                     {
                                         askForFile();
                                     }
                                     else if(result == 2)
                                     {
                                         startAction();
                                         mAccessor.setAttr<AttrType::warnings>(WarningType::none, NotificationType::synchronous);
                                         mAccessor.setAttr<AttrType::results>(Results{}, NotificationType::synchronous);
                                         mAccessor.setAttr<AttrType::file>(FileInfo{}, NotificationType::synchronous);
                                         endAction(ActionState::newTransaction, juce::translate("Remove track's results file"));
                                     }
                                 });
}

void Track::Director::askForFile()
{
    if(mLoaderSelector == nullptr)
    {
        return;
    }
    auto const file = mAccessor.getAttr<AttrType::file>().file;
    mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Load track's results file..."), file, Loader::getWildCardForAllFormats());
    if(mFileChooser == nullptr)
    {
        return;
    }
    using Flags = juce::FileBrowserComponent::FileChooserFlags;
    juce::WeakReference<Director> safePointer(this);
    mFileChooser->launchAsync(Flags::openMode | Flags::canSelectFiles, [=, this](juce::FileChooser const& fileChooser)
                              {
                                  if(safePointer.get() == nullptr || mLoaderSelector == nullptr)
                                  {
                                      return;
                                  }
                                  auto const results = fileChooser.getResults();
                                  if(results.isEmpty())
                                  {
                                      return;
                                  }
                                  auto const sampleRate = mAudioFormatReader != nullptr ? mAudioFormatReader->sampleRate : 44100.0;
                                  if(mLoaderSelector->setFile(results.getFirst(), sampleRate, [=, this](Track::FileInfo fileInfo)
                                                              {
                                                                  if(safePointer.get() == nullptr)
                                                                  {
                                                                      return;
                                                                  }
                                                                  if(mLoaderSelectorShowHideFn != nullptr)
                                                                  {
                                                                      mLoaderSelectorShowHideFn(false);
                                                                  }
                                                                  auto isPerformingAction = mIsPerformingAction;
                                                                  if(!isPerformingAction)
                                                                  {
                                                                      startAction();
                                                                  }
                                                                  mAccessor.setAttr<AttrType::results>(Results{}, NotificationType::synchronous);
                                                                  mAccessor.setAttr<AttrType::warnings>(WarningType::none, NotificationType::synchronous);
                                                                  mAccessor.setAttr<AttrType::file>(fileInfo, NotificationType::synchronous);
                                                                  if(!isPerformingAction)
                                                                  {
                                                                      endAction(ActionState::newTransaction, juce::translate("Change results file"));
                                                                  }
                                                              }))
                                  {
                                      if(mLoaderSelectorShowHideFn != nullptr)
                                      {
                                          mLoaderSelectorShowHideFn(true);
                                      }
                                  }
                              });
}

void Track::Director::askToResolveWarnings()
{
    switch(mAccessor.getAttr<AttrType::warnings>())
    {
        case WarningType::none:
            break;
        case WarningType::library:
        {
            askToReloadPlugin("Library cannot be loaded!");
        }
        break;
        case WarningType::state:
        {
            askToRestoreState("Parameters are invalid!");
        }
        break;
        case WarningType::file:
        {
            askToReloadFile("The file cannot be parsed!");
        }
        break;
        case WarningType::plugin:
        {
            warmAboutPlugin("The plugin cannot be allocated!");
        }
        break;
    }
}

void Track::Director::deleteBackup() const
{
    if(mBackupDirectory.exists())
    {
        Exporter::getConsolidatedFile(mAccessor, mBackupDirectory).deleteFile();
    }
}

void Track::Director::saveBackup() const
{
    if(mBackupDirectory != juce::File{} && mAccessor.getAttr<AttrType::file>().commit.isNotEmpty())
    {
        mBackupDirectory.createDirectory();
        Exporter::consolidateInDirectory(mAccessor, mBackupDirectory);
        MiscDebug("Track::Director", "saved");
    }
}

juce::File Track::Director::getEffectiveFile() const
{
    if(mAccessor.getAttr<AttrType::file>().commit.isNotEmpty() && mBackupDirectory.exists())
    {
        return Exporter::getConsolidatedFile(mAccessor, mBackupDirectory);
    }
    return mAccessor.getAttr<AttrType::file>().file;
}

bool Track::Director::isFileModified() const
{
    if(mAccessor.getAttr<AttrType::file>().commit.isEmpty())
    {
        return false;
    }
    return getEffectiveFile().getFileName() != mAccessor.getAttr<AttrType::file>().file.getFileName();
}

ANALYSE_FILE_END
