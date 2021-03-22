#include "AnlAnalyzerDirector.h"
#include "AnlAnalyzerProcessor.h"
#include "AnlAnalyzerResults.h"

ANALYSE_FILE_BEGIN

Analyzer::Director::Director(Accessor& accessor, PluginList::Scanner& pluginListScanner, std::unique_ptr<juce::AudioFormatReader> audioFormatReader)
: mAccessor(accessor)
, mPluginListScanner(pluginListScanner)
, mAudioFormatReaderManager(std::move(audioFormatReader))
{
    accessor.onAttrUpdated = [&](AttrType anlAttr, NotificationType notification)
    {
        switch(anlAttr)
        {
            case AttrType::key:
            {
                try
                {
                    auto const& description = mPluginListScanner.getDescription(accessor.getAttr<AttrType::key>());
                    mAccessor.setAttr<AttrType::name>(description.name, NotificationType::synchronous);
                    mAccessor.setAttr<AttrType::description>(description, NotificationType::synchronous);
                    mAccessor.setAttr<AttrType::state>(description.defaultState, NotificationType::synchronous);
                }
                catch(...)
                {
                    
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
                updateZoomAccessors(notification);
            }
                break;
            case AttrType::results:
            {
                if(!mAccessor.canContinueToReadResults())
                {
                    mAccessor.releaseResultsWrittingAccess();
                }
                updateZoomAccessors(notification);
            }
                break;
            case AttrType::name:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::colours:
            case AttrType::propertyState:
            case AttrType::time:
            case AttrType::warnings:
            case AttrType::processing:
                break;
        }
    };
    
    mProcessor.onAnalysisEnded = [&](std::vector<Plugin::Result> const& results)
    {
        mAccessor.setAttr<AttrType::processing>(false, NotificationType::synchronous);
        anlDebug("Analyzer", "analysis succeded");
        auto const now = juce::Time::getCurrentTime();
        mAccessor.acquireResultsWrittingAccess();
        mAccessor.setAttr<AttrType::results>(results, NotificationType::synchronous);
        if(!mAccessor.canContinueToReadResults())
        {
            mAccessor.releaseResultsWrittingAccess();
        }
        anlDebug("Analyzer", "analysis stored in " + (juce::Time::getCurrentTime() - now).getDescription());
    };
    
    mProcessor.onAnalysisAborted = [&]()
    {
        mAccessor.setAttr<AttrType::processing>(false, NotificationType::synchronous);
        mAccessor.acquireResultsWrittingAccess();
        mAccessor.setAttr<AttrType::results>(std::vector<Plugin::Result>{}, NotificationType::synchronous);
        if(!mAccessor.canContinueToReadResults())
        {
            mAccessor.releaseResultsWrittingAccess();
        }
    };
    
    runAnalysis(NotificationType::synchronous);
}

Analyzer::Director::~Director()
{
    mProcessor.onAnalysisAborted = nullptr;
    mProcessor.onAnalysisEnded = nullptr;
    mAccessor.onAttrUpdated = nullptr;
    mAccessor.onAccessorInserted = nullptr;
    mAccessor.onAccessorErased = nullptr;
}

void Analyzer::Director::setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, NotificationType const notification)
{
    anlStrongAssert(audioFormatReader == nullptr || audioFormatReader != mAudioFormatReaderManager);
    if(audioFormatReader == mAudioFormatReaderManager)
    {
        return;
    }
    
    std::swap(mAudioFormatReaderManager, audioFormatReader);
    runAnalysis(notification);
}

void Analyzer::Director::runAnalysis(NotificationType const notification)
{
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
        mAccessor.setAttr<AttrType::processing>(false, notification);
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
            anlDebug("Analyzer", "analysis launched");
            mAccessor.setAttr<AttrType::processing>(true, notification);
            mAccessor.setAttr<AttrType::description>(pluginDescription, notification);
        }
            break;
        case WarningType::state:
        {
            mAccessor.setAttr<AttrType::processing>(false, notification);
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
            mAccessor.setAttr<AttrType::processing>(false, notification);
            showWarningWindow(message);
        }
            break;
    }
}

void Analyzer::Director::updateZoomAccessors(NotificationType const notification)
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
        else if(!results.empty())
        {
            valueZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Results::getValueRange(results), notification);
        }
        valueZoomAcsr.setAttr<Zoom::AttrType::minimumLength>(output.isQuantized ? static_cast<double>(output.quantizeStep) : Zoom::epsilon(), notification);
        valueZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(valueZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), notification);
    }
    
    auto& binZoomAcsr = mAccessor.getAccessor<AcsrType::binZoom>(0);
    if(output.hasFixedBinCount)
    {
        binZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{0.0, static_cast<double>(output.binCount)}, notification);
    }
    else if(!results.empty())
    {
        binZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Results::getBinRange(results), notification);
    }
    if(binZoomAcsr.getAttr<Zoom::AttrType::visibleRange>() == Zoom::Range{0.0, 0.0})
    {
        binZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(binZoomAcsr.getAttr<Zoom::AttrType::globalRange>(), notification);
    }
    binZoomAcsr.setAttr<Zoom::AttrType::minimumLength>(1.0, notification);
}

ANALYSE_FILE_END
