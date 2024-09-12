#include "AnlTrackGraphics.h"
#include "../Plugin/AnlPluginTools.h"
#include "AnlTrackTools.h"

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

bool Track::Graphics::runRendering(Accessor const& accessor, std::unique_ptr<Ive::PluginWrapper> plugin)
{
    auto hasPluginColorMap = accessor.getAttr<AttrType::hasPluginColourMap>();
    std::unique_lock<std::mutex> lock(mRenderingMutex, std::try_to_lock);
    anlStrongAssert(lock.owns_lock());
    if(!lock.owns_lock())
    {
        return hasPluginColorMap;
    }
    abortRendering();

    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        if(onRenderingEnded != nullptr)
        {
            onRenderingEnded({});
        }
        return hasPluginColorMap;
    }

    auto const columns = results.getColumns();
    if(columns == nullptr || columns->empty())
    {
        if(onRenderingEnded != nullptr)
        {
            onRenderingEnded({});
        }
        return hasPluginColorMap;
    }

    auto const& output = accessor.getAttr<AttrType::description>().output;
    auto const height = static_cast<int>(output.hasFixedBinCount ? output.binCount : results.getNumBins().value_or(0_z));
    auto const width = static_cast<int>(results.getNumColumns().value_or(0_z));
    anlWeakAssert(width > 0 && height > 0);
    if(width < 0 || height < 0)
    {
        if(onRenderingEnded != nullptr)
        {
            onRenderingEnded({});
        }
        return hasPluginColorMap;
    }

    mData = results;
    mAccess = std::make_unique<Result::Access>(access);
    if(mAccess == nullptr)
    {
        if(onRenderingEnded != nullptr)
        {
            onRenderingEnded({});
        }
        return hasPluginColorMap;
    }

    auto featureIndex = 0;
    if(plugin != nullptr)
    {
        auto const feature = Plugin::Tools::getFeatureIndex(*plugin.get(), accessor.getAttr<AttrType::key>().feature);
        hasPluginColorMap = feature.has_value() && plugin->supportColorMap(static_cast<int>(feature.value()));
        if(!hasPluginColorMap)
        {
            plugin.reset();
        }
        else
        {
            featureIndex = static_cast<int>(feature.value());
        }
    }

    auto const colourMap = accessor.getAttr<AttrType::colours>().map;
    auto const logScale = accessor.getAttr<AttrType::zoomLogScale>() && Tools::hasVerticalZoomInHertz(accessor);
    auto const valueRange = accessor.getAcsr<AcsrType::valueZoom>().getAttr<Zoom::AttrType::visibleRange>();
    auto const hasDuration = output.hasDuration;
    auto const sampleRate = accessor.getAttr<AttrType::sampleRate>();
    mChrono.start();
    mRenderingProcess = std::thread([=, this, plugin = std::move(plugin)]()
                                    {
                                        juce::Thread::setCurrentThreadName("Track::Graphics::Process");
                                        performRendering(*columns, colourMap, valueRange, logScale, sampleRate, width, height, plugin.get(), featureIndex, hasDuration);
                                    });
    return hasPluginColorMap;
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
    mAccess.reset();
    mData = {};
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
    mAccess.reset();
    mData = {};
}

