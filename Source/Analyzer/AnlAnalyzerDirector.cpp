#include "AnlAnalyzerDirector.h"
#include "AnlAnalyzerPropertyPanel.h"

#include "../Plugin/AnlPluginProcessor.h"
#include "../Plugin/AnlPluginListScanner.h"

JUCE_COMPILER_WARNING("check this")
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
            {
                runAnalysis(notification);
            }
                break;
            case AttrType::state:
            {
                runAnalysis(notification);
            }
                break;
            case AttrType::zoomMode:
            {
                updateZoomRange(notification);
            }
                break;
            case AttrType::description:
            {
                
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
    anlStrongAssert(audioFormatReader == nullptr || audioFormatReader != mAudioFormatReaderManager);
    if(audioFormatReader == mAudioFormatReaderManager)
    {
        return;
    }
    
    std::unique_lock<std::mutex> lock(mAnalysisMutex);
    if(mAnalysisProcess.valid())
    {
        mAnalysisState = ProcessState::aborted;
        mAnalysisProcess.get();
    }
    mAnalysisState = ProcessState::available;
    mAudioFormatReaderManager = std::move(audioFormatReader);
    lock.unlock();
    
    runAnalysis(notification);
}

//void Analyzer::Director::sanitizeProcessor(NotificationType const notification)
//{
//    auto const descriptors = processor->getOutputDescriptors();
//    anlWeakAssert(!descriptors.empty());
//    
//    JUCE_COMPILER_WARNING("to fix feature")
//    auto const descriptor = descriptors[0];
//    
//    // Ensures that the type of results returned for this feature is valid
//    anlWeakAssert(descriptor.hasFixedBinCount);
//    if(!descriptor.hasFixedBinCount)
//    {
//        warnings[WarningType::resultType] = juce::translate("The type of results is undefined by the plugin and might not be supported!");
//    }
//    auto const numDimension = descriptor.hasFixedBinCount ? std::min(descriptor.binCount, 2_z) + 1 : 0;
//    mAccessor.setAttr<AttrType::resultsType>(static_cast<ResultsType>(numDimension), notification);
//    
//    // Ensures that the range of results returned for this feature is valid
//    auto& valueZoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
//    valueZoomAcsr.setAttr<Zoom::AttrType::minimumLength>(descriptor.isQuantized ? descriptor.quantizeStep : std::numeric_limits<double>::epsilon(), notification);
//    if(!descriptor.hasKnownExtents)
//    {
//        warnings[WarningType::zoomMode] = juce::translate("The type of zoom...");
//        valueZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max()), notification);
//    }
//    else
//    {
//        valueZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range(static_cast<double>(descriptor.minValue), static_cast<double>(descriptor.maxValue)), notification);
//    }
//    
//    // Updates the zoom range of the bins based on the dimensions of the results
//    auto& binZoomAcsr = mAccessor.getAccessor<AcsrType::binZoom>(0);
//    binZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range(0.0, std::max(static_cast<double>(std::max(descriptor.binCount, 1_z)), 1.0)), notification);
//}

