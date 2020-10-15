#include "AnlViewAnalyzer.h"

#include <vamp-hostsdk/PluginLoader.h>
#include <vamp-hostsdk/PluginHostAdapter.h>

ANALYSE_FILE_BEGIN

class Analyzer::View::Impl
: public Accessor::Listener
{
public:
    Impl(Accessor& accessor)
    : mAccessor(accessor)
    {
        onChanged = [&](Accessor& acsr, Attribute attr)
        {
            switch (attr)
            {
                case Attribute::key:
                {
                    auto const pluginKey = acsr.getModel().key;
                    if(pluginKey.isEmpty())
                    {
                        setPluginContainer({nullptr});
                        return;
                    }
                }
                    break;
                case Attribute::parameters:
                    std::cout << "parameters: ";
                    for(auto const& param : acsr.getModel().parameters)
                    {
                        std::cout << "[" << param.first << " " << param.second << "] ";
                    }
                    std::cout << "\n";
                    break;
                case Attribute::programName:
                    std::cout << "program: " << acsr.getModel().programName << "\n";
                    break;
                case Attribute::resultFile:
                    std::cout << "result file: " << acsr.getModel().resultFile.getFullPathName() << "\n";
                    break;
                case Attribute::projectFile:
                    std::cout << "project file: " << acsr.getModel().projectFile.getFullPathName() << "\n";
                    break;
            }
        };
        mAccessor.addListener(*this, juce::NotificationType::sendNotificationSync);
    }
    
    ~Impl()
    {
        mAccessor.removeListener(*this);
        deletePluginContainer({nullptr});
    }
    
    bool initialize(double sampleRate, size_t numChannels, size_t stepSize, size_t blockSize)
    {
        using namespace Vamp;
        using namespace Vamp::HostExt;
        
        using AlertIconType = juce::AlertWindow::AlertIconType;
        auto const errorMessage = juce::translate("Plugin cannot be loaded!");
        
        auto const pluginKey = mAccessor.getModel().key;
        auto* pluginLoader = PluginLoader::getInstance();
        anlWeakAssert(pluginLoader != nullptr);
        if(pluginLoader == nullptr)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, juce::translate("The plugin PLGNKEY cannot be loaded because the plugin manager is not available.").replace("PLGNKEY", pluginKey));
            return false;
        }
        
        auto pluginInstance = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(pluginKey.toStdString(), 48000, PluginLoader::ADAPT_ALL));
        if(pluginInstance == nullptr)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, juce::translate("The plugin PLGNKEY cannot be loaded because the plugin key is invalid.").replace("PLGNKEY", pluginKey));
            return false;
        }
        
        auto pluginContainer = std::make_shared<PluginContainer>(std::move(pluginInstance));
        if(pluginContainer == nullptr)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, juce::translate("The plugin PLGNKEY cannot be loaded because the plugin container cannot be allocated.").replace("PLGNKEY", pluginKey));
            return false;
        }
        
        setPluginContainer(pluginContainer);
        if(pluginContainer != nullptr)
        {
            return pluginContainer->initialize(sampleRate, numChannels, stepSize, blockSize);
        }
        return false;
    }
    
    Vamp::Plugin::FeatureSet process(juce::AudioBuffer<float> const& inputBuffer, long const timeStamp)
    {
        auto pluginContainer = getPluginContainer();
        if(pluginContainer != nullptr)
        {
            return pluginContainer->process(inputBuffer, timeStamp);
        }
        return {};
    }
    
    Vamp::Plugin::FeatureSet getRemainingFeatures()
    {
        auto pluginContainer = getPluginContainer();
        if(pluginContainer != nullptr)
        {
            return pluginContainer->getRemainingFeatures();
        }
        return {};
    }
    
