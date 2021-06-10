#include "AnlTrackDirector.h"
#include "../Zoom/AnlZoomTools.h"
#include "AnlTrackProcessor.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Director::Director(Accessor& accessor, juce::UndoManager& undoManager, std::unique_ptr<juce::AudioFormatReader> audioFormatReader)
: mAccessor(accessor)
, mUndoManager(undoManager)
, mAudioFormatReaderManager(std::move(audioFormatReader))
{
    accessor.onAttrUpdated = [this](AttrType attr, NotificationType notification)
    {
        switch(attr)
        {
            case AttrType::key:
            {
                if(mAccessor.getAttr<AttrType::state>() == Plugin::State{})
                {
                    auto const sampleRate = mAudioFormatReaderManager != nullptr ? mAudioFormatReaderManager->sampleRate : 48000.0;
                    auto const description = PluginList::Scanner::loadDescription(mAccessor.getAttr<AttrType::key>(), sampleRate);
                    mAccessor.setAttr<AttrType::description>(description, NotificationType::synchronous);
                    mAccessor.setAttr<AttrType::state>(description.defaultState, NotificationType::synchronous);
                }
                else
                {
                    runAnalysis(notification);
                }
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
            case AttrType::results:
            {
                sanitizeZooms(notification);
                auto getNumChannels = [this]() -> std::optional<size_t>
                {
                    auto const results = mAccessor.getAttr<AttrType::results>();
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
            }
            break;
            case AttrType::graphics:
            case AttrType::name:
            case AttrType::identifier:
            case AttrType::height:
                break;
            case AttrType::colours:
            {
                runRendering();
            }
            break;
            case AttrType::channelsLayout:
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

    runAnalysis(NotificationType::synchronous);
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

void Track::Director::startAction()
{
    anlStrongAssert(mIsPerformingAction == false);
    mSavedState.copyFrom(mAccessor, NotificationType::synchronous);
    mIsPerformingAction = true;
}

void Track::Director::endAction(ActionState state, juce::String const& name)
{
    anlStrongAssert(mIsPerformingAction == true);
    if(mAccessor.isEquivalentTo(mSavedState))
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
    mIsPerformingAction = false;
}

void Track::Director::setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, NotificationType const notification)
{
    anlStrongAssert(audioFormatReader == nullptr || audioFormatReader != mAudioFormatReaderManager);
    if(audioFormatReader == mAudioFormatReaderManager)
    {
        return;
    }

    std::swap(mAudioFormatReaderManager, audioFormatReader);
    runAnalysis(notification);
}

void Track::Director::runAnalysis(NotificationType const notification)
{
    mGraphics.stopRendering();
    if(mAudioFormatReaderManager == nullptr)
    {
        mProcessor.stopAnalysis();
        return;
    }
    auto showWarningWindow = [&](juce::String const& reason)
    {
        // clang-format off
        AlertWindow::showMessage(AlertWindow::MessageType::warning,
                            "Plugin cannot be loaded",
                            "The plugin \"KEYID - KEYFEATURE\" of the track \"TRACKNAME\" cannot be loaded due to: REASON.",
                            {
                                {"KEYID", mAccessor.getAttr<AttrType::key>().identifier},
                                {"KEYFEATURE", mAccessor.getAttr<AttrType::key>().feature},
                                {"TRACKNAME", mAccessor.getAttr<AttrType::name>()},
                                {"REASON", reason}
                            });
        // clang-format on
    };

    auto const result = mProcessor.runAnalysis(mAccessor, *mAudioFormatReaderManager.get());
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
            auto const key = mAccessor.getAttr<AttrType::key>();
            auto const sampleRate = mAudioFormatReaderManager->sampleRate;
            auto description = PluginList::Scanner::loadDescription(key, sampleRate);
            description.output = std::get<2>(*result);
            mAccessor.setAttr<AttrType::description>(description, notification);
            startTimer(50);
            timerCallback();
        }
        break;
        case WarningType::state:
        {
            auto currentState = mAccessor.getAttr<AttrType::state>();
            auto const& defaultState = mAccessor.getAttr<AttrType::description>().defaultState;
            if(currentState.blockSize == defaultState.blockSize && currentState.stepSize == defaultState.stepSize)
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

void Track::Director::timerCallback()
{
    auto const processorRunning = mProcessor.isRunning();
    auto const processorProgress = mProcessor.getAdvancement();
    auto const graphicsRunning = mGraphics.isRunning();
    auto const graphicsProgress = mGraphics.getAdvancement();
    if(!processorRunning && !graphicsRunning)
    {
        stopTimer();
    }
    mAccessor.setAttr<AttrType::processing>(std::make_tuple(processorRunning, processorProgress, graphicsRunning, graphicsProgress), NotificationType::synchronous);
}

ANALYSE_FILE_END
