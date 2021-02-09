#include "AnlAnalyzerDirector.h"
#include "AnlAnalyzerPropertyPanel.h"

#include "../Plugin/AnlPluginProcessor.h"
#include "../Plugin/AnlPluginListScanner.h"
#include "../Plot/AnlPlotRenderer.h"


ANALYSE_FILE_BEGIN

Analyzer::Director::Director(Accessor& accessor, std::unique_ptr<juce::AudioFormatReader> audioFormatReader)
: mAccessor(accessor)
, mAudioFormatReaderManager(std::move(audioFormatReader))
{
    accessor.onAttrUpdated = [&](AttrType anlAttr, NotificationType notification)
    {
        switch (anlAttr)
        {
            case AttrType::name:
                break;
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
                break;
            
            case AttrType::results:
            {
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
    
    auto const descriptions = PluginList::Scanner::getPluginDescription(key, reader->sampleRate, AlertType::window);
    anlWeakAssert(descriptions != Plugin::Description{});
    if(descriptions == Plugin::Description{})
    {
        return;
    }
    mAccessor.setAttr<AttrType::description>(descriptions, notification);
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
    
    auto const colourMap = mAccessor.getAccessor<AcsrType::plot>(0).getAttr<Plot::AttrType::colourMap>();
    
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
        
        auto image = Plot::Renderer::createImage(results, colourMap, [this]()
        {
            return mRenderingState.load() != ProcessState::aborted;
        });
    
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
    auto& plotAscr = mAccessor.getAccessor<AcsrType::plot>(0);
    auto const& results = mAccessor.getAttr<AttrType::results>();
    if(results.empty())
    {
        return;
    }
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
    
    auto const info = getZoomInfo();
    auto& valueZoomAcsr = plotAscr.getAccessor<Plot::AcsrType::valueZoom>(0);
    valueZoomAcsr.setAttr<Zoom::AttrType::globalRange>(std::get<0>(info), notification);
    valueZoomAcsr.setAttr<Zoom::AttrType::minimumLength>(std::get<1>(info), notification);
    
    auto& binZoomAcsr = plotAscr.getAccessor<Plot::AcsrType::binZoom>(0);
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
