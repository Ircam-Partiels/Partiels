#include "AnlAnalyzerDirector.h"
#include "AnlAnalyzerProcessor.h"
#include "AnlAnalyzerPropertyPanel.h"
#include "../../tinycolormap/include/tinycolormap.hpp"

ANALYSE_FILE_BEGIN

Analyzer::Director::Director(Accessor& accessor)
: mAccessor(accessor)
{
    accessor.onAttrUpdated = [&](AttrType anlAttr, NotificationType notification)
    {
        switch (anlAttr)
        {
            case AttrType::key:
            case AttrType::feature:
            case AttrType::parameters:
            {
                runAnalysis(notification);
            }
                break;
            case AttrType::zoomMode:
            {
                updateZoomRange(notification);
            }
                break;
            case AttrType::name:
            case AttrType::colour:
                break;
            case AttrType::colourMap:
            {
                runRendering(notification);
            }
                break;
            case AttrType::results:
            {
                updateFromResults(notification);
                updateZoomRange(notification);
                runRendering(notification);
            }
                break;
        }
    };
    accessor.onAttrUpdated(AttrType::key, NotificationType::synchronous);
}

Analyzer::Director::~Director()
{
    mAccessor.onAttrUpdated = nullptr;
    mAccessor.onAccessorInserted = nullptr;
    mAccessor.onAccessorErased = nullptr;
    
    if(mAnalysisProcess.valid())
    {
        mAnalysisState = ProcessState::aborted;
        mAnalysisProcess.get();
    }
    
    if(mRenderingProcess.valid())
    {
        mRenderingState = ProcessState::aborted;
        mRenderingProcess.get();
    }
}

void Analyzer::Director::setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, NotificationType const notification)
{
    mAudioFormatReaderManager.setInstance(std::shared_ptr<juce::AudioFormatReader>(audioFormatReader.release()));
    runAnalysis(notification);
}

void Analyzer::Director::sanitizeProcessor(NotificationType const notification)
{
    auto processor = mProcessorManager.getInstance();
    std::map<WarningType, juce::String> warnings;
    if(processor == nullptr)
    {
        mAccessor.setAttr<AttrType::warnings>(warnings, notification);
        return;
    }
    mAccessor.setAttr<AttrType::parameters>(processor->getParameters(), notification);
    
    auto const descriptors = processor->getOutputDescriptors();
    anlWeakAssert(!descriptors.empty());
    
    // Ensures that the selected feature is consistent with the plugin
    auto const feature = mAccessor.getAttr<AttrType::feature>();
    if(descriptors.empty())
    {
        warnings[WarningType::feature] = juce::translate("The plugin doesn't have any feature or the feature is not supported!");
        mAccessor.setAttr<AttrType::warnings>(warnings, notification);
        return;
    }
    else if(feature >= descriptors.size())
    {
        warnings[WarningType::feature] = juce::translate("The selected feature FTRINDEX is not supported by the plugin!").replace("FTRINDEX", juce::String(feature));
        mAccessor.setAttr<AttrType::warnings>(warnings, notification);
        return;
    }
    
    auto const descriptor = descriptors[feature];
    
    // Ensures that the type of results returned for this feature is valid
    anlWeakAssert(descriptor.hasFixedBinCount);
    if(!descriptor.hasFixedBinCount)
    {
        warnings[WarningType::resultType] = juce::translate("The type of results is undefined by the plugin and might not be supported!");
    }
    auto const numDimension = descriptor.hasFixedBinCount ? std::min(descriptor.binCount, 2ul) + 1 : 0;
    mAccessor.setAttr<AttrType::resultsType>(static_cast<ResultsType>(numDimension), notification);
    
    // Ensures that the range of results returned for this feature is valid
    auto& valueZoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
    valueZoomAcsr.setAttr<Zoom::AttrType::minimumLength>(descriptor.isQuantized ? descriptor.quantizeStep : std::numeric_limits<double>::epsilon(), notification);
    if(!descriptor.hasKnownExtents)
    {
        warnings[WarningType::zoomMode] = juce::translate("The type of zoom...");
        valueZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max()), notification);
    }
    else
    {
        valueZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range(static_cast<double>(descriptor.minValue), static_cast<double>(descriptor.maxValue)), notification);
    }
    
    // Updates the zoom range of the bins based on the dimensions of the results
    auto& binZoomAcsr = mAccessor.getAccessor<AcsrType::binZoom>(0);
    binZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range(0.0, std::max(static_cast<double>(std::max(descriptor.binCount, 1ul)), 1.0)), notification);
    
    JUCE_COMPILER_WARNING("to clean");
    updateZoomRange(notification);
}

