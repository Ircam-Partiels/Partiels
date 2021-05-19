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

    auto const columns = accessor.getAttr<AttrType::results>().getColumns();
    if(columns == nullptr || columns->empty())
    {
        if(onRenderingEnded != nullptr)
        {
            onRenderingEnded({});
        }
        return;
    }

    auto const width = static_cast<int>(columns->at(0).size());
    auto const height = static_cast<int>(std::get<2>(columns->at(0).front()).size());
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

                                        std::array<juce::PixelARGB, 256> colours;
                                        for(size_t i = 0; i < colours.size(); ++i)
                                        {
                                            auto const color = tinycolormap::GetColor(static_cast<double>(i) / 256.0, colourMap);
                                            colours[i] = juce::Colour::fromFloatRGBA(static_cast<float>(color.r()), static_cast<float>(color.g()), static_cast<float>(color.b()), 1.0f).getPixelARGB();
                                        }

                                        auto createImage = [&](size_t index, int imageWidth, int imageHeight, float& advancement, std::function<bool(void)> predicate) -> juce::Image
                                        {
                                            advancement = 0.0f;
                                            auto const& channel = columns->at(index);
                                            if(channel.empty() || std::get<2>(channel.front()).empty())
                                            {
                                                return {};
                                            }

                                            anlStrongAssert(imageWidth > 0 && imageHeight > 0);
                                            if(imageWidth <= 0 || imageHeight <= 0)
                                            {
                                                return {};
                                            }

                                            auto const numColumns = static_cast<int>(channel.size());
                                            auto const numRows = static_cast<int>(std::get<2>(channel.front()).size());
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

                                            auto const valueStart = static_cast<float>(valueRange.getStart());
                                            auto const valueScale = static_cast<float>(colours.size()) / static_cast<float>(valueRange.getLength());

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
                                                auto const& values = std::get<2>(channel.at(columnIndex));
                                                auto const valuesSize = values.size();
                                                for(int j = 0; j < imageHeight; ++j)
                                                {
                                                    auto const rowIndex = std::min(static_cast<size_t>(std::round(j * hd)), static_cast<size_t>(numRows - 1));
                                                    if(rowIndex >= valuesSize)
                                                    {
                                                        anlDebug("Track::Graphics::Process", "inconsistent values at [" + juce::String(columnIndex) + "][" + juce::String(rowIndex) + "]");
                                                    }
                                                    auto const& value = rowIndex < valuesSize ? values.at(rowIndex) : valueStart;
                                                    auto const colorIndex = std::min(std::max(static_cast<size_t>(std::ceil(value - valueStart) * valueScale), 0_z), colours.size() - 1_z);
                                                    reinterpret_cast<juce::PixelARGB*>(pixel)->set(colours[colorIndex]);
                                                    pixel -= lineStride;
                                                }
                                                advancement = static_cast<float>(i) / static_cast<float>(imageWidth);
                                            }
                                            advancement = 1.0f;
                                            return predicate() ? image : juce::Image();
                                        };
                                        auto const numChannels = columns->size();

                                        {
                                            std::unique_lock<std::mutex> imageLock(mMutex);
                                            mImages.resize(numChannels);
                                        }

                                        auto constexpr maxImageSize = 4096;
                                        auto const dimension = std::max(width, height);
                                        if(dimension > maxImageSize)
                                        {
                                            float advancement = 0.0f;
                                            for(size_t channel = 0; channel < numChannels; ++channel)
                                            {
                                                auto image = createImage(channel, std::min(maxImageSize, width), std::min(maxImageSize, height), advancement, [&]()
                                                                         {
                                                                             mAdvancement.store(0.05f * (static_cast<float>(channel) + advancement) / static_cast<float>(numChannels));
                                                                             return mRenderingState.load() != ProcessState::aborted;
                                                                         });

                                                if(image.isNull())
                                                {
                                                    mRenderingState = ProcessState::aborted;
                                                    triggerAsyncUpdate();
                                                    return;
                                                }

                                                std::unique_lock<std::mutex> imageLock(mMutex);
                                                mImages[channel].push_back(image);
                                                imageLock.unlock();
                                                triggerAsyncUpdate();

                                                mAdvancement.store(0.05f * (static_cast<float>(channel) / static_cast<float>(numChannels)));
                                            }
                                            mAdvancement.store(0.05f);
                                        }

                                        std::vector<juce::Image> tempImages;
                                        auto const currentAdvancement = mAdvancement.load();
                                        float advancement = 0.0f;
                                        for(size_t channel = 0; channel < numChannels; ++channel)
                                        {
                                            auto image = createImage(channel, width, height, advancement, [&]()
                                                                     {
                                                                         mAdvancement.store(currentAdvancement + (static_cast<float>(channel) + 0.98f - currentAdvancement) * advancement / static_cast<float>(numChannels));
                                                                         return mRenderingState.load() != ProcessState::aborted;
                                                                     });

                                            tempImages.push_back(image);
                                            mAdvancement.store(0.98f * (static_cast<float>(channel) / static_cast<float>(numChannels)));
                                            if(image.isNull())
                                            {
                                                mRenderingState = ProcessState::aborted;
                                                triggerAsyncUpdate();
                                                return;
                                            }
                                        }
                                        mAdvancement.store(0.98f);

                                        for(size_t channel = 0; channel < numChannels; ++channel)
                                        {
                                            auto const& image = tempImages[channel];
                                            for(int i = maxImageSize; i < dimension; i *= 2)
                                            {
                                                auto const rescaledImage = image.rescaled(std::min(i, image.getWidth()), std::min(i, image.getHeight()));
                                                std::unique_lock<std::mutex> imageLock(mMutex);
                                                if(i == maxImageSize)
                                                {
                                                    mImages[channel] = {rescaledImage};
                                                }
                                                else
                                                {
                                                    mImages[channel].push_back(rescaledImage);
                                                }
                                                imageLock.unlock();
                                                triggerAsyncUpdate();
                                                mAdvancement.store(std::min(mAdvancement.load() + 0.005f, 0.99f));
                                            }

                                            std::unique_lock<std::mutex> imageLock(mMutex);
                                            mImages[channel].push_back(image);
                                            imageLock.unlock();
                                        }

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
