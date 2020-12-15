#include "AnlAnalyzerDirector.h"
#include "AnlAnalyzerProcessor.h"
#include "AnlAnalyzerPropertyPanel.h"

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
            case AttrType::colourMap:
            case AttrType::results:
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
            auto const descriptors = instance->getOutputDescriptors();
            
            // Sanitize the feature
            {
                anlStrongAssert(!descriptors.empty());
                if(!descriptors.empty() && mAccessor.getAttr<AttrType::feature>() >= descriptors.size())
                {
                    mAccessor.setAttr<AttrType::feature>(descriptors.size() - 1, notification);
                }
            }
            
            // Sanitize the zooms
            {
                auto const feature = mAccessor.getAttr<AttrType::feature>();
                auto const descriptor = descriptors.size() > feature ? descriptors[feature] : OutputDescriptor{};
                if(descriptor.hasFixedBinCount)
                {
                    auto const numZoomAcsrs = std::min(descriptor.binCount, 2ul);
                    while (mAccessor.getNumAccessors<AcsrType::zoom>() < numZoomAcsrs)
                    {
                        mAccessor.insertAccessor<AcsrType::zoom>(mAccessor.getNumAccessors<AcsrType::zoom>(), notification);
                    }
                    while(mAccessor.getNumAccessors<AcsrType::zoom>() > numZoomAcsrs)
                    {
                        auto const index = mAccessor.getNumAccessors<AcsrType::zoom>() - 1;
                        mAccessor.eraseAccessor<AcsrType::zoom>(index, notification);
                    }
                }
                
                if(!instance->hasZoomInfo(feature) && mAccessor.getAttr<AttrType::zoomMode>() == ZoomMode::plugin)
                {
                    mAccessor.setAttr<AttrType::zoomMode>(ZoomMode::results, notification);
                }
            }
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