void Analyzer::Director::runAnalysis(NotificationType const notification)
{
    if(mAnalysisProcess.valid())
    {
        mAnalysisState = ProcessState::aborted;
        mAnalysisProcess.get();
    }
    mAnalysisState = ProcessState::available;
    
    auto reader = mAudioFormatReaderManager.getInstance();
    if(reader == nullptr)
    {
        return;
    }
    
    auto instance = mProcessorManager.getInstance();
    if(instance == nullptr || std::abs(instance->getInputSampleRate() - reader->sampleRate) > std::numeric_limits<double>::epsilon())
    {
        instance = std::shared_ptr<Processor>(Analyzer::createProcessor(mAccessor, reader->sampleRate, AlertType::window).release());
        mProcessorManager.setInstance(instance);
    }
    
    if(instance == nullptr)
    {
        return;
    }
    instance->setParameters(mAccessor.getAttr<AttrType::parameters>());
    sanitizeProcessor(notification);
    
    auto const feature = mAccessor.getAttr<AttrType::feature>();
    instance->setParameters(mAccessor.getAttr<AttrType::parameters>());
    if(!instance->prepareForAnalysis(feature, *reader.get()))
    {
        using AlertIconType = juce::AlertWindow::AlertIconType;
        auto const errorMessage = juce::translate("Analysis cannot be performed!");
        
        juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, juce::translate("The analysis \"ANLNAME\" cannot be performed due to incompatibility with the plugin. Please, ensure that the analysis parameters (such as the feature, the sample rate, the window size, the factor overlapping)  are consistent and compatible with the plugin \"PLGKEY\".").replace("ANLNAME", mAccessor.getAttr<AttrType::name>()).replace("PLGKEY", mAccessor.getAttr<AttrType::key>()));
        return;
    }
    
    mAnalysisProcess = std::async([=, this]() -> std::tuple<std::vector<Analyzer::Result>, NotificationType>
    {
        auto expected = ProcessState::available;
        if(!mAnalysisState.compare_exchange_weak(expected, ProcessState::running))
        {
            triggerAsyncUpdate();
            return std::make_tuple(std::vector<Analyzer::Result>{}, notification);
        }

        std::vector<Analyzer::Result> results;
        while(mAnalysisState.load() != ProcessState::aborted && instance->performNextAudioBlock(results))
        {
        }
        expected = ProcessState::running;
        if(mAnalysisState.compare_exchange_weak(expected, ProcessState::ended))
        {
            triggerAsyncUpdate();
            return std::make_tuple(std::move(results), notification);
        }
        triggerAsyncUpdate();
        return std::make_tuple(std::vector<Analyzer::Result>{}, notification);
    });
}

void Analyzer::Director::runRendering(NotificationType const notification)
{
    if(mRenderingProcess.valid())
    {
        mRenderingState = ProcessState::aborted;
        mRenderingProcess.get();
    }
    mRenderingState = ProcessState::available;
    
    auto const& results = mAccessor.getAttr<AttrType::results>();
    if(results.empty() || results[0].values.size() <= 1)
    {
        return;
    }
    
    auto const colourMap = mAccessor.getAttr<AttrType::colourMap>();
    auto const binCounts = static_cast<size_t>(mAccessor.getAccessor<AcsrType::binZoom>(0).getAttr<Zoom::AttrType::globalRange>().getEnd());
    mRenderingProcess = std::async([=, this]() -> std::tuple<juce::Image, NotificationType>
    {
        auto expected = ProcessState::available;
        if(!mRenderingState.compare_exchange_weak(expected, ProcessState::running))
        {
            triggerAsyncUpdate();
            return std::make_tuple(juce::Image(), notification);
        }
        
        anlWeakAssert(results.front().values.size() == binCounts);
        
        auto const witdh = static_cast<int>(results.size());
        auto const height = static_cast<int>(binCounts);
        auto const yadv = static_cast<double>(results.front().values.size()) / static_cast<double>(height);
    
        auto image = juce::Image(juce::Image::PixelFormat::ARGB, witdh, height, false);
        juce::Image::BitmapData const data(image, juce::Image::BitmapData::writeOnly);
        
        auto valueToColour = [&](float const value)
        {
            auto const color = tinycolormap::GetColor(static_cast<double>(value) / (height * 0.25), colourMap);
            return juce::Colour::fromFloatRGBA(static_cast<float>(color.r()), static_cast<float>(color.g()), static_cast<float>(color.b()), 1.0f);
        };
        
        for(int i = 0; i < witdh && mRenderingState.load() != ProcessState::aborted; ++i)
        {
            for(int j = 0; j < height; ++j)
            {
                auto const colour = valueToColour(results[static_cast<size_t>(i)].values[static_cast<size_t>(static_cast<double>(j) * yadv)]);
                data.setPixelColour(i, height - 1 - j, colour);
            }
        }
        
        expected = ProcessState::running;
        if(mRenderingState.compare_exchange_weak(expected, ProcessState::ended))
        {
            triggerAsyncUpdate();
            return std::make_tuple(image, notification);
        }
        triggerAsyncUpdate();
        return std::make_tuple(juce::Image(), notification);
    });
}

