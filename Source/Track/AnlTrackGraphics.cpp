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
        if(onRenderingEnded != nullptr)
        {
            onRenderingEnded({});
        }
        return;
    }
    
    auto const width = static_cast<int>(results->size());
    auto const height = static_cast<int>(results->at(0).values.size());
    anlWeakAssert(width > 0 && height > 0);
    if(width < 0 || height < 0)
    {
        if(onRenderingEnded != nullptr)
        {
            onRenderingEnded({});
        }
        return;
    }
    
    auto const colourMap = accessor.getAttr<AttrType::colours>().map;
    auto const valueRange = accessor.getAcsr<AcsrType::valueZoom>().getAttr<Zoom::AttrType::visibleRange>();
    
    mChrono.start();
    mRenderingProcess = std::thread([=, this]()
    {
        juce::Thread::setCurrentThreadName("Track::Graphics::Process");
        juce::Thread::setCurrentThreadPriority(10);
        auto expected = ProcessState::available;
        if(!mRenderingState.compare_exchange_weak(expected, ProcessState::running))
        {
            triggerAsyncUpdate();
        }
        
        auto createImage = [&](int imageWidth, int imageHeight, float& advancement, std::function<bool(void)> predicate) -> juce::Image
        {
            advancement = 0.0f;
            if(results->empty())
            {
                return {};
            }
            
            anlStrongAssert(imageWidth > 0 && imageHeight > 0);
            if(imageWidth <= 0 || imageHeight <= 0)
            {
                return {};
            }
            
            auto const numColumns = static_cast<int>(results->size());
            auto const numRows = static_cast<int>(results->front().values.size());
            anlStrongAssert(numColumns > 0 && numRows > 0 && numColumns >= width && numRows >= imageHeight);
            if(numColumns <= 0 || numRows <= 0)
            {
                return {};
            }
            imageWidth = std::min(numColumns, imageWidth);
            imageHeight = std::min(numRows, imageHeight);
            
            if(predicate == nullptr)
            {
                predicate = []()
                {
                    return true;
                };
            }
            
            auto image = juce::Image(juce::Image::PixelFormat::ARGB, imageWidth, imageHeight, false);
            juce::Image::BitmapData const bitmapData(image, juce::Image::BitmapData::writeOnly);
            
            auto const valueStart = valueRange.getStart();
            auto const valueLength = valueRange.getLength();
            
            auto* data = bitmapData.data;
            auto const pixelStride = static_cast<size_t>(bitmapData.pixelStride);
            auto const lineStride = static_cast<size_t>(bitmapData.lineStride);
            auto const columnStride = lineStride * static_cast<size_t>(imageHeight - 1);
            
            auto const wd = std::max(static_cast<double>(numColumns) / static_cast<double>(imageWidth), 1.0);
            auto const hd = std::max(static_cast<double>(numRows) / static_cast<double>(imageHeight), 1.0);
            for(int i = 0; i < imageWidth && predicate(); ++i)
            {
                auto* pixel = data + static_cast<size_t>(i) * pixelStride + columnStride;
                auto const columnIndex = std::min(static_cast<size_t>(std::round(i * wd)), static_cast<size_t>(numColumns - 1));
                auto const& values = results->at(columnIndex).values;
                for(int j = 0; j < imageHeight; ++j)
                {
                    auto const rowIndex = std::min(static_cast<size_t>(std::round(j * hd)), static_cast<size_t>(numRows - 1));
                    auto const& value = values[rowIndex];
                    auto const color = tinycolormap::GetColor(static_cast<double>((value - valueStart) / valueLength), colourMap);
                    auto const rgba = juce::Colour::fromFloatRGBA(static_cast<float>(color.r()), static_cast<float>(color.g()), static_cast<float>(color.b()), 1.0f).getPixelARGB();
                    reinterpret_cast<juce::PixelARGB*>(pixel)->set(rgba);
                    pixel -= lineStride;
                }
                advancement = static_cast<float>(i) / static_cast<float>(imageWidth);
            }
            advancement = 1.0f;
            return predicate() ? image : juce::Image();
        };
        
        auto constexpr maxImageSize = 4096;
        auto const dimension = std::max(width, height);
        if(dimension > maxImageSize)
        {
            float advancement = 0.0f;
            auto image = createImage(std::min(maxImageSize, width), std::min(maxImageSize, height), advancement, [&]()
            {
                mAdvancement.store(0.05f * advancement);
                return mRenderingState.load() != ProcessState::aborted;
            });
            
            if(image.isNull())
            {
                mRenderingState = ProcessState::aborted;
                triggerAsyncUpdate();
                return;
            }
            
            std::unique_lock<std::mutex> imageLock(mMutex);
            mImages.push_back(image);
            imageLock.unlock();
            triggerAsyncUpdate();
            
            mAdvancement.store(0.05f);
        }
        
        auto const currentAdvancement = mAdvancement.load();
        float advancement = 0.0f;
        auto image = createImage(width, height, advancement, [&]()
        {
            mAdvancement.store(currentAdvancement + (0.98f - currentAdvancement) * advancement);
            return mRenderingState.load() != ProcessState::aborted;
        });
        
        mAdvancement.store(0.98f);
        if(image.isNull())
        {
            mRenderingState = ProcessState::aborted;
            triggerAsyncUpdate();
            return;
        }
        
        for(int i = maxImageSize; i < dimension; i *= 2)
        {
            auto const rescaledImage = image.rescaled(std::min(i, image.getWidth()), std::min(i, image.getHeight()));
            std::unique_lock<std::mutex> imageLock(mMutex);
            if(i == maxImageSize)
            {
                mImages = {rescaledImage};
            }
            else
            {
                mImages.push_back(rescaledImage);
            }
            imageLock.unlock();
            triggerAsyncUpdate();
            mAdvancement.store(std::min(mAdvancement.load() + 0.005f, 0.99f));
        }
        
        std::unique_lock<std::mutex> imageLock(mMutex);
        mImages.push_back(image);
        imageLock.unlock();

        mAdvancement.store(1.0f);
        expected = ProcessState::running;
        mRenderingState.compare_exchange_weak(expected, ProcessState::ended);
        triggerAsyncUpdate();
    });
}

