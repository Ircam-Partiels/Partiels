#include "AnlTrackGraphics.h"

ANALYSE_FILE_BEGIN

Track::Graphics::~Graphics()
{
    std::unique_lock<std::mutex> lock(mRenderingMutex);
    abortRendering();
}

void Track::Graphics::stopRendering()
{
    std::unique_lock<std::mutex> lock(mRenderingMutex);
    abortRendering();
}

void Track::Graphics::runRendering(Accessor const& accessor)
{
    std::unique_lock<std::mutex> lock(mRenderingMutex, std::try_to_lock);
    anlStrongAssert(lock.owns_lock());
    if(!lock.owns_lock())
    {
        return;
    }
    abortRendering();
    
    auto const results = accessor.getAttr<AttrType::results>();
    if(results == nullptr || results->empty() || results->at(0).values.size() <= 1)
    {
        return;
    }
    
    auto const width = static_cast<int>(results->size());
    auto const height = static_cast<int>(results->at(0).values.size());
    anlWeakAssert(width > 0 && height > 0);
    if(width < 0 || height < 0)
    {
        return;
    }
    
    auto const colourMap = accessor.getAttr<AttrType::colours>().map;
    auto const valueRange = accessor.getAccessor<AcsrType::valueZoom>(0).getAttr<Zoom::AttrType::visibleRange>();
    
    mChrono.start();
    mRenderingProcess = std::async([=, this]() -> std::vector<juce::Image>
    {
        juce::Thread::setCurrentThreadName("Track::Graphics::Process");
        juce::Thread::setCurrentThreadPriority(10);
        auto expected = ProcessState::available;
        if(!mRenderingState.compare_exchange_weak(expected, ProcessState::running))
        {
            triggerAsyncUpdate();
            return {};
        }
        
        
        auto image = createImage(*results, colourMap, valueRange, [this]()
        {
            return mRenderingState.load() != ProcessState::aborted;
        });

        if(image.isNull())
        {
            mRenderingState = ProcessState::aborted;
            triggerAsyncUpdate();
            return {};
        }
        
        expected = ProcessState::running;
        if(mRenderingState.compare_exchange_weak(expected, ProcessState::ended))
        {
            triggerAsyncUpdate();
            return {image};
        }
        triggerAsyncUpdate();
        return {};
    });
}

void Track::Graphics::handleAsyncUpdate()
{
    std::unique_lock<std::mutex> lock(mRenderingMutex);
    if(mRenderingProcess.valid())
    {
        anlWeakAssert(mRenderingState != ProcessState::available);
        anlWeakAssert(mRenderingState != ProcessState::running);
        
        auto const results = mRenderingProcess.get();
        auto expected = ProcessState::ended;
        if(mRenderingState.compare_exchange_weak(expected, ProcessState::available))
        {
            mChrono.stop();
            if(onRenderingEnded != nullptr)
            {
                onRenderingEnded(results);
            }
        }
        else if(expected == ProcessState::aborted)
        {
            if(onRenderingAborted != nullptr)
            {
                onRenderingAborted();
            }
        }
        abortRendering();
    }
}

void Track::Graphics::abortRendering()
{
    if(mRenderingProcess.valid())
    {
        mRenderingState = ProcessState::aborted;
        mRenderingProcess.get();
        cancelPendingUpdate();
        
        if(onRenderingAborted != nullptr)
        {
            onRenderingAborted();
        }
    }
    mRenderingState = ProcessState::available;
}

juce::Image Track::Graphics::createImage(std::vector<Plugin::Result> const& results, ColourMap const colourMap, Zoom::Range const valueRange, std::function<bool(void)> predicate)
{
    if(results.empty())
    {
        return {};
    }
    
    auto const width = static_cast<int>(results.size());
    auto const height = static_cast<int>(results.front().values.size());
    anlWeakAssert(width > 0 && height > 0);
    if(width <= 0 || height <= 0)
    {
        return {};
    }
    
    if(predicate == nullptr)
    {
        predicate = []()
        {
            return true;
        };
    }
    
    auto image = juce::Image(juce::Image::PixelFormat::ARGB, width, height, false);
    juce::Image::BitmapData const bitmapData(image, juce::Image::BitmapData::writeOnly);
    
    auto const valueStart = valueRange.getStart();
    auto const valueLength = valueRange.getLength();
    
    auto* data = bitmapData.data;
    auto const pixelStride = static_cast<size_t>(bitmapData.pixelStride);
    auto const lineStride = static_cast<size_t>(bitmapData.lineStride);
    auto const columnStride = lineStride * static_cast<size_t>(height - 1);
    for(int i = 0; i < width && predicate(); ++i)
    {
        auto* pixel = data + static_cast<size_t>(i) * pixelStride + columnStride;
        for(int j = 0; j < height && predicate(); ++j)
        {
            auto const& value = results[static_cast<size_t>(i)].values[static_cast<size_t>(j)];
            auto const color = tinycolormap::GetColor(static_cast<double>((value - valueStart) / valueLength), colourMap);
            auto const rgba = juce::Colour::fromFloatRGBA(static_cast<float>(color.r()), static_cast<float>(color.g()), static_cast<float>(color.b()), 1.0f).getPixelARGB();
            reinterpret_cast<juce::PixelARGB*>(pixel)->set(rgba);
            pixel -= lineStride;
        }
    }
    return predicate() ? image : juce::Image();
}

ANALYSE_FILE_END
