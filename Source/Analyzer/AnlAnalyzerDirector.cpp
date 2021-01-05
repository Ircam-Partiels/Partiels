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
            case AttrType::results:
            {
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
    
    auto verifyFeature = [&]() -> bool
    {
        auto const feature = mAccessor.getAttr<AttrType::feature>();
        if(feature < descriptors.size())
        {
            return true;
        }
        else if(descriptors.empty())
        {
            warnings[WarningType::feature] = juce::translate("The plugin doesn't have any feature or the feature is not supported!");
            return false;
        }
        else
        {
            warnings[WarningType::feature] = juce::translate("The selected feature FTRINDEX is not supported by the plugin!").replace("FTRINDEX", juce::String(feature));
            return false;
        }
    };
    
    if(!verifyFeature())
    {
        mAccessor.setAttr<AttrType::warnings>(warnings, notification);
        return;
    }
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
        if(instance != nullptr)
        {
            mAccessor.setAttr<AttrType::parameters>(instance->getParameters(), notification);
            auto const descriptors = instance->getOutputDescriptors();
            auto warnings = mAccessor.getAttr<AttrType::warnings>();
            
            // Sanitize the feature
            anlWeakAssert(!descriptors.empty());
            auto const feature = mAccessor.getAttr<AttrType::feature>();
            if(feature < descriptors.size())
            {
                warnings.erase(WarningType::feature);
            }
            else if(descriptors.empty())
            {
                warnings[WarningType::feature] = juce::translate("The plugin doesn't have any feature or the feature is not supported!");
            }
            else
            {
                warnings[WarningType::feature] = juce::translate("The selected feature FTRINDEX is not supported by the plugin!").replace("FTRINDEX", juce::String(feature));
            }
            
            // Sanitize the results type and zooms
            auto const descriptor = descriptors.size() > feature ? descriptors[feature] : OutputDescriptor{};
            anlWeakAssert(descriptor.hasFixedBinCount);
            if(descriptor.hasFixedBinCount)
            {
                auto const numZoomAcsrs = std::min(descriptor.binCount, 2ul);
                while(mAccessor.getNumAccessors<AcsrType::zoom>() < numZoomAcsrs)
                {
                    mAccessor.insertAccessor<AcsrType::zoom>(mAccessor.getNumAccessors<AcsrType::zoom>(), notification);
                }
                while(mAccessor.getNumAccessors<AcsrType::zoom>() > numZoomAcsrs)
                {
                    auto const index = mAccessor.getNumAccessors<AcsrType::zoom>() - 1;
                    mAccessor.eraseAccessor<AcsrType::zoom>(index, notification);
                }
            }
            else
            {
                mAccessor.setAttr<AttrType::resultsType>(ResultsType::undefined, notification);
                warnings[WarningType::resultType] = juce::translate("The type of results is undefined by the plugin and might not be supported!");
                mAccessor.setAttr<AttrType::warnings>(warnings, notification);
            }
            
            if(!instance->hasZoomInfo(feature) && mAccessor.getAttr<AttrType::zoomMode>() == ZoomMode::plugin)
            {
                mAccessor.setAttr<AttrType::zoomMode>(ZoomMode::results, notification);
            }
        }
        mProcessorManager.setInstance(instance);
    }
    
    if(instance == nullptr)
    {
        return;
    }
    
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
    mRenderingProcess = std::async([=, this]() -> std::tuple<juce::Image, NotificationType>
    {
        auto expected = ProcessState::available;
        if(!mRenderingState.compare_exchange_weak(expected, ProcessState::running))
        {
            triggerAsyncUpdate();
            return std::make_tuple(juce::Image(), notification);
        }
        
        auto const witdh = static_cast<int>(results.size());
        auto const height = static_cast<int>(results.front().values.size());
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
                auto const colour = valueToColour(results[static_cast<size_t>(i)].values[static_cast<size_t>(j)]);
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
    JUCE_COMPILER_WARNING("look at that");
//    Analyzer::ResultType Analyzer::Accessor::getResultType() const
//    {
//        auto const& results = getAttr<AttrType::results>();
//        if(results.empty())
//        {
//            return ResultType::undefined;
//        }
//        if(results[0].values.empty())
//        {
//            return ResultType::points;
//        }
//        if(results[0].values.size() == 1)
//        {
//            return ResultType::segments;
//        }
//        return ResultType::matrix;
//    }
//
    if(mAccessor.getNumAccessors<AcsrType::zoom>() == 0)
    {
        return;
    }
    switch(mAccessor.getAttr<AttrType::zoomMode>())
    {
        case ZoomMode::plugin:
        {
            auto instance = mProcessorManager.getInstance();
            if(instance != nullptr)
            {
                auto const feature = mAccessor.getAttr<AttrType::feature>();
                anlStrongAssert(instance->hasZoomInfo(feature));
                auto const info = instance->getZoomInfo(feature);
                auto& zoomAcsr = mAccessor.getAccessor<AcsrType::zoom>(0);
                
                zoomAcsr.setAttr<Zoom::AttrType::globalRange>(std::get<0>(info), notification);
                zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(std::get<1>(info), notification);
            }
        }
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
            auto& zoomAcsr = mAccessor.getAccessor<AcsrType::zoom>(0);
            zoomAcsr.setAttr<Zoom::AttrType::globalRange>(std::get<0>(info), notification);
            zoomAcsr.setAttr<Zoom::AttrType::minimumLength>(std::get<1>(info), notification);
        }
            break;
        case ZoomMode::custom:
            break;
    }
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
            if(mAccessor.getAttr<AttrType::zoomMode>() == ZoomMode::results)
            {
                updateZoomRange(std::get<1>(result));
            }
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