bool Track::Graphics::isRunning() const
{
    return mRenderingProcess.joinable();
}

float Track::Graphics::getAdvancement() const
{
    return mAdvancement.load();
}

void Track::Graphics::handleAsyncUpdate()
{
    std::unique_lock<std::mutex> lock(mRenderingMutex);
    if(mRenderingProcess.joinable())
    {
        anlWeakAssert(mRenderingState != ProcessState::available);
        
        auto expected = ProcessState::ended;
        if(mRenderingState.load() == ProcessState::running)
        {
            std::unique_lock<std::mutex> imageLock(mMutex);
            if(onRenderingEnded != nullptr)
            {
                onRenderingUpdated(mImages);
            }
            imageLock.unlock();
        }
        else if(mRenderingState.compare_exchange_weak(expected, ProcessState::available))
        {
            mRenderingProcess.join();
            mChrono.stop();
            std::unique_lock<std::mutex> imageLock(mMutex);
            if(onRenderingEnded != nullptr)
            {
                onRenderingEnded(mImages);
            }
            imageLock.unlock();
            mRenderingState = ProcessState::available;
        }
        else
        {
            mRenderingProcess.join();
            if(onRenderingAborted != nullptr)
            {
                onRenderingAborted();
            }
            std::unique_lock<std::mutex> imageLock(mMutex);
            mImages = {};
            imageLock.unlock();
            mRenderingState = ProcessState::available;
        }
    }
}

void Track::Graphics::abortRendering()
{
    mAdvancement.store(0.0f);
    if(mRenderingProcess.joinable())
    {
        mRenderingState = ProcessState::aborted;
        mRenderingProcess.join();
        cancelPendingUpdate();
        
        if(onRenderingAborted != nullptr)
        {
            onRenderingAborted();
        }
    }
    std::unique_lock<std::mutex> lock(mMutex);
    mImages = {};
    lock.unlock();
    mRenderingState = ProcessState::available;
}

ANALYSE_FILE_END
