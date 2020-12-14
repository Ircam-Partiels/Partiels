#include "AnlAnalyzerDirector.h"
#include "AnlAnalyzerProcessor.h"
#include "AnlAnalyzerPropertyPanel.h"

ANALYSE_FILE_BEGIN

Analyzer::Director::Director(Accessor& accessor)
: mAccessor(accessor)
{
    accessor.onUpdated = [&](AttrType anlAttr, NotificationType notification)
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
            case AttrType::zoom:
            case AttrType::name:
            case AttrType::colour:
            case AttrType::colourMap:
            case AttrType::results:
                break;
        }
    };
    accessor.onUpdated(AttrType::key, NotificationType::synchronous);
}

Analyzer::Director::~Director()
{
    mAccessor.onUpdated = nullptr;
}

void Analyzer::Director::setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, NotificationType const notification)
{
    mAudioFormatReaderManager.setInstance(std::shared_ptr<juce::AudioFormatReader>(audioFormatReader.release()));
    runAnalysis(notification);
}

void Analyzer::Director::runAnalysis(NotificationType const notification)
{
    if(mFuture.valid())
    {
        mState = AnalysisState::abort;
        mFuture.get();
    }
    mState = AnalysisState::available;
    
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
            auto const feature = mAccessor.getAttr<AttrType::feature>();
            if(!instance->hasZoomInfo(feature) && mAccessor.getAttr<AttrType::zoomMode>() == ZoomMode::plugin)
            {
                mAccessor.setAttr<AttrType::zoomMode>(ZoomMode::results, notification);
            }
            JUCE_COMPILER_WARNING("sanitize feature");
        }
        mProcessorManager.setInstance(instance);
    }
    
    if(instance == nullptr)
    {
        return;
    }
    
    auto const feature = mAccessor.getAttr<AttrType::feature>();
    if(!instance->prepareForAnalysis(feature, *reader.get()))
    {
        using AlertIconType = juce::AlertWindow::AlertIconType;
        auto const errorMessage = juce::translate("Analysis cannot be performed!");
        
        juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, juce::translate("The analysis \"ANLNAME\" cannot be performed due to incompatibility with the plugin. Please, ensure that the analysis parameters (such as the feature, the sample rate, the window size, the factor overlapping)  are consistent and compatible with the plugin \"PLGKEY\".").replace("ANLNAME", mAccessor.getAttr<AttrType::name>()).replace("PLGKEY", mAccessor.getAttr<AttrType::key>()));
        return;
    }
    
    mFuture = std::async([=, this]() -> std::tuple<std::vector<Analyzer::Result>, NotificationType>
    {
        auto expected = AnalysisState::available;
        if(!mState.compare_exchange_weak(expected, AnalysisState::run))
        {
            triggerAsyncUpdate();
            return std::make_tuple(std::vector<Analyzer::Result>{}, notification);
        }

        std::vector<Analyzer::Result> results;
        while(mState.load() != AnalysisState::abort && instance->performNextAudioBlock(results))
        {
        }
        triggerAsyncUpdate();
        return std::make_tuple(std::move(results), notification);
    });
}

void Analyzer::Director::updateZoomRange(NotificationType const notification)
{
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
                auto& zoomAcsr = mAccessor.getAccessor<AttrType::zoom>(0);
                
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
                auto const numDimension = results.front().values.size() + 1;
                if(numDimension == 1)
                {
                    return {{0.0, 1.0}, 1.0};
                }
                else if(numDimension == 2)
                {
                    auto pair = std::minmax_element(results.cbegin(), results.cend(), [](auto const& lhs, auto const& rhs)
                                                    {
                        return lhs.values[0] < rhs.values[0];
                    });
                    return {{static_cast<double>(pair.first->values[0]), static_cast<double>(pair.second->values[0])}, std::numeric_limits<double>::epsilon()};
                }
                else
                {
                    auto rit = std::max_element(results.cbegin(), results.cend(), [](auto const& lhs, auto const& rhs)
                                                {
                        return lhs.values.size() < rhs.values.size();
                    });
                    return {{0.0, static_cast<double>(rit->values.size())}, 1.0};
                }
            };
            
            auto const info = getZoomInfo();
            auto& zoomAcsr = mAccessor.getAccessor<AttrType::zoom>(0);
            
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
    if(mFuture.valid())
    {
        auto const result = mFuture.get();
        if(mState != AnalysisState::abort)
        {
            mAccessor.setAttr<AttrType::results>(std::get<0>(result), std::get<1>(result));
            if(mAccessor.getAttr<AttrType::zoomMode>() == ZoomMode::results)
            {
                updateZoomRange(std::get<1>(result));
            }
        }
    }
    mState = AnalysisState::available;
}

ANALYSE_FILE_END
