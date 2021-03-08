#include "AnlAnalyzerDirector.h"
#include "AnlAnalyzerPropertyPanel.h"
#include "AnlAnalyzerRenderer.h"

#include "../Plugin/AnlPluginProcessor.h"
#include "../Plugin/AnlPluginListScanner.h"

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
            case AttrType::state:
            {
                runAnalysis(notification);
            }
                break;
            case AttrType::results:
            case AttrType::name:
            case AttrType::description:
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
    runAnalysis(NotificationType::synchronous);
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
        cancelPendingUpdate();
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
        cancelPendingUpdate();
    }
    mAnalysisState = ProcessState::available;
    mAudioFormatReaderManager = std::move(audioFormatReader);
    lock.unlock();
    
    runAnalysis(notification);
}

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
        cancelPendingUpdate();
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
    
    auto state = mAccessor.getAttr<AttrType::state>();
    auto processor = Plugin::Processor::create(key, state, *reader, AlertType::window);
    if(processor == nullptr)
    {
        mAccessor.setAttr<AttrType::warnings>(std::map<WarningType, juce::String>{}, notification);
        return;
    }
    
    auto const description = mPluginListScanner.getPluginDescription(key, reader->sampleRate, AlertType::window);
    anlWeakAssert(description != Plugin::Description{});
    if(description == Plugin::Description{})
    {
        return;
    }
    mAccessor.setAttr<AttrType::description>(description, notification);
    mAccessor.setAttr<AttrType::processing>(true, notification);
    
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

void Analyzer::Director::updateZooms(NotificationType const notification)
{
    JUCE_COMPILER_WARNING("If zoom extent is defined, the zoom can be updated before the analysis")
    auto const& results = mAccessor.getAttr<AttrType::results>();
    if(results.empty())
    {
        mUpdateZoom = std::make_tuple(true, notification);
        return;
    }
    mUpdateZoom = std::make_tuple(false, notification);
    auto getZoomInfo = [&]() -> std::tuple<Zoom::Range, double>
    {
        Zoom::Range range;
        bool initialized = false;
        for(auto const& result : results)
        {
            auto const& values = result.values;
            auto const pair = std::minmax_element(values.cbegin(), values.cend());
            if(pair.first != values.cend() && pair.second != values.cend())
            {
                auto const start = static_cast<double>(*pair.first);
                auto const end = static_cast<double>(*pair.second);
                range = !initialized ? Zoom::Range{start, end} : range.getUnionWith({start, end});
                initialized = true;
            }
        }
        auto constexpr epsilon = std::numeric_limits<double>::epsilon() * 100.0;
        return !initialized ? std::make_tuple(Zoom::Range{0.0, 1.0}, 1.0) : std::make_tuple(range, epsilon);
    };
    
    auto& valueZoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
    auto const& output = mAccessor.getAttr<AttrType::description>().output;
    if(output.hasKnownExtents)
    {
        valueZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{static_cast<double>(output.minValue), static_cast<double>(output.maxValue)}, notification);
        valueZoomAcsr.setAttr<Zoom::AttrType::minimumLength>((output.isQuantized ? static_cast<double>(output.quantizeStep) : std::numeric_limits<double>::epsilon()), notification);
    }
    else
    {
        auto const info = getZoomInfo();
        valueZoomAcsr.setAttr<Zoom::AttrType::globalRange>(std::get<0>(info), notification);
        valueZoomAcsr.setAttr<Zoom::AttrType::minimumLength>(std::get<1>(info), notification);
    }
    
    auto& binZoomAcsr = mAccessor.getAccessor<AcsrType::binZoom>(0);
    binZoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{0.0, static_cast<double>(results[0].values.size())}, notification);
    binZoomAcsr.setAttr<Zoom::AttrType::minimumLength>(1.0, notification);
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
            mAccessor.setAttr<AttrType::processing>(false, std::get<1>(result));
            if(std::get<0>(mUpdateZoom))
            {
                updateZooms(std::get<1>(mUpdateZoom));
            }
        }
        else if(expected == ProcessState::aborted)
        {
            mAnalysisProcess.get();
            mAnalysisState = ProcessState::available;
        }
    }
}

ANALYSE_FILE_END
