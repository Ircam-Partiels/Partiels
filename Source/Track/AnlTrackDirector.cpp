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
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::visibleRange:
            {
                auto zoomAcsr = Tools::getVerticalZoomAccessor(mAccessor, false);
                if(zoomAcsr.has_value())
                {
                    if(zoomAcsr.value().get().getAttr<Zoom::AttrType::globalRange>().isEmpty())
                    {
                        return;
                    }
                    auto const range = Zoom::Tools::getScaledVisibleRange(sharedZoomAcsr, zoomAcsr.value().get().getAttr<Zoom::AttrType::globalRange>());
                    zoomAcsr.value().get().setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
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
                            return PluginList::Scanner::loadDescription(mAccessor.getAttr<AttrType::key>(), mAccessor.getAttr<AttrType::sampleRate>());
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
            case AttrType::voiceLink:
            {
                // If the track leaves the link while performing the analysis of the linked tracks,
                // the analysis is stopped and must be performed by one of the other linked tracks
                auto const previousVoiceLink = std::exchange(mVoiceLink, mAccessor.getAttr<AttrType::voiceLink>());
                if(previousVoiceLink != mVoiceLink)
                {
                    mIsLinkedAnalysisPending = false;
                    mLinkedAnalysisAdvancementFn = nullptr;
                    if(std::exchange(mIsRunningLinkedAnalysis, false))
                    {
                        mProcessor.stopAnalysis();
                        if(onLinkedAnalysisInterrupted != nullptr)
                        {
                            onLinkedAnalysisInterrupted(previousVoiceLink);
                        }
                    }
                }
                runAnalysis(notification);
            }
            break;
            case AttrType::voiceCounts:
            {
                runAnalysis(notification);
            }
            break;
            case AttrType::inputs:
            {
                notifyLinkedAttrUpdated(attr, notification);
                if(onInputUpdated != nullptr)
                {
                    onInputUpdated(notification);
                }
                runAnalysis(notification);
            }
            break;
            case AttrType::useInputResultsExtraThresholds:
            {
                notifyLinkedAttrUpdated(attr, notification);
                runAnalysis(notification);
            }
            break;
            case AttrType::state:
            {
                auto const& description = mAccessor.getAttr<AttrType::description>();
                auto state = mAccessor.getAttr<AttrType::state>();
                // The values of the alias parameters are not part of the state (they depend on the real parameters)
                for(auto const& parameter : description.parameters)
                {
                    if(parameter.isAlias)
                    {
                        state.parameters.erase(parameter.identifier);
                    }
                }
                if(description.inputDomain == Plugin::InputDomain::TimeDomain && description.defaultState.blockSize != 0_z && state.blockSize != description.defaultState.blockSize)
                {
                    state.blockSize = description.defaultState.blockSize;
                }
                if(state != mAccessor.getAttr<AttrType::state>())
                {
                    // The nested notification synchronizes the linked tracks and runs the analysis
                    mAccessor.setAttr<AttrType::state>(state, notification);
                    break;
                }
                notifyLinkedAttrUpdated(attr, notification);
                runAnalysis(notification);
            }
            break;
            case AttrType::description:
            {
                auto const& description = mAccessor.getAttr<AttrType::description>();
                auto state = mAccessor.getAttr<AttrType::state>();
                // Update the default values of the parameters if they are not already set to avoid overwriting user
                // values when changing the plugin key for example (the values of the alias parameters are not part of
                // the state because they depend on the real parameters)
                for(auto const& parameter : description.parameters)
                {
                    if(parameter.isAlias)
                    {
                        state.parameters.erase(parameter.identifier);
                    }
                    else if(!state.parameters.count(parameter.identifier))
                    {
                        state.parameters[parameter.identifier] = parameter.defaultValue;
                    }
                }
                if(description.inputDomain == Plugin::InputDomain::TimeDomain && description.defaultState.blockSize != 0_z && state.blockSize != description.defaultState.blockSize)
                {
                    state.blockSize = description.defaultState.blockSize;
                }
                mAccessor.setAttr<AttrType::state>(state, notification);
                sanitizeZooms(notification);
                sanitizeExtraOutputs(notification);
            }
            break;
            case AttrType::file:
            {
                // A track that loads its results from a file is no more linked to the voices of an analysis
                if(!mAccessor.getAttr<AttrType::file>().isEmpty())
                {
                    mAccessor.setAttr<AttrType::voiceLink>(juce::String{}, notification);
                }
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
                sanitizeExtraOutputs(notification);
                auto const& results = mAccessor.getAttr<AttrType::results>();
                auto const access = results.getReadAccess();
                if(static_cast<bool>(access))
                {
                    auto const numChannels = results.getNumChannels();
                    if(numChannels.has_value())
                    {
                        auto channelsLayout = mAccessor.getAttr<AttrType::channelsLayout>();
                        channelsLayout.resize(numChannels.value(), true);
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
            case AttrType::graphicsSettings:
            {
                auto const colourMap = mAccessor.getAttr<AttrType::graphicsSettings>().colours.map;
                if(!mLastColourMap.has_value() || mLastColourMap.value() != colourMap)
                {
                    mLastColourMap = colourMap;
                    runRendering();
                }
            }
            break;
            case AttrType::zoomLogScale:
            case AttrType::sampleRate:
            {
                if(mAudioFormatReader != nullptr && std::abs(mAccessor.getAttr<AttrType::sampleRate>() - mAudioFormatReader->sampleRate) > std::numeric_limits<double>::epsilon())
                {
                    mAccessor.setAttr<AttrType::sampleRate>(mAudioFormatReader->sampleRate, NotificationType::synchronous);
                }
                else
                {
                    // The ranges of the bin zoom accessor must be updated if they are expressed
                    // in hertz (the domain of the zoom depends on the log scale and the sample rate)
                    sanitizeZooms(notification);
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
            case AttrType::focused:
            {
                if(onFocusUpdated != nullptr)
                {
                    onFocusUpdated(notification);
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
            case AttrType::grid:
            case AttrType::showInGroup:
            case AttrType::oscIdentifier:
            case AttrType::sendViaOsc:
            case AttrType::zoomValueMode:
            case AttrType::fileDescription:
                break;
            case AttrType::extraThresholds:
            {
                if(onExtraThresholdsUpdated != nullptr)
                {
                    onExtraThresholdsUpdated(notification);
                }
            }
            break;
        }
    };

    auto const updateLinkedZoom = [this](NotificationType notification)
    {
        auto zoomAcsr = Tools::getVerticalZoomAccessor(mAccessor, false);
        if(!mAccessor.getAttr<AttrType::zoomLink>() || !mSharedZoomAccessor.has_value() || !zoomAcsr.has_value())
        {
            return;
        }
        std::unique_lock<std::mutex> lock(mSharedZoomMutex, std::try_to_lock);
        if(!lock.owns_lock())
        {
            return;
        }
        auto& sharedZoom = mSharedZoomAccessor.value().get();
        if(zoomAcsr.value().get().getAttr<Zoom::AttrType::globalRange>().isEmpty() && zoomAcsr.value().get().getAttr<Zoom::AttrType::visibleRange>().isEmpty())
        {
            return;
        }
        auto const range = Zoom::Tools::getScaledVisibleRange(zoomAcsr.value().get(), sharedZoom.getAttr<Zoom::AttrType::globalRange>());
        sharedZoom.setAttr<Zoom::AttrType::visibleRange>(range, notification);
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

    mProcessor.onAnalysisEnded = [&](juce::Result const& result, std::vector<Processor::FeatureResult> const& featureResults)
    {
        auto const isLinkedAnalysis = std::exchange(mIsRunningLinkedAnalysis, false);
        auto const& feature = mAccessor.getAttr<AttrType::key>().feature;
        auto const it = std::find_if(featureResults.cbegin(), featureResults.cend(), [&](auto const& featureResult)
                                     {
                                         return featureResult.feature == feature;
                                     });
        MiscWeakAssert(result.failed() || it != featureResults.cend());
        if(result.wasOk() && it != featureResults.cend())
        {
            applyAnalysisResults(result, it->results, it->description, true);
        }
        else
        {
            applyAnalysisResults(result.wasOk() ? juce::Result::fail(juce::translate("The results of the feature FEATURE cannot be found").replace("FEATURE", feature)) : result, {}, {}, true);
        }
        // The other linked tracks are notified at the end to avoid accessing the track if it has been deleted
        if(isLinkedAnalysis && onLinkedAnalysisEnded != nullptr)
        {
            onLinkedAnalysisEnded(result, featureResults);
        }
    };

    mProcessor.onAnalysisAborted = [&]()
    {
        mAccessor.setAttr<AttrType::results>(Results{}, NotificationType::synchronous);
        mAccessor.setAttr<AttrType::graphics>(Graph{}, NotificationType::synchronous);
    };

    mLoader.onLoadingSucceeded = [&](Results const& results)
    {
        auto const fd = mAccessor.getAttr<AttrType::fileDescription>();
        if(!fd.extra.is_null())
        {
            mAccessor.fromJson(fd.extra, NotificationType::synchronous);
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
        mAccessor.setAttr<AttrType::graphics>(Graph{}, NotificationType::synchronous);
    };

    mGraphics.onRenderingUpdated = [&](Graph graph)
    {
        mAccessor.setAttr<AttrType::graphics>(graph, NotificationType::synchronous);
    };

    mGraphics.onRenderingEnded = [&](Graph graph)
    {
        stopTimer();
        timerCallback();
        mAccessor.setAttr<AttrType::graphics>(graph, NotificationType::synchronous);
    };

    mGraphics.onRenderingAborted = [&]()
    {
        stopTimer();
        timerCallback();
    };

    mHierarchyListener.onResultsChanged = [this]([[maybe_unused]] HierarchyManager const& manager, juce::String const& identifier, HierarchyManager::InputChangeType type)
    {
        auto const inputs = mAccessor.getAttr<AttrType::inputs>();
        auto const useTrack = std::any_of(inputs.cbegin(), inputs.cend(), [&](auto const& pair)
                                          {
                                              return pair.second == identifier;
                                          });
        if(useTrack && (type == HierarchyManager::InputChangeType::results || mAccessor.getAttr<AttrType::useInputResultsExtraThresholds>()))
        {
            runAnalysis(NotificationType::synchronous);
        }
    };
    mHierarchyListener.onHierarchyChanged = [this](HierarchyManager const& manager)
    {
        auto const inputs = mAccessor.getAttr<AttrType::inputs>();
        if(std::any_of(inputs.cbegin(), inputs.cend(), [&](auto const& pair)
                       {
                           return pair.second.isNotEmpty() && !manager.hasAccessor(pair.second);
                       }))
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
    mVoiceLink = mAccessor.getAttr<AttrType::voiceLink>();

    // Make sure the sample rate of the track matches the one of the audio format reader, the
    // analyses, the plugin descriptions and the vertical zoom in hertz depend on it
    if(mAudioFormatReader != nullptr && std::abs(mAccessor.getAttr<AttrType::sampleRate>() - mAudioFormatReader->sampleRate) > std::numeric_limits<double>::epsilon())
    {
        mAccessor.setAttr<AttrType::sampleRate>(mAudioFormatReader->sampleRate, NotificationType::synchronous);
    }

    runAnalysis(NotificationType::synchronous);
    resetSavedState();
}

Track::Director::~Director()
{
    mHierarchyManager.removeHierarchyListener(mHierarchyListener);
    setPluginTable(nullptr, nullptr);
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

void Track::Director::resetSavedState()
{
    mSavedState.copyFrom(mAccessor, NotificationType::synchronous);
}

bool Track::Director::hasChanged() const
{
    return !mAccessor.isEquivalentTo(mSavedState) || !mAccessor.getAcsr<AcsrType::valueZoom>().isEquivalentTo(mSavedState.getAcsr<AcsrType::valueZoom>()) || !mAccessor.getAcsr<AcsrType::binZoom>().isEquivalentTo(mSavedState.getAcsr<AcsrType::binZoom>());
}

bool Track::Director::isPerformingAction() const
{
    return mIsPerformingAction;
}

void Track::Director::startAction()
{
    MiscDebug("Track", "Director::startAction");
    MiscWeakAssert(mIsPerformingAction == false);
    // The action of a linked track is performed by the document because it can modify the other linked tracks
    if(isLinked() && onLinkedActionStarted != nullptr)
    {
        if(!std::exchange(mIsPerformingAction, true))
        {
            mIsPerformingLinkedAction = true;
            resetSavedState();
            onLinkedActionStarted();
        }
        return;
    }
    if(!std::exchange(mIsPerformingAction, true))
    {
#if PARTIELS_DEBUG_DIFF
        MiscWeakAssert(!hasChanged());
        if(hasChanged())
        {
            MiscDebug("Track::Director", mAccessor.getDiff(mSavedState).dump());
        }
#endif
        resetSavedState();
    }
}

void Track::Director::endAction(ActionState state, juce::String const& name)
{
    MiscDebug("Track", "Director::endAction");
    MiscWeakAssert(mIsPerformingAction == true);
    mIsPerformingAction = false;
    if(std::exchange(mIsPerformingLinkedAction, false))
    {
        resetSavedState();
        if(onLinkedActionEnded != nullptr)
        {
            onLinkedActionEnded(state, name);
        }
        return;
    }
    if(!hasChanged())
    {
        return;
    }

    class Action
    : public juce::UndoableAction
    {
    public:
        Action(std::function<Accessor&()> fn, Accessor const& undoAcsr, std::function<void()> resetFn)
        : mGetSafeAccessor(fn)
        , mResetFn(std::move(resetFn))
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
            if(mResetFn != nullptr)
            {
                mResetFn();
            }
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
            if(mResetFn != nullptr)
            {
                mResetFn();
            }
            return true;
        }

    private:
        std::function<Accessor&()> mGetSafeAccessor;
        std::function<void()> mResetFn;
        Accessor mRedoAccessor;
        Accessor mUndoAccessor;
    };

    juce::WeakReference<Director> weakThis(this);
    auto action = std::make_unique<Action>(getSafeAccessorFn(), mSavedState, [weakThis]()
                                           {
                                               if(weakThis != nullptr)
                                               {
                                                   weakThis->resetSavedState();
                                               }
                                           });
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
    MiscStrongAssert(audioFormatReader == nullptr || audioFormatReader != mAudioFormatReader);
    if(audioFormatReader == mAudioFormatReader)
    {
        return;
    }
    std::swap(mAudioFormatReader, audioFormatReader);
    mAccessor.setAttr<AttrType::sampleRate>(mAudioFormatReader != nullptr ? mAudioFormatReader->sampleRate : 48000.0, notification);
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

void Track::Director::setSilentResultsFileManagement(bool state)
{
    mSilentResultsFileManagement = state;
}

void Track::Director::setPreserveFullDurationWhenEditing(bool state)
{
    mIsPreserveFullDurationWhenEditingEnabled = state;
}

bool Track::Director::isPreserveFullDurationWhenEditingEnabled() const
{
    return mIsPreserveFullDurationWhenEditingEnabled;
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

    // The analysis of linked tracks is shared and performed by one of the tracks so the
    // request is forwarded and the track is considered as processing until the results
    if(isLinked() && onLinkedAnalysisRequested != nullptr)
    {
        mProcessor.stopAnalysis();
        mIsRunningLinkedAnalysis = false;
        mLinkedAnalysisAdvancementFn = nullptr;
        mIsLinkedAnalysisPending = true;
        onLinkedAnalysisRequested(notification);
        startTimer(50);
        timerCallback();
        return;
    }

    mIsLinkedAnalysisPending = false;
    mLinkedAnalysisAdvancementFn = nullptr;
    launchAnalysis({}, notification);
}

bool Track::Director::runLinkedAnalysis(std::vector<std::string> const& features)
{
    mIsLinkedAnalysisPending = false;
    mLinkedAnalysisAdvancementFn = nullptr;
    auto const& key = mAccessor.getAttr<AttrType::key>();
    if(!isLinked() || mAudioFormatReader == nullptr || key.identifier.empty() || key.feature.empty())
    {
        timerCallback();
        return false;
    }
    mGraphics.stopRendering();
    auto const launched = launchAnalysis(features, NotificationType::synchronous);
    mIsRunningLinkedAnalysis = launched;
    if(!launched)
    {
        timerCallback();
    }
    return launched;
}

bool Track::Director::launchAnalysis(std::vector<std::string> const& features, NotificationType const notification)
{
    mIsRunningLinkedAnalysis = false;
    auto const inputs = mAccessor.getAttr<AttrType::inputs>();
    if(std::any_of(inputs.cbegin(), inputs.cend(), [this](auto const& pair)
                   {
                       return pair.second.isNotEmpty() && !mHierarchyManager.hasAccessor(pair.second);
                   }))
    {
        return false;
    }

    auto const useExtraThresholds = mAccessor.getAttr<AttrType::useInputResultsExtraThresholds>();
    Processor::InputStates inputStates;
    for(auto const& input : inputs)
    {
        if(input.second.isNotEmpty())
        {
            auto const& inputAcsr = mHierarchyManager.getAccessor(input.second);
            inputStates[input.first].first = inputAcsr.getAttr<AttrType::results>();
            if(useExtraThresholds)
            {
                inputStates[input.first].second = inputAcsr.getAttr<AttrType::extraThresholds>();
            }
        }
    }

    try
    {
        auto const result = mProcessor.runAnalysis(mAccessor, *mAudioFormatReader.get(), inputStates, features);
        if(result)
        {
            MiscDebug("Track", "analysis launched");
            mAccessor.setAttr<AttrType::warnings>(WarningType::none, notification);
            startTimer(50);
            timerCallback();
        }
        return result;
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
    return false;
}

void Track::Director::applyAnalysisResults(juce::Result const& result, Results const& results, Plugin::Description const& description, bool showWarning)
{
    if(result.wasOk())
    {
        // The warning of a previous analysis is cleared (the warning of the tracks that receive
        // the results of a linked analysis is not cleared when the analysis is launched)
        mAccessor.setAttr<AttrType::warnings>(WarningType::none, NotificationType::synchronous);
        mAccessor.setAttr<AttrType::description>(description, NotificationType::synchronous);
        mAccessor.setAttr<AttrType::results>(results, NotificationType::synchronous);
        runRendering();
    }
    else
    {
        mAccessor.setAttr<AttrType::warnings>(WarningType::plugin, NotificationType::synchronous);
        if(showWarning)
        {
            warmAboutPlugin(result.getErrorMessage());
        }
    }
}

bool Track::Director::isLinked() const
{
    return mAccessor.getAttr<AttrType::voiceLink>().isNotEmpty() && mAccessor.getAttr<AttrType::file>().isEmpty();
}

std::optional<float> Track::Director::getAnalysisAdvancement() const
{
    if(!mProcessor.isRunning())
    {
        return {};
    }
    return mProcessor.getAdvancement();
}

void Track::Director::setLinkedAnalysisAdvancementFn(std::function<std::optional<float>()> fn)
{
    mIsLinkedAnalysisPending = false;
    mLinkedAnalysisAdvancementFn = std::move(fn);
    startTimer(50);
    timerCallback();
}

void Track::Director::requestLinkedAnalysis()
{
    if(isLinked())
    {
        runAnalysis(NotificationType::synchronous);
    }
}

void Track::Director::requestVoiceCount(size_t count)
{
    if(isLinked() && onVoiceCountRequested != nullptr)
    {
        onVoiceCountRequested(count);
    }
}

void Track::Director::setLinkedAnalysisResults(juce::Result const& result, Processor::FeatureResult const& featureResult)
{
    mIsLinkedAnalysisPending = false;
    mLinkedAnalysisAdvancementFn = nullptr;
    // The warning is only displayed by the track that performed the analysis
    MiscWeakAssert(result.failed() || featureResult.feature == mAccessor.getAttr<AttrType::key>().feature);
    applyAnalysisResults(result, featureResult.results, featureResult.description, false);
    timerCallback();
}

Ive::PluginWrapper* Track::Director::getAliasParameterPlugin()
{
    // The instance is recreated if the plugin or the sample rate changed
    auto const& key = mAccessor.getAttr<AttrType::key>();
    auto const sampleRate = mAccessor.getAttr<AttrType::sampleRate>();
    if(mAliasParameterPlugin != nullptr && mAliasParameterKey == key && std::abs(mAliasParameterSampleRate - sampleRate) <= std::numeric_limits<double>::epsilon())
    {
        return mAliasParameterPlugin.get();
    }
    mAliasParameterPlugin.reset();
    mAliasParameterKey = key;
    mAliasParameterSampleRate = sampleRate;
    if(key.identifier.empty() || key.feature.empty() || sampleRate <= 0.0)
    {
        return nullptr;
    }
    try
    {
        mAliasParameterPlugin = Plugin::Tools::createPluginWrapper(key, sampleRate);
    }
    catch(std::exception const& e)
    {
        MiscDebug("Track", e.what());
    }
    catch(...)
    {
    }
    return mAliasParameterPlugin.get();
}

Plugin::State Track::Director::getStateWithParameterValue(std::string const& identifier, float value)
{
    return getStateWithParameterValue(mAccessor.getAttr<AttrType::state>(), identifier, value);
}

Plugin::State Track::Director::getStateWithParameterValue(Plugin::State const& state, std::string const& identifier, float value)
{
    auto const& parameters = mAccessor.getAttr<AttrType::description>().parameters;
    auto const it = std::find_if(parameters.cbegin(), parameters.cend(), [&](auto const& parameter)
                                 {
                                     return parameter.identifier == identifier;
                                 });
    if(it == parameters.cend() || !it->isAlias)
    {
        auto newState = state;
        newState.parameters[identifier] = value;
        return newState;
    }
    // If the plugin cannot be loaded, the error is reported by the analysis
    auto* plugin = getAliasParameterPlugin();
    if(plugin == nullptr)
    {
        return state;
    }
    return Plugin::Tools::applyAliasParameterValue(*plugin, state, identifier, value);
}

std::map<std::string, float> Track::Director::getAliasParameterValues()
{
    auto const& parameters = mAccessor.getAttr<AttrType::description>().parameters;
    if(std::none_of(parameters.cbegin(), parameters.cend(), [](auto const& parameter)
                    {
                        return parameter.isAlias;
                    }))
    {
        return {};
    }
    auto* plugin = getAliasParameterPlugin();
    if(plugin == nullptr)
    {
        return {};
    }
    return Plugin::Tools::getAliasParameterValues(*plugin, mAccessor.getAttr<AttrType::state>());
}

void Track::Director::notifyLinkedAttrUpdated(AttrType attribute, NotificationType const notification)
{
    if(onLinkedAttrUpdated != nullptr && isLinked())
    {
        onLinkedAttrUpdated(attribute, notification);
    }
}

void Track::Director::runLoading()
{
    mGraphics.stopRendering();
    auto const file = getEffectiveFile();
    if(file != juce::File{})
    {
        auto const fdResult = Loader::getFileDescription(file, mAccessor.getAttr<AttrType::sampleRate>());
        mAccessor.setAttr<AttrType::fileDescription>(std::get<1_z>(fdResult), NotificationType::synchronous);
        if(std::get<0_z>(fdResult).failed())
        {
            mAccessor.setAttr<AttrType::warnings>(WarningType::file, NotificationType::synchronous);
            askToReloadFile(std::get<0_z>(fdResult).getErrorMessage());
        }
        else
        {
            mAccessor.setAttr<AttrType::warnings>(WarningType::none, NotificationType::synchronous);
            mLoader.loadAnalysis(std::get<1_z>(fdResult));
            startTimer(50);
            timerCallback();
        }
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

        // If the vertical zoom can be displayed and controlled in hertz, the ranges of the
        // zoom accessor are expressed in hertz (or in scaled midi values if the log scale is
        // enabled) instead of bin indices
        auto const nyquist = mAccessor.getAttr<AttrType::sampleRate>() / 2.0;
        if(Tools::hasVerticalZoomInHertz(mAccessor) && nyquist > 0.0)
        {
            Zoom::Range const hertzRange{0.0, nyquist};
            auto const visibleRange = getVisibleRange(zoomAcsr, hertzRange, useZoomLink);
            zoomAcsr.setAttr<Zoom::AttrType::globalRange>(hertzRange, notification);
            zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(Zoom::epsilon(), notification);
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange, notification);
        }
        else if(pluginRange.has_value() && !pluginRange.value().isEmpty())
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

void Track::Director::sanitizeExtraOutputs(NotificationType const notification)
{
    auto const& results = mAccessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return;
    }

    auto description = mAccessor.getAttr<AttrType::description>();
    auto const updateDescription = [&](auto const& data)
    {
        auto& extraOutputs = description.extraOutputs;
        auto const maxSize = std::accumulate(data.cbegin(), data.cend(), extraOutputs.size(), [](auto const dcount, auto const& channelData)
                                             {
                                                 return std::max(std::accumulate(channelData.cbegin(), channelData.cend(), 0_z, [](auto const ccount, auto const& elemData)
                                                                                 {
                                                                                     return std::max(ccount, std::get<3>(elemData).size());
                                                                                 }),
                                                                 dcount);
                                             });
        extraOutputs.resize(maxSize);
        for(auto const& channelData : data)
        {
            for(auto const& elemData : channelData)
            {
                for(auto i = 0_z; i < std::get<3>(elemData).size(); ++i)
                {
                    auto const value = std::get<3>(elemData).at(i);
                    auto& extraOutput = extraOutputs[i];
                    if(std::exchange(extraOutput.hasKnownExtents, true))
                    {
                        extraOutput.minValue = std::min(extraOutput.minValue, value);
                        extraOutput.maxValue = std::max(extraOutput.maxValue, value);
                    }
                    else
                    {
                        extraOutput.minValue = value;
                        extraOutput.maxValue = value;
                    }
                }
            }
        }
    };

    if(auto const markers = results.getMarkers())
    {
        updateDescription(*markers);
    }
    else if(auto points = results.getPoints())
    {
        updateDescription(*points);
    }
    else if(auto columns = results.getColumns())
    {
        updateDescription(*columns);
    }
    mAccessor.setAttr<AttrType::description>(description, notification);
}

void Track::Director::fileHasBeenRemoved(juce::File const& file)
{
    if(mSilentResultsFileManagement)
    {
        return;
    }
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
                             .withButton(juce::translate("Restore"), 1)
                             .withButton(juce::translate("Cancel"), 0);

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
    if(mSilentResultsFileManagement)
    {
        runLoading();
        return;
    }
    if(mAlertCatcher != nullptr)
    {
        return;
    }

    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::WarningIcon)
                             .withTitle(juce::translate("Results file has been restored!"))
                             .withMessage(juce::translate("The results file FLNAME of the track TKNAME has been restored. Would you like to reload it?").replace("FLNAME", file.getFullPathName()).replace("TKNAME", mAccessor.getAttr<AttrType::name>()))
                             .withButton(juce::translate("Reload"), 1)
                             .withButton(juce::translate("Cancel"), 0);

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
    if(mSilentResultsFileManagement)
    {
        runLoading();
        return;
    }
    if(mAlertCatcher != nullptr)
    {
        return;
    }

    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::WarningIcon)
                             .withTitle(juce::translate("Results file has been modified!"))
                             .withMessage(juce::translate("The results file FLNAME of the track TKNAME has been modified. Would you like to reload it?").replace("FLNAME", file.getFullPathName()).replace("TKNAME", mAccessor.getAttr<AttrType::name>()))
                             .withButton(juce::translate("Reload"), 1)
                             .withButton(juce::translate("Cancel"), 0);

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
    // The progress of the analysis of a linked track can be performed by another linked track
    auto const linkedAdvancement = mLinkedAnalysisAdvancementFn != nullptr ? mLinkedAnalysisAdvancementFn() : std::optional<float>{};
    auto const processorRunning = mProcessor.isRunning() || mLoader.isRunning() || mIsLinkedAnalysisPending || linkedAdvancement.has_value();
    auto const processorProgress = mProcessor.isRunning() ? mProcessor.getAdvancement() : (mLoader.isRunning() ? mLoader.getAdvancement() : linkedAdvancement.value_or(0.0f));
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
    MiscDebug("Track::Director", reason);
    auto const name = mAccessor.getAttr<AttrType::name>();
    auto const key = mAccessor.getAttr<AttrType::key>();
    auto const errorMessage = juce::translate("The plugin [KEYID - KEYFEATURE] of the track TKNAME cannot be loaded: ERRSTR.")
                                  .replace("KEYID", key.identifier)
                                  .replace("KEYFEATURE", key.feature)
                                  .replace("TKNAME", name)
                                  .replace("ERRSTR", reason);

    if(mAlertCatcher != nullptr)
    {
        mAlertCatcher->postMessage(AlertWindow::MessageType::warning, juce::translate("Plugin cannot be loaded!"), errorMessage);
        return;
    }

    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::WarningIcon)
                             .withTitle(juce::translate("Plugin cannot be loaded!"))
                             .withMessage(errorMessage)
                             .withButton(juce::translate("Ok"), 0);
    juce::AlertWindow::showAsync(options, nullptr);
}

void Track::Director::askToReloadPlugin(juce::String const& reason)
{
    auto const name = mAccessor.getAttr<AttrType::name>();
    auto const key = mAccessor.getAttr<AttrType::key>();
    auto const errorMessage = juce::translate("The plugin [KEYID - KEYFEATURE] of the track TKNAME cannot be loaded: ERRSTR.")
                                  .replace("KEYID", key.identifier)
                                  .replace("KEYFEATURE", key.feature)
                                  .replace("TKNAME", name)
                                  .replace("ERRSTR", reason);

    if(mAlertCatcher != nullptr)
    {
        mAlertCatcher->postMessage(AlertWindow::MessageType::warning, juce::translate("Plugin cannot be loaded!"), errorMessage);
        return;
    }

    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::WarningIcon)
                             .withTitle(juce::translate("Plugin cannot be loaded!"))
                             .withMessage(errorMessage + " " + juce::translate("Would you like to select another plugin?"))
                             .withButton(juce::translate("Replace plugin"), 1)
                             .withButton(juce::translate("Ignore"), 0);

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
                                         mPluginTable->onAddPlugins = [=, this](std::vector<Plugin::Key> keys)
                                         {
                                             if(weakReference.get() == nullptr)
                                             {
                                                 return;
                                             }
                                             MiscWeakAssert(keys.size() == 1_z);
                                             if(keys.empty())
                                             {
                                                 return;
                                             }
                                             if(mPluginTableShowHideFn != nullptr)
                                             {
                                                 mPluginTableShowHideFn(false);
                                             }
                                             startAction();
                                             // The replacement plugin is analyzed by the track alone because
                                             // the shared analysis of the linked tracks uses the plugin of the link
                                             mAccessor.setAttr<AttrType::voiceCounts>(Plugin::VoiceCounts{}, NotificationType::synchronous);
                                             mAccessor.setAttr<AttrType::key>(keys.front(), NotificationType::synchronous);
                                             mAccessor.setAttr<AttrType::voiceLink>(juce::String{}, NotificationType::synchronous);
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
    auto const errorMessage = juce::translate("The plugin [KEYID - KEYFEATURE] of the track TKNAME cannot be loaded: ERRSTR.")
                                  .replace("KEYID", key.identifier)
                                  .replace("KEYFEATURE", key.feature)
                                  .replace("TKNAME", name)
                                  .replace("ERRSTR", reason);

    if(mAlertCatcher != nullptr)
    {
        mAlertCatcher->postMessage(AlertWindow::MessageType::warning, juce::translate("Plugin cannot be loaded!"), errorMessage);
        return;
    }

    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::WarningIcon)
                             .withTitle(juce::translate("Plugin cannot be loaded!"))
                             .withMessage(errorMessage + " " + juce::translate("Would you like to reset to the default state?"))
                             .withButton(juce::translate("Restore default state"), 1)
                             .withButton(juce::translate("Ignore"), 0);

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
                             .withButton(juce::translate("Remove file"), 1)
                             .withButton(juce::translate("Cancel"), 0);
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
    auto const errorMessage = juce::translate("The file FLNAME of the track TKNAME cannot be loaded: ERRSTR.")
                                  .replace("FLNAME", file.getFullPathName())
                                  .replace("TKNAME", name)
                                  .replace("ERRSTR", reason);

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
                .withButton(juce::translate("Replace"), 1)
                .withButton(juce::translate("Analyze"), 2)
                .withButton(juce::translate("Ignore"), 0);
        }
        return juce::MessageBoxOptions()
            .withIconType(juce::AlertWindow::WarningIcon)
            .withTitle(juce::translate("Loading file failed!"))
            .withMessage(errorMessage + " " + juce::translate("Would you like to replace the file or to ignore the file?"))
            .withButton(juce::translate("Replace"), 1)
            .withButton(juce::translate("Ignore"), 0);
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
    auto const file = mAccessor.getAttr<AttrType::file>().file;
    mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Load track's results file..."), file, Loader::getWildCardForAllFormats());
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
                                  auto isPerformingAction = mIsPerformingAction;
                                  if(!isPerformingAction)
                                  {
                                      startAction();
                                  }
                                  mAccessor.setAttr<AttrType::results>(Results{}, NotificationType::synchronous);
                                  mAccessor.setAttr<AttrType::warnings>(WarningType::none, NotificationType::synchronous);
                                  mAccessor.setAttr<AttrType::file>(FileInfo{results.getFirst()}, NotificationType::synchronous);
                                  if(!isPerformingAction)
                                  {
                                      endAction(ActionState::newTransaction, juce::translate("Change results file"));
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
