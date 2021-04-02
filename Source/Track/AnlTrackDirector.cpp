#include "AnlTrackDirector.h"
#include "AnlTrackProcessor.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Director::Director(Accessor& accessor, PluginList::Scanner& pluginListScanner, std::unique_ptr<juce::AudioFormatReader> audioFormatReader)
: mAccessor(accessor)
, mPluginListScanner(pluginListScanner)
, mAudioFormatReaderManager(std::move(audioFormatReader))
{
    accessor.onAttrUpdated = [&](AttrType attr, NotificationType notification)
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
                updateZoomAccessors(notification);
            }
                break;
            case AttrType::results:
            {
                updateZoomAccessors(notification);
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
            case AttrType::propertyState:
            case AttrType::time:
            case AttrType::warnings:
            case AttrType::processing:
                break;
        }
    };
    
    auto& valueZoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
    valueZoomAcsr.onAttrUpdated = [&](Zoom::AttrType attr, NotificationType notification)
    {
        juce::ignoreUnused(notification);
        switch(attr)
        {
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::minimumLength:
                break;
            case Zoom::AttrType::visibleRange:
            {
                runRendering();
            }
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
        mAccessor.setAttr<AttrType::graphics>(std::vector<juce::Image>{}, NotificationType::synchronous);
    };
    
    runAnalysis(NotificationType::synchronous);
}

Track::Director::~Director()
{
    mGraphics.onRenderingAborted = nullptr;
    mGraphics.onRenderingEnded = nullptr;
    mProcessor.onAnalysisAborted = nullptr;
    mProcessor.onAnalysisEnded = nullptr;
    mAccessor.onAttrUpdated = nullptr;
    mAccessor.onAccessorInserted = nullptr;
    mAccessor.onAccessorErased = nullptr;
    auto& valueZoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
    valueZoomAcsr.onAttrUpdated = nullptr;
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

void Track::Director::updateZoomAccessors(NotificationType const notification)
{
    auto const& results = mAccessor.getAttr<AttrType::results>();
    auto const& output = mAccessor.getAttr<AttrType::description>().output;
    
    auto& valueZoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
    if(valueZoomAcsr.getAttr<Zoom::AttrType::globalRange>() == Zoom::Range{0.0, 0.0})
    {
        if(output.hasKnownExtents)
        {
            valueZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{static_cast<double>(output.minValue), static_cast<double>(output.maxValue)}, notification);
        }
        else if(results != nullptr && !results->empty())
        {
            valueZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Tools::getValueRange(*results), notification);
        }
        valueZoomAcsr.setAttr<Zoom::AttrType::minimumLength>(output.isQuantized ? static_cast<double>(output.quantizeStep) : Zoom::epsilon(), notification);
        valueZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(valueZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), notification);
    }
    
    auto& binZoomAcsr = mAccessor.getAccessor<AcsrType::binZoom>(0);
    if(output.hasFixedBinCount)
    {
        binZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{0.0, static_cast<double>(output.binCount)}, notification);
    }
    else if(results != nullptr && !results->empty())
    {
        binZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Tools::getBinRange(*results), notification);
    }
    if(binZoomAcsr.getAttr<Zoom::AttrType::visibleRange>() == Zoom::Range{0.0, 0.0})
    {
        binZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(binZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), notification);
    }
    binZoomAcsr.setAttr<Zoom::AttrType::minimumLength>(1.0, notification);
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