private:
    
    struct PluginContainer
    {
        PluginContainer(std::unique_ptr<Vamp::Plugin> istance)
        : mInstance(std::move(istance))
        {
            anlStrongAssert(mInstance != nullptr);
        }
        
        ~PluginContainer() = default;
        
        bool initialize(double sampleRate, size_t numChannels, size_t stepSize, size_t blockSize)
        {
            if(mInstance == nullptr)
            {
                return false;
            }
            mSampleRate = 0;
            auto const result = mInstance->initialise(numChannels, stepSize, blockSize);
            anlStrongAssert(numChannels != 0 && sampleRate > 0.0);
            if(result && sampleRate > 0.0)
            {
                mSampleRate = static_cast<unsigned int>(sampleRate);
            }
            return mSampleRate != 0;
        }
        
        Vamp::Plugin::FeatureSet process(juce::AudioBuffer<float> const& inputBuffer, long const timeStamp)
        {
            anlStrongAssert(mInstance != nullptr && mSampleRate != 0);
            if(mInstance == nullptr || mSampleRate == 0)
            {
                return {};
            }
            return mInstance->process(inputBuffer.getArrayOfReadPointers(), Vamp::RealTime::frame2RealTime(timeStamp, mSampleRate));
        }
        
        Vamp::Plugin::FeatureSet getRemainingFeatures()
        {
            anlStrongAssert(mInstance != nullptr && mSampleRate != 0);
            if(mInstance == nullptr || mSampleRate == 0)
            {
                return {};
            }
            return mInstance->getRemainingFeatures();
        }
        
    private:
        std::unique_ptr<Vamp::Plugin> mInstance {nullptr};
        unsigned int mSampleRate {0};
    };
    
    std::shared_ptr<PluginContainer> getPluginContainer()
    {
        return std::atomic_load(&mPluginContainer);
    }
    
    void setPluginContainer(std::shared_ptr<PluginContainer> instance)
    {
        anlStrongAssert(instance == nullptr || instance != getPluginContainer());
        if(instance != getPluginContainer())
        {
            deletePluginContainer(std::atomic_exchange(&mPluginContainer, instance));
        }
    }
    
    static void deletePluginContainer(std::shared_ptr<PluginContainer> instance)
    {
        if(instance != nullptr && instance.use_count() > 1)
        {
            juce::MessageManager::callAsync([ptr = std::move(instance)]() mutable
            {
                deletePluginContainer(std::move(ptr));
                ilfStrongAssert(ptr == nullptr);
            });
            ilfStrongAssert(instance == nullptr);
        }
    }
    
    Accessor& mAccessor;
    std::shared_ptr<PluginContainer> mPluginContainer {nullptr};
};

Analyzer::View::View(Accessor& accessor)
: mImpl(std::make_unique<Impl>(accessor))
{
    anlStrongAssert(mImpl != nullptr);
}

Analyzer::View::~View()
{
}

void Analyzer::View::perform(juce::AudioFormatReader& audioFormatReader, size_t blockSize)
{
    anlStrongAssert(mImpl != nullptr);
    if(mImpl == nullptr)
    {
        return;
    }
    
    auto printResult = [](Vamp::Plugin::FeatureSet const& results)
    {
        for(auto const& channel : results)
        {
            std::cout << "Channel " << channel.first << " - ";
            for(auto const& feature : channel.second)
            {
                std::cout << feature.label << ": ";
                for(auto const& value : feature.values)
                {
                    std::cout << value << " ";
                }
            }
            std::cout << "\n";
        }
    };
    
    auto const numChannels = static_cast<int>(audioFormatReader.numChannels);
    auto const lengthInSamples = audioFormatReader.lengthInSamples;
    
    juce::AudioBuffer<float> buffer(numChannels, blockSize);
    mImpl->initialize(audioFormatReader.sampleRate, static_cast<size_t>(numChannels), blockSize, blockSize);
    for(juce::int64 timeStamp = 0; timeStamp < lengthInSamples; timeStamp += blockSize)
    {
        auto const remaininSamples = std::min(lengthInSamples - timeStamp, static_cast<juce::int64>(blockSize));
        audioFormatReader.read(buffer.getArrayOfWritePointers(), numChannels, timeStamp, static_cast<int>(remaininSamples));
        auto const result = mImpl->process(buffer, static_cast<long>(timeStamp));
        printResult(result);
    }
    printResult(mImpl->getRemainingFeatures());
}

ANALYSE_FILE_END
