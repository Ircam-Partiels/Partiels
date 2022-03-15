#include "AnlTrackDirector.h"
#include "AnlTrackExporter.h"
#include "AnlTrackProcessor.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Director::Director(Accessor& accessor, juce::UndoManager& undoManager, std::unique_ptr<juce::AudioFormatReader> audioFormatReader)
: mAccessor(accessor)
, mUndoManager(undoManager)
, mAudioFormatReader(std::move(audioFormatReader))
{
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
            case AttrType::key:
            {
                if(mAccessor.getAttr<AttrType::description>() == Plugin::Description{})
                {
                    auto getDescription = [&]()
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
                        mAccessor.setAttr<AttrType::description>(description, NotificationType::synchronous);
                        auto newState = description.defaultState;
                        if(newState.blockSize == 0_z)
                        {
                            newState.blockSize = 64_z;
                        }
                        if(newState.stepSize == 0_z)
                        {
                            newState.stepSize = newState.blockSize;
                        }
                        mAccessor.setAttr<AttrType::state>(newState, NotificationType::synchronous);
                    }
                }
                else if(mAccessor.getAttr<AttrType::file>().file == juce::File{})
                {
                    runAnalysis(notification);
                }
            }
            break;
            case AttrType::state:
            {
                if(mAccessor.getAttr<AttrType::file>().file == juce::File{})
                {
                    runAnalysis(notification);
                }
            }
            break;
            case AttrType::description:
            {
                sanitizeZooms(notification);
            }
            break;
            case AttrType::file:
            {
                clearFilesToWatch();
                auto const file = mAccessor.getAttr<AttrType::file>().file;
                if(file != juce::File{})
                {
                    addFileToWatch(file);
                }
                auto const results = mAccessor.getAttr<AttrType::results>();
                if(!results.isEmpty())
                {
                    break;
                }
                if(file != juce::File{})
                {
                    runLoading();
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
                auto const results = mAccessor.getAttr<AttrType::results>();
                auto getNumChannels = [&]() -> std::optional<size_t>
                {
                    if(auto markers = results.getMarkers())
                    {
                        return markers->size();
                    }
                    if(auto points = results.getPoints())
                    {
                        return points->size();
                    }
                    if(auto columns = results.getColumns())
                    {
                        return columns->size();
                    }
                    return {};
                };

                auto const numChannels = getNumChannels();
                if(numChannels.has_value())
                {
                    auto channelsLayout = mAccessor.getAttr<AttrType::channelsLayout>();
                    channelsLayout.resize(*numChannels, true);
                    mAccessor.setAttr<AttrType::channelsLayout>(channelsLayout, NotificationType::synchronous);
                }

                if(onResultsUpdated != nullptr)
                {
                    onResultsUpdated(notification);
                }
            }
            break;
            case AttrType::graphics:
            case AttrType::name:
            case AttrType::height:
                break;
            case AttrType::colours:
            {
                runRendering();
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
                if(mSharedZoomAccessor.has_value() && (!mAccessor.getAttr<AttrType::zoomLink>() || !sharedZoomAcsr.has_value() || std::addressof(*mSharedZoomAccessor) != std::addressof(*sharedZoomAcsr)))
                {
                    mSharedZoomAccessor->get().removeListener(mSharedZoomListener);
                }
                mSharedZoomAccessor = sharedZoomAcsr;
                if(mSharedZoomAccessor.has_value() && mAccessor.getAttr<AttrType::zoomLink>())
                {
                    mSharedZoomAccessor->get().addListener(mSharedZoomListener, NotificationType::synchronous);
                }
            }
            break;
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::focused:
            case AttrType::grid:
                break;
        }
    };

    auto updateLinkedZoom = [this](NotificationType notification)
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

        auto& sharedZoom = mSharedZoomAccessor->get();
        switch(Tools::getDisplayType(mAccessor))
        {
            case Tools::DisplayType::markers:
                break;
            case Tools::DisplayType::points:
            {
                auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
                auto const range = Zoom::Tools::getScaledVisibleRange(zoomAcsr, sharedZoom.getAttr<Zoom::AttrType::globalRange>());
                sharedZoom.setAttr<Zoom::AttrType::visibleRange>(range, notification);
            }
            break;
            case Tools::DisplayType::columns:
            {
                auto& zoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
                auto const range = Zoom::Tools::getScaledVisibleRange(zoomAcsr, sharedZoom.getAttr<Zoom::AttrType::globalRange>());
                sharedZoom.setAttr<Zoom::AttrType::visibleRange>(range, notification);
            }
            break;
        }
    };

    auto& valueZoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
    valueZoomAcsr.onAttrUpdated = [=, this](Zoom::AttrType attr, NotificationType notification)
    {
        switch(attr)
        {
            case Zoom::AttrType::globalRange:
            {
                auto const globalRange = mAccessor.getAcsr<AcsrType::valueZoom>().getAttr<Zoom::AttrType::globalRange>();
                auto const pluginRange = Tools::getValueRange(mAccessor.getAttr<AttrType::description>());
                auto const resultsRange = Tools::getValueRange(mAccessor.getAttr<AttrType::results>());
                if(globalRange.isEmpty())
                {
                    mValueRangeMode = ValueRangeMode::undefined;
                }
                else if(pluginRange.has_value() && !globalRange.isEmpty() && globalRange == *pluginRange)
                {
                    mValueRangeMode = ValueRangeMode::plugin;
                }
                else if(resultsRange.has_value() && !globalRange.isEmpty() && globalRange == *resultsRange)
                {
                    mValueRangeMode = ValueRangeMode::results;
                }
                else
                {
                    mValueRangeMode = ValueRangeMode::custom;
                }

                if(Tools::getDisplayType(mAccessor) == Tools::DisplayType::points)
                {
                    updateLinkedZoom(notification);
                }
            }
            break;
            case Zoom::AttrType::visibleRange:
            {
                runRendering();
                if(Tools::getDisplayType(mAccessor) == Tools::DisplayType::points)
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
    valueZoomAcsr.onAttrUpdated(Zoom::AttrType::globalRange, NotificationType::synchronous);

    auto& binZoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
    binZoomAcsr.onAttrUpdated = [=, this](Zoom::AttrType attr, NotificationType notification)
    {
        switch(attr)
        {
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::visibleRange:
            {
                if(Tools::getDisplayType(mAccessor) == Tools::DisplayType::columns)
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

    mSharedZoomListener.onAttrChanged = [=, this](Zoom::Accessor const& sharedZoomAcsr, Zoom::AttrType attribute)
    {
        std::unique_lock<std::mutex> lock(mSharedZoomMutex, std::try_to_lock);
        if(!lock.owns_lock())
        {
            return;
        }
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::visibleRange:
            {
                switch(Tools::getDisplayType(mAccessor))
                {
                    case Tools::DisplayType::markers:
                        break;
                    case Tools::DisplayType::points:
                    {
                        auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
                        auto const range = Zoom::Tools::getScaledVisibleRange(sharedZoomAcsr, zoomAcsr.getAttr<Zoom::AttrType::globalRange>());
                        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
                    }
                    break;
                    case Tools::DisplayType::columns:
                    {
                        auto& zoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
                        auto const range = Zoom::Tools::getScaledVisibleRange(sharedZoomAcsr, zoomAcsr.getAttr<Zoom::AttrType::globalRange>());
                        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
                    }
                    break;
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
        mAccessor.setAttr<AttrType::results>(Results(), NotificationType::synchronous);
        mAccessor.setAttr<AttrType::graphics>(Images{}, NotificationType::synchronous);
    };

    mLoader.onLoadingSucceeded = [&](Results const& results)
    {
        auto const fileInfo = mAccessor.getAttr<AttrType::file>();
        Loader::ArgumentSelector::apply(mAccessor, fileInfo, NotificationType::synchronous);
        mAccessor.setAttr<AttrType::results>(results, NotificationType::synchronous);
        runRendering();
    };

    mLoader.onLoadingFailed = [&](juce::String const& message)
    {
        if(mAlertCatcher != nullptr)
        {
            mAlertCatcher->postMessage(AlertWindow::MessageType::warning, "Loading results failed!", message);
            return;
        }

        auto const trackFile = mAccessor.getAttr<AttrType::file>();
        mAccessor.setAttr<AttrType::warnings>(WarningType::file, NotificationType::synchronous);
        auto const& key = mAccessor.getAttr<AttrType::key>();
        auto getMessageBoxOptions = [&]()
        {
            if(!key.identifier.empty() && !key.feature.empty())
            {
                return juce::MessageBoxOptions()
                    .withIconType(juce::AlertWindow::WarningIcon)
                    .withTitle(juce::translate("Loading results file failed!"))
                    .withMessage(juce::translate("The loading of results from file FILENAME failed: ERRORMESSAGE. Would you like to select another file, to run the plugin or to ignore the missing file?").replace("FILENAME", trackFile.file.getFullPathName()).replace("ERRORMESSAGE", message))
                    .withButton(juce::translate("Select File"))
                    .withButton(juce::translate("Run Plugin"))
                    .withButton(juce::translate("Ignore File"));
            }
            return juce::MessageBoxOptions()
                .withIconType(juce::AlertWindow::WarningIcon)
                .withTitle(juce::translate("Loading results file failed!"))
                .withMessage(juce::translate("The loading of results from file FILENAME failed: ERRORMESSAGE. Would you like to select another file, to run the plugin or to ignore the missing file?").replace("FILENAME", trackFile.file.getFullPathName()).replace("ERRORMESSAGE", message))
                .withButton(juce::translate("Select File"))
                .withButton(juce::translate("Ignore File"));
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
                                             askForResultsFile(juce::translate("Load analysis results..."), trackFile.file, NotificationType::synchronous);
                                         }
                                         else if(result == 2)
                                         {
                                             startAction();
                                             mAccessor.setAttr<AttrType::results>(Results{}, NotificationType::synchronous);
                                             mAccessor.setAttr<AttrType::file>(FileInfo{}, NotificationType::synchronous);
                                             endAction(ActionState::newTransaction, juce::translate("Remove results file"));
                                         }
                                     });
    };

    mLoader.onLoadingAborted = [&]()
    {
        mAccessor.setAttr<AttrType::results>(Results(), NotificationType::synchronous);
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

    if(mAccessor.getAttr<AttrType::file>().file == juce::File{})
    {
        runAnalysis(NotificationType::synchronous);
    }
}

Track::Director::~Director()
{
    if(mSharedZoomAccessor.has_value())
    {
        mSharedZoomAccessor->get().removeListener(mSharedZoomListener);
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
        Action(Accessor& accessor, Accessor const& undoAcsr)
        : mAccessor(accessor)
        {
            mRedoAccessor.copyFrom(mAccessor, NotificationType::synchronous);
            mUndoAccessor.copyFrom(undoAcsr, NotificationType::synchronous);
        }

        ~Action() override = default;

        bool perform() override
        {
            mAccessor.copyFrom(mRedoAccessor, NotificationType::synchronous);
            return true;
        }

        bool undo() override
        {
            mAccessor.copyFrom(mUndoAccessor, NotificationType::synchronous);
            return true;
        }

    private:
        Accessor& mAccessor;
        Accessor mRedoAccessor;
        Accessor mUndoAccessor;
    };

    auto action = std::make_unique<Action>(mAccessor, mSavedState);
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

void Track::Director::setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, NotificationType const notification)
{
    anlStrongAssert(audioFormatReader == nullptr || audioFormatReader != mAudioFormatReader);
    if(audioFormatReader == mAudioFormatReader)
    {
        return;
    }

    std::swap(mAudioFormatReader, audioFormatReader);
    if(mAccessor.getAttr<AttrType::file>().file == juce::File{})
    {
        runAnalysis(notification);
    }
}

void Track::Director::setAlertCatcher(AlertWindow::Catcher* catcher)
{
    mAlertCatcher = catcher;
}

void Track::Director::runAnalysis(NotificationType const notification)
{
    mGraphics.stopRendering();
    if(mAudioFormatReader == nullptr)
    {
        mProcessor.stopAnalysis();
        return;
    }
    auto showWarningWindow = [&](juce::String const& reason)
    {
        auto const title = juce::translate("Plugin cannot be loaded!");
        auto const message = juce::String("The plugin \"KEYID - KEYFEATURE\" of the track \"TRACKNAME\" cannot be loaded due to: REASON.").replace("KEYID", mAccessor.getAttr<AttrType::key>().identifier).replace("KEYFEATURE", mAccessor.getAttr<AttrType::key>().feature).replace("TRACKNAME", mAccessor.getAttr<AttrType::name>()).replace("REASON", reason);
        if(mAlertCatcher != nullptr)
        {
            mAlertCatcher->postMessage(AlertWindow::MessageType::warning, title, message);
        }
        else
        {
            auto const options = juce::MessageBoxOptions()
                                     .withIconType(juce::AlertWindow::WarningIcon)
                                     .withTitle(title)
                                     .withMessage(message)
                                     .withButton(juce::translate("Ok"));
            juce::AlertWindow::showAsync(options, nullptr);
        }
    };

    auto const key = mAccessor.getAttr<AttrType::key>();
    if(key.identifier.empty() || key.feature.empty())
    {
        return;
    }
    auto const sampleRate = mAudioFormatReader->sampleRate;
    auto const description = PluginList::Scanner::loadDescription(key, sampleRate);
    if(description != Plugin::Description{})
    {
        mAccessor.setAttr<AttrType::description>(description, notification);
    }

    auto const result = mProcessor.runAnalysis(mAccessor, *mAudioFormatReader.get());
    if(!result.has_value())
    {
        return;
    }
    auto const warning = std::get<0>(*result);
    auto const message = std::get<1>(*result);
    mAccessor.setAttr<AttrType::warnings>(warning, notification);
    switch(std::get<0>(*result))
    {
        case WarningType::none:
        {
            anlDebug("Track", "analysis launched");
            startTimer(50);
            timerCallback();
        }
        break;
        case WarningType::state:
        {
            auto currentState = mAccessor.getAttr<AttrType::state>();
            auto const& defaultState = mAccessor.getAttr<AttrType::description>().defaultState;
            if(mAlertCatcher != nullptr || (currentState.blockSize == defaultState.blockSize && currentState.stepSize == defaultState.stepSize))
            {
                showWarningWindow(message);
            }
            else if(AlertWindow::showOkCancel(AlertWindow::MessageType::warning,
                                              "Plugin cannot be loaded",
                                              "The plugin \"KEYID - KEYFEATURE\" of the track \"TRACKNAME\" cannot be initialized because the step size, the block size or the number of channels might not be supported. Would you like to use the plugin default value for the block size and the step size?",
                                              {{"KEYID", mAccessor.getAttr<AttrType::key>().identifier}, {"KEYFEATURE", mAccessor.getAttr<AttrType::key>().feature}, {"TRACKNAME", mAccessor.getAttr<AttrType::name>()}}))

            {
                auto isPerformingAction = mIsPerformingAction;
                if(!isPerformingAction)
                {
                    startAction();
                }
                currentState.blockSize = defaultState.blockSize;
                currentState.stepSize = defaultState.stepSize;
                mAccessor.setAttr<AttrType::state>(defaultState, notification);
                if(!isPerformingAction)
                {
                    endAction(ActionState::newTransaction, juce::translate("Restore track factory properties"));
                }
            }
        }
        break;
        case WarningType::plugin:
        {
            showWarningWindow(message);
        }
        break;
        case WarningType::file:
            break;
    }
}

void Track::Director::runLoading()
{
    mGraphics.stopRendering();
    auto const trackFile = mAccessor.getAttr<AttrType::file>();
    if(trackFile.file != juce::File{})
    {
        mAccessor.setAttr<AttrType::warnings>(WarningType::none, NotificationType::synchronous);
        startTimer(50);
        timerCallback();
        mLoader.loadAnalysis(mAccessor, trackFile);
    }
}

void Track::Director::runRendering()
{
    mGraphics.runRendering(mAccessor);
    startTimer(50);
    timerCallback();
}

void Track::Director::sanitizeZooms(NotificationType const notification)
{
    // Value Zoom
    {
        auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
        auto applyZoom = [&](Zoom::Range const& globalRange)
        {
            auto const& output = mAccessor.getAttr<AttrType::description>().output;
            auto const minimumLength = output.isQuantized ? static_cast<double>(output.quantizeStep) : Zoom::epsilon();
            auto const visibleRange = Zoom::Tools::getScaledVisibleRange(zoomAcsr, globalRange);
            zoomAcsr.setAttr<Zoom::AttrType::globalRange>(globalRange, NotificationType::synchronous);
            zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(minimumLength, notification);
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange, NotificationType::synchronous);
        };

        auto const globalRange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
        auto const pluginRange = Tools::getValueRange(mAccessor.getAttr<AttrType::description>());
        auto const resultsRange = Tools::getValueRange(mAccessor.getAttr<AttrType::results>());
        if((mValueRangeMode == ValueRangeMode::undefined || mValueRangeMode == ValueRangeMode::plugin || globalRange.isEmpty()) && pluginRange.has_value() && !pluginRange->isEmpty())
        {
            applyZoom(*pluginRange);
        }
        else if((mValueRangeMode == ValueRangeMode::undefined || mValueRangeMode == ValueRangeMode::results || globalRange.isEmpty()) && resultsRange.has_value() && !resultsRange->isEmpty())
        {
            applyZoom(*resultsRange);
        }
    }

    // Bin Zoom
    {
        auto& zoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
        auto applyZoom = [&](Zoom::Range const& globalRange)
        {
            auto const visibleRange = Zoom::Tools::getScaledVisibleRange(zoomAcsr, globalRange);
            zoomAcsr.setAttr<Zoom::AttrType::globalRange>(globalRange, notification);
            zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(1.0, notification);
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange, notification);
        };

        auto const pluginRange = Tools::getBinRange(mAccessor.getAttr<AttrType::description>());
        auto const resultsRange = Tools::getBinRange(mAccessor.getAttr<AttrType::results>());
        if(pluginRange.has_value() && !pluginRange->isEmpty())
        {
            applyZoom(*pluginRange);
        }
        else if(resultsRange.has_value() && !resultsRange->isEmpty())
        {
            applyZoom(*resultsRange);
        }
    }
}

bool Track::Director::fileHasBeenRemoved(juce::File const& file)
{
    if(AlertWindow::showOkCancel(AlertWindow::MessageType::warning, "Analysis file has been removed!", "The analysis file FILENAME has been moved or deleted. Would you like to restore it?", {{"FILENAME", file.getFullPathName()}}))
    {
        askForResultsFile(juce::translate("Restore analysis results..."), file, NotificationType::synchronous);
        return false;
    }
    mAccessor.setAttr<AttrType::warnings>(WarningType::file, NotificationType::synchronous);
    return true;
}

bool Track::Director::fileHasBeenRestored(juce::File const& file)
{
    if(AlertWindow::showOkCancel(AlertWindow::MessageType::warning, "Analysis file has been restored!", "The analysis file FILENAME has been restored. Would you like to reload it?", {{"FILENAME", file.getFullPathName()}}))
    {
        runLoading();
        return false;
    }
    return true;
}

bool Track::Director::fileHasBeenModified(juce::File const& file)
{
    if(AlertWindow::showOkCancel(AlertWindow::MessageType::warning, "Analysis file has been modified!", "The analysis file FILENAME has been modified. Would you like to reload it?", {{"FILENAME", file.getFullPathName()}}))
    {
        runLoading();
        return false;
    }
    return true;
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

juce::Result Track::Director::consolidate(juce::File const& file)
{
    auto result = file.createDirectory();
    if(result.failed())
    {
        return result;
    }

    auto const currentFile = mAccessor.getAttr<AttrType::file>().file;
    auto const expectedFile = file.getChildFile(mAccessor.getAttr<AttrType::identifier>() + ".dat");
    if(currentFile == expectedFile)
    {
        return result;
    }
    if(currentFile == juce::File{} || !currentFile.hasFileExtension("dat"))
    {
        std::atomic<bool> shouldAbort = false;
        result = Exporter::toBinary(mAccessor, expectedFile, shouldAbort);
        if(result.failed())
        {
            return result;
        }
    }
    else if(currentFile.existsAsFile())
    {
        if(!currentFile.copyFileTo(expectedFile))
        {
            return juce::Result::fail(juce::translate("Cannot copy to SRCFLNAME to DSTFLNAME").replace("SRCFLNAME", currentFile.getFullPathName()).replace("DSTFLNAME", expectedFile.getFullPathName()));
        }
    }
    mAccessor.setAttr<AttrType::file>(FileInfo{expectedFile}, NotificationType::synchronous);
    return result;
}

void Track::Director::askForResultsFile(juce::String const& message, juce::File const& defaultFile, NotificationType const notification)
{
    mFileChooser = std::make_unique<juce::FileChooser>(message, defaultFile, Loader::getWildCardForAllFormats());
    if(mFileChooser == nullptr)
    {
        return;
    }
    using Flags = juce::FileBrowserComponent::FileChooserFlags;
    juce::WeakReference<Director> safePointer(this);
    mFileChooser->launchAsync(Flags::openMode | Flags::canSelectFiles, [=, this](juce::FileChooser const& fileChooser)
                              {
                                  if(safePointer.get() == nullptr)
                                  {
                                      return;
                                  }
                                  auto const results = fileChooser.getResults();
                                  if(results.isEmpty())
                                  {
                                      return;
                                  }
                                  if(mLoaderArgumentSelector == nullptr)
                                  {
                                      mLoaderArgumentSelector = std::make_unique<Loader::ArgumentSelector>();
                                      anlWeakAssert(mLoaderArgumentSelector != nullptr);
                                      if(mLoaderArgumentSelector == nullptr)
                                      {
                                          return;
                                      }
                                  }
                                  mLoaderArgumentSelector->onLoad = [=, this](Track::FileInfo fileInfo)
                                  {
                                      if(safePointer.get() == nullptr)
                                      {
                                          return;
                                      }
                                      mLoaderArgumentSelector->hide();
                                      auto isPerformingAction = mIsPerformingAction;
                                      if(!isPerformingAction)
                                      {
                                          startAction();
                                      }
                                      mAccessor.setAttr<AttrType::results>(Results{}, notification);
                                      mAccessor.setAttr<AttrType::warnings>(WarningType::none, notification);
                                      mAccessor.setAttr<AttrType::file>(fileInfo, notification);
                                      if(!isPerformingAction)
                                      {
                                          endAction(ActionState::newTransaction, juce::translate("Change results file"));
                                      }
                                  };
                                  mLoaderArgumentSelector->setFile(results.getFirst());
                                  mLoaderArgumentSelector->show();
                              });
}

ANALYSE_FILE_END