void Analyzer::Director::updateZoomRange(NotificationType const notification)
{
    switch(mAccessor.getAttr<AttrType::zoomMode>())
    {
        case ZoomMode::plugin:
            break;
        case ZoomMode::results:
        {
            auto const& results = mAccessor.getAttr<AttrType::results>();
            if(results.empty())
            {
                return;
            }
            auto getZoomInfo = [&]() -> std::tuple<Zoom::Range, double>
            {
                auto constexpr epsilon = std::numeric_limits<double>::epsilon() * 100.0;
                Zoom::Range range;
                for(auto const& result : results)
                {
                    auto const& values = result.values;
                    auto const pair = std::minmax_element(values.cbegin(), values.cend());
                    if(pair.first != values.cend() && pair.second != values.cend())
                    {
                        auto const start = static_cast<double>(*pair.first);
                        auto const end = std::max(static_cast<double>(*pair.second), start + epsilon);
                        range = range.isEmpty() ? Zoom::Range{start, end} : range.getUnionWith({start, end});
                    }
                }
                return range.isEmpty() ? std::make_tuple(Zoom::Range{0.0, 1.0}, 1.0) : std::make_tuple(range, epsilon);
            };
            
            auto const info = getZoomInfo();
            auto& zoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
            zoomAcsr.setAttr<Zoom::AttrType::globalRange>(std::get<0>(info), notification);
            zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(std::get<1>(info), notification);
        }
            break;
        case ZoomMode::custom:
            break;
    }
}

void Analyzer::Director::updateFromResults(NotificationType const notification)
{
    auto const& results = mAccessor.getAttr<AttrType::results>();
    if(results.empty())
    {
        return;
    }
    
    auto getResultsType = [&]()
    {
        if(results.empty())
        {
            return ResultsType::undefined;
        }
        if(results[0].values.empty())
        {
            return ResultsType::points;
        }
        if(results[0].values.size() == 1)
        {
            return ResultsType::segments;
        }
        return ResultsType::matrix;
    };
    
    auto warnings = mAccessor.getAttr<AttrType::warnings>();
    auto const previousResultsType = mAccessor.getAttr<AttrType::resultsType>();
    auto const newResultsType = getResultsType();
    mAccessor.setAttr<AttrType::resultsType>(newResultsType, notification);
    if(previousResultsType != ResultsType::undefined && previousResultsType != newResultsType)
    {
        warnings[WarningType::resultType] = juce::translate("The results returned by the plugin are incompatible with the plugin description of the feature.");
    }
    mAccessor.setAttr<AttrType::warnings>(warnings, notification);
}

void Analyzer::Director::handleAsyncUpdate()
{
    if(mAnalysisProcess.valid())
    {
        auto expected = ProcessState::ended;
        if(mAnalysisState.compare_exchange_weak(expected, ProcessState::available))
        {
            auto const result = mAnalysisProcess.get();
            mAccessor.setAttr<AttrType::results>(std::get<0>(result), std::get<1>(result));
        }
        else if(expected == ProcessState::aborted)
        {
            mAnalysisProcess.get();
            mAnalysisState = ProcessState::available;
        }
    }
    
    if(mRenderingProcess.valid())
    {
        auto expected = ProcessState::ended;
        if(mRenderingState.compare_exchange_weak(expected, ProcessState::available))
        {
            auto const result = mRenderingProcess.get();
            mAccessor.setImage(std::make_shared<juce::Image>(std::get<0>(result)), std::get<1>(result));
        }
        else if(expected == ProcessState::aborted)
        {
            mAnalysisProcess.get();
            mRenderingState = ProcessState::available;
        }
    }
}

ANALYSE_FILE_END