void Analyzer::Director::runAnalysis(NotificationType const notification)
{
    std::unique_lock<std::mutex> lock(mAnalysisMutex, std::try_to_lock);
    anlStrongAssert(lock.owns_lock());
    if(!lock.owns_lock())
    {
        return;
    }
    if(mAnalysisProcess.valid())
    {
        mAnalysisState = ProcessState::aborted;
        mAnalysisProcess.get();
    }
    mAnalysisState = ProcessState::available;
    
    auto* reader = mAudioFormatReaderManager.get();
    if(reader == nullptr)
    {
        return;
    }
    
    auto const key = mAccessor.getAttr<AttrType::key>();
    if(key == Plugin::Key{})
    {
        return;
    }
    
    auto const state = mAccessor.getAttr<AttrType::state>();
    auto processor = Plugin::Processor::create(key, state, *reader, AlertType::window);
    if(processor == nullptr)
    {
        mAccessor.setAttr<AttrType::warnings>(std::map<WarningType, juce::String>{}, notification);
        return;
    }
    
    auto const descriptions = PluginList::Scanner::getPluginDescription(key, 48000.0, AlertType::window);
    anlWeakAssert(descriptions != Plugin::Description{});
    if(descriptions != Plugin::Description{} && descriptions != mAccessor.getAttr<AttrType::description>())
    {
        if(mAccessor.getAttr<AttrType::description>() != Plugin::Description{})
        {
            JUCE_COMPILER_WARNING("ALert Window - New descriptin or key with description initialization");
        }
        mAccessor.setAttr<AttrType::description>(descriptions, notification);
    }
    
    updateZoomRange(notification);

    mAnalysisProcess = std::async([=, this, processor = std::move(processor)]() -> std::tuple<std::vector<Plugin::Result>, NotificationType>
    {
        juce::Thread::setCurrentThreadName("Analyzer::Director::runAnalysis");
        
        auto expected = ProcessState::available;
        if(!mAnalysisState.compare_exchange_weak(expected, ProcessState::running))
        {
            triggerAsyncUpdate();
            return std::make_tuple(std::vector<Plugin::Result>{}, notification);
        }

        std::vector<Plugin::Result> results;
        while(mAnalysisState.load() != ProcessState::aborted && processor->performNextAudioBlock(results))
        {
        }
        expected = ProcessState::running;
        if(mAnalysisState.compare_exchange_weak(expected, ProcessState::ended))
        {
            triggerAsyncUpdate();
            return std::make_tuple(std::move(results), notification);
        }
        triggerAsyncUpdate();
        return std::make_tuple(std::vector<Plugin::Result>{}, notification);
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
    
    auto const witdh = static_cast<int>(results.size());
    auto const height = static_cast<int>(results.empty() ? 0 : results[0].values.size());
    anlWeakAssert(witdh > 0 && height > 0);
    if(witdh < 0 || height < 0)
    {
        return;
    }
    
    mRenderingProcess = std::async([=, this]() -> std::tuple<juce::Image, NotificationType>
    {
        juce::Thread::setCurrentThreadName("Analyzer::Director::runRendering");
        auto expected = ProcessState::available;
        if(!mRenderingState.compare_exchange_weak(expected, ProcessState::running))
        {
            triggerAsyncUpdate();
            return std::make_tuple(juce::Image(), notification);
        }
        
        auto image = juce::Image(juce::Image::PixelFormat::ARGB, witdh, height, false);
        juce::Image::BitmapData const data(image, juce::Image::BitmapData::writeOnly);
        
        auto valueToColour = [&](float const value)
        {
            auto const color = tinycolormap::GetColor(static_cast<double>(value) / (height * 0.25), colourMap);
            return juce::Colour::fromFloatRGBA(static_cast<float>(color.r()), static_cast<float>(color.g()), static_cast<float>(color.b()), 1.0f);
        };
        
        for(int i = 0; i < witdh && mRenderingState.load() != ProcessState::aborted; ++i)
        {
            for(int j = 0; j < height && mRenderingState.load() != ProcessState::aborted; ++j)
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
    switch(mAccessor.getAttr<AttrType::zoomMode>())
    {
        case ZoomMode::custom:
        case ZoomMode::plugin:
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
            auto& valueZoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
            valueZoomAcsr.setAttr<Zoom::AttrType::globalRange>(std::get<0>(info), notification);
            valueZoomAcsr.setAttr<Zoom::AttrType::minimumLength>(std::get<1>(info), notification);
            
            auto& binZoomAcsr = mAccessor.getAccessor<AcsrType::binZoom>(0);
            binZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{0.0, static_cast<double>(results[0].values.size())}, notification);
            binZoomAcsr.setAttr<Zoom::AttrType::minimumLength>(1.0, notification);
        }
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