void Track::Graphics::performRendering(std::vector<Track::Result::Data::Columns> const columns, ColourMap const colourMap, juce::Range<double> const valueRange, bool logScale, double sampleRate, int width, int height, Ive::PluginWrapper* plugin, int featureIndex, bool hasDuration)
{
    mAdvancement.store(0.0f);
    auto expected = ProcessState::available;
    if(!mRenderingState.compare_exchange_weak(expected, ProcessState::running))
    {
        triggerAsyncUpdate();
    }

    std::array<juce::PixelARGB, 256> colours;
    for(size_t i = 0; i < colours.size(); ++i)
    {
        auto const color = tinycolormap::GetColor(static_cast<double>(i) / 255.0, colourMap);
        colours[i] = juce::Colour::fromFloatRGBA(static_cast<float>(color.r()), static_cast<float>(color.g()), static_cast<float>(color.b()), 1.0f).getPixelARGB();
    }

    auto const createImage = [&](size_t index, int imageWidth, int imageHeight, float& advancement, std::function<bool(void)> predicate) -> juce::Image
    {
        advancement = 0.0f;
        auto const& channel = columns.at(index);
        if(channel.empty() || std::get<2>(channel.front()).empty())
        {
            return {};
        }

        anlStrongAssert(imageWidth > 0 && imageHeight > 0);
        if(imageWidth <= 0 || imageHeight <= 0 || width <= 0 || height <= 0)
        {
            return {};
        }

        auto const numColumns = width;
        auto const numRows = height;
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
        auto const valueScale = 255.f / static_cast<float>(valueRange.getLength());

        auto* data = bitmapData.data;
        auto const pixelStride = static_cast<size_t>(bitmapData.pixelStride);
        auto const lineStride = static_cast<size_t>(bitmapData.lineStride);
        auto const columnStride = lineStride * static_cast<size_t>(imageHeight - 1);

        auto const wd = std::max(static_cast<double>(numColumns) / static_cast<double>(imageWidth), 1.0);
        auto const hd = std::max(static_cast<double>(numRows) / static_cast<double>(imageHeight), 1.0);
        for(int i = 0; i < imageWidth && predicate(); ++i)
        {
            auto* pixel = data + static_cast<size_t>(i) * pixelStride + columnStride;
            auto const columnIndex = static_cast<size_t>(std::round(i * wd));
            if(columnIndex >= channel.size())
            {
                for(int j = 0; j < imageHeight; ++j)
                {
                    reinterpret_cast<juce::PixelARGB*>(pixel)->set(colours[0_z]);
                    pixel -= lineStride;
                }
            }
            else
            {
                auto const& result = channel.at(columnIndex);
                if(plugin != nullptr)
                {
                    Vamp::Plugin::Feature feature;
                    feature.hasTimestamp = true;
                    feature.timestamp = Vamp::RealTime::fromSeconds(std::get<0>(result));
                    feature.hasDuration = hasDuration;
                    feature.duration = Vamp::RealTime::fromSeconds(std::get<1>(result));
                    feature.values = std::get<2>(result);
                    auto const colorMap = plugin->getColorMap(featureIndex, feature);
                    for(int j = 0; j < imageHeight; ++j)
                    {
                        auto const rowIndex = static_cast<size_t>(std::round(j * hd));
                        if(rowIndex >= colorMap.size())
                        {
                            reinterpret_cast<juce::PixelARGB*>(pixel)->set(colours[0_z]);
                        }
                        else
                        {
                            auto const colour = juce::Colour(colorMap.at(rowIndex));
                            reinterpret_cast<juce::PixelARGB*>(pixel)->set(colour.getPixelARGB());
                        }
                        pixel -= lineStride;
                    }
                }
                else
                {
                    auto const& values = std::get<2>(result);
                    if(logScale)
                    {
                        auto const hertzRatio = static_cast<double>(imageHeight) / (sampleRate / 2.0);
                        auto const midiRatio = Tools::getMidiFromHertz(sampleRate / 2.0) / static_cast<double>(imageHeight);
                        for(int j = 0; j < imageHeight; ++j)
                        {
                            auto const startMidi = Tools::getHertzFromMidi(static_cast<double>(j) * midiRatio);
                            auto const startRow = static_cast<size_t>(std::max(std::round(startMidi * hertzRatio), 0.0));

                            auto const endMidi = Tools::getHertzFromMidi(static_cast<double>(j + 1) * midiRatio);
                            auto const endRow = static_cast<size_t>(std::max(std::round(endMidi * hertzRatio), 0.0));
                            if(startRow >= values.size())
                            {
                                reinterpret_cast<juce::PixelARGB*>(pixel)->set(colours[0_z]);
                            }
                            else
                            {
                                using diff_t = decltype(values.cbegin())::difference_type;
                                auto const rval = *std::max_element(std::next(values.cbegin(), static_cast<diff_t>(startRow)), std::next(values.cbegin(), static_cast<diff_t>(endRow)));
                                auto const value = std::round((rval - valueStart) * valueScale);
                                auto const colorIndex = static_cast<size_t>(std::min(std::max(value, 0.0f), 255.0f));
                                reinterpret_cast<juce::PixelARGB*>(pixel)->set(colours[colorIndex]);
                            }
                            pixel -= lineStride;
                        }
                    }
                    else
                    {
                        for(int j = 0; j < imageHeight; ++j)
                        {
                            auto const startRow = static_cast<size_t>(std::round(j * hd));
                            auto const endRow = static_cast<size_t>(std::round((j + 1) * hd));
                            if(startRow >= values.size())
                            {
                                reinterpret_cast<juce::PixelARGB*>(pixel)->set(colours[0_z]);
                            }
                            else
                            {
                                using diff_t = decltype(values.cbegin())::difference_type;
                                auto const rval = *std::max_element(std::next(values.cbegin(), static_cast<diff_t>(startRow)), std::next(values.cbegin(), static_cast<diff_t>(endRow)));
                                auto const value = std::round((rval - valueStart) * valueScale);
                                auto const colorIndex = static_cast<size_t>(std::min(std::max(value, 0.0f), 255.0f));
                                reinterpret_cast<juce::PixelARGB*>(pixel)->set(colours[colorIndex]);
                            }
                            pixel -= lineStride;
                        }
                    }
                }
            }

            advancement = static_cast<float>(i) / static_cast<float>(imageWidth);
        }
        advancement = 1.0f;
        return predicate() ? image : juce::Image();
    };
    auto const numChannels = columns.size();

    {
        std::unique_lock<std::mutex> imageLock(mMutex);
        mImages.resize(numChannels);
    }

    auto constexpr maxImageSize = 4096;
    auto const dimension = std::max(width, height);
    if(dimension > maxImageSize)
    {
        float adv = 0.0f;
        auto const advRatio = 0.05f / static_cast<float>(numChannels);
        for(size_t channel = 0; channel < numChannels; ++channel)
        {
            auto image = createImage(channel, std::min(maxImageSize, width), std::min(maxImageSize, height), adv, [&]()
                                     {
                                         mAdvancement.store((static_cast<float>(channel) + adv) * advRatio);
                                         return mRenderingState.load() != ProcessState::aborted;
                                     });

            if(image.isValid())
            {
                std::unique_lock<std::mutex> imageLock(mMutex);
                mImages[channel].push_back(image);
                imageLock.unlock();
                triggerAsyncUpdate();
            }

            mAdvancement.store(static_cast<float>(channel + 1_z) * advRatio);
        }
        mAdvancement.store(0.05f);
    }

    std::vector<juce::Image> tempImages;
    {
        auto const currentAdv = mAdvancement.load();
        auto const advRatio = (0.92f - currentAdv) / static_cast<float>(numChannels);
        float adv = 0.0f;
        for(size_t channel = 0; channel < numChannels; ++channel)
        {
            auto image = createImage(channel, width, height, adv, [&]()
                                     {
                                         mAdvancement.store(currentAdv + (static_cast<float>(channel) + adv) * advRatio);
                                         return mRenderingState.load() != ProcessState::aborted;
                                     });

            tempImages.push_back(image);
            mAdvancement.store(currentAdv + static_cast<float>(channel + 1_z) * advRatio);
        }
        mAdvancement.store(0.92f);
    }

    auto const currentAdv = mAdvancement.load();
    auto const advRatio = (1.0f - currentAdv) / static_cast<float>(numChannels);
    auto const numRescales = static_cast<int>(std::log2(std::max(dimension - std::min(maxImageSize, dimension), 1)));
    for(size_t channel = 0; channel < numChannels; ++channel)
    {
        auto const& image = tempImages[channel];
        if(image.isValid())
        {
            int rescaleIndex = 0;
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
                auto const adv = static_cast<float>(++rescaleIndex) / static_cast<float>(numRescales);
                mAdvancement.store(currentAdv + (static_cast<float>(channel) + adv) * advRatio);
            }

            std::unique_lock<std::mutex> imageLock(mMutex);
            mImages[channel].push_back(image);
            imageLock.unlock();
        }
        mAdvancement.store(currentAdv + (static_cast<float>(channel) + 1.0f) * advRatio);
    }

    mAdvancement.store(1.0f);
    expected = ProcessState::running;
    mRenderingState.compare_exchange_weak(expected, ProcessState::ended);
    triggerAsyncUpdate();
}

ANALYSE_FILE_END
