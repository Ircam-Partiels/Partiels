#include "AnlTrackDirector.h"
#include "AnlTrackProcessor.h"
#include "AnlTrackTools.h"
#include "../Zoom/AnlZoomTools.h"

ANALYSE_FILE_BEGIN

Track::Director::Director(Accessor& accessor, std::unique_ptr<juce::AudioFormatReader> audioFormatReader)
: mAccessor(accessor)
, mAudioFormatReaderManager(std::move(audioFormatReader))
{
    auto sanitizeZooms = [this](NotificationType notification)
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
            if(globalRange.isEmpty() && pluginRange.has_value())
            {
                applyZoom(*pluginRange);
            }
            else if(globalRange.isEmpty() && resultsRange.has_value())
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
            if(pluginRange.has_value())
            {
                applyZoom(*pluginRange);
            }
            else if(resultsRange.has_value())
            {
                applyZoom(*resultsRange);
            }
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
            case Tools::DisplayType::segments:
            {
                auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
                auto const range = Zoom::Tools::getScaledVisibleRange(zoomAcsr, sharedZoom.getAttr<Zoom::AttrType::globalRange>());
                sharedZoom.setAttr<Zoom::AttrType::visibleRange>(range, notification);
            }
                break;
            case Tools::DisplayType::grid:
            {
                auto& zoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
                auto const range = Zoom::Tools::getScaledVisibleRange(zoomAcsr, sharedZoom.getAttr<Zoom::AttrType::globalRange>());
                sharedZoom.setAttr<Zoom::AttrType::visibleRange>(range, notification);
            }
                break;
        }
    };
    
    accessor.onAttrUpdated = [=, this](AttrType attr, NotificationType notification)
    {
        switch(attr)
        {
            case AttrType::key:
            {
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
            case AttrType::results:
            {
                sanitizeZooms(notification);
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
            case AttrType::time:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::focused:
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
                if(Tools::getDisplayType(mAccessor) == Tools::DisplayType::segments)
                {
                    updateLinkedZoom(notification);
                }
            }
                break;
            case Zoom::AttrType::minimumLength:
                break;
            case Zoom::AttrType::visibleRange:
            {
                runRendering();
                if(Tools::getDisplayType(mAccessor) == Tools::DisplayType::segments)
                {
                    updateLinkedZoom(notification);
                }
            }
                break;
            case Zoom::AttrType::anchor:
                break;
        }
    };
    
    auto& binZoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
    binZoomAcsr.onAttrUpdated = [=](Zoom::AttrType attr, NotificationType notification)
    {
        switch(attr)
        {
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::visibleRange:
            {
                if(Tools::getDisplayType(mAccessor) == Tools::DisplayType::grid)
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
    
    mSharedZoomListener.onAttrChanged = [=](Zoom::Accessor const& sharedZoomAcsr, Zoom::AttrType attribute)
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
                    case Tools::DisplayType::segments:
                    {
                        auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
                        auto const range = Zoom::Tools::getScaledVisibleRange(sharedZoomAcsr, zoomAcsr.getAttr<Zoom::AttrType::globalRange>());
                        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
                    }
                        break;
                    case Tools::DisplayType::grid:
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
    
    mProcessor.onAnalysisEnded = [&](std::shared_ptr<std::vector<Plugin::Result>> results)
    {
        stopTimer();
        timerCallback();
        mAccessor.setAttr<AttrType::results>(results, NotificationType::synchronous);
        runRendering();
    };
    
    mProcessor.onAnalysisAborted = [&]()
    {
        stopTimer();
        timerCallback();
        mAccessor.setAttr<AttrType::results>(nullptr, NotificationType::synchronous);
        mAccessor.setAttr<AttrType::graphics>(std::vector<juce::Image>{}, NotificationType::synchronous);
    };
    
    mGraphics.onRenderingUpdated = [&](std::vector<juce::Image> images)
    {
        mAccessor.setAttr<AttrType::graphics>(images, NotificationType::synchronous);
    };
    
    mGraphics.onRenderingEnded = [&](std::vector<juce::Image> images)
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
    mAccessor.onAccessorErased = nullptr;
    auto& valueZoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
    valueZoomAcsr.onAttrUpdated = nullptr;
    auto& binZoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
    binZoomAcsr.onAttrUpdated = nullptr;
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
        MessageWindow::show(MessageWindow::MessageType::warning,
                            "Plugin cannot be loaded",
                            "The plugin \"KEYID - KEYFEATURE\" of the track \"TRACKNAME\" cannot be loaded due to: REASON.",
        {
              {"KEYID", mAccessor.getAttr<AttrType::key>().identifier}
            , {"KEYFEATURE", mAccessor.getAttr<AttrType::key>().feature}
            , {"TRACKNAME", mAccessor.getAttr<AttrType::name>()}
            , {"REASON", reason}
        });
    };
    
    auto const result = mProcessor.runAnalysis(mAccessor, *mAudioFormatReaderManager.get());
    if(!result.has_value())
    {
        return;
    }
    auto const warning = std::get<0>(*result);
    auto const message = std::get<1>(*result);
    auto const pluginDescription = std::get<2>(*result);
    auto const pluginState = std::get<3>(*result);
    mAccessor.setAttr<AttrType::warnings>(warning, notification);
    switch(std::get<0>(*result))
    {
        case WarningType::none:
        {
            anlDebug("Track", "analysis launched");
            mAccessor.setAttr<AttrType::description>(pluginDescription, notification);
            startTimer(50);
        }
            break;
        case WarningType::state:
        {
            auto const state = mAccessor.getAttr<AttrType::state>();
            if(state.blockSize == pluginDescription.defaultState.blockSize &&
               state.stepSize == pluginDescription.defaultState.stepSize)
            {
                showWarningWindow(message);
            }
            else if(juce::AlertWindow::showOkCancelBox(juce::AlertWindow::AlertIconType::WarningIcon,
                                                       juce::translate("Plugin cannot be loaded"),
                                                       juce::translate("The plugin \"KEYID - KEYFEATURE\" of the track \"TRACKNAME\" cannot be initialized because the step size or the block size might not be supported. Would you like to use the plugin default value for the block size and the step size?")))
            {
                mAccessor.setAttr<AttrType::state>(pluginState, notification);
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
    startTimer(50);
    mGraphics.runRendering(mAccessor);
}

void Track::Director::timerCallback()
{
    auto const processorRunning = mProcessor.isRunning();
    auto const processorProgress = mProcessor.getAdvancement();
    auto const graphicsRunning = mGraphics.isRunning();
    auto const graphicsProgress = mGraphics.getAdvancement();
    mAccessor.setAttr<AttrType::processing>(std::make_tuple(processorRunning, processorProgress, graphicsRunning, graphicsProgress), NotificationType::synchronous);
}

ANALYSE_FILE_END
