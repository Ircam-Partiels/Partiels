#include "AnlDocumentControlPanel.h"
#include "../Plugin/AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

Document::ControlPanel::ControlPanel(Accessor& accessor, PluginList::Accessor& pluginListAccessor, juce::AudioFormatManager& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
, mPluginListTable(pluginListAccessor)
{
    mPluginListTable.onPluginSelected = [&](juce::String key)
    {
        if(dialogWindow != nullptr)
        {
            dialogWindow->exitModalState(0);
        }
        
        auto const& analyzers = mAccessor.getValue<AttrType::analyzers>();
        using container_type = std::remove_const<std::remove_reference<decltype(analyzers)>::type>::type;
        using pointer_type = container_type::value_type;
        using value_type = pointer_type::element_type;
        container_type copy;
        for(auto const& analyzer : analyzers)
        {
            auto value = std::make_unique<value_type>(analyzer != nullptr ? *analyzer.get() : value_type{});
            if(value != nullptr)
            {
                copy.push_back(std::move(value));
            }
        }
        auto container = std::make_unique<value_type>(value_type{{key}, {""}, {44100.0}, {0ul}, {{}}});
        anlWeakAssert(container != nullptr);
        if(container != nullptr)
        {
            copy.push_back(std::move(container));
            mAccessor.setValue<AttrType::analyzers>(copy);
        }
    };
    
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch (attribute)
        {
            case AttrType::analyzers:
            {
                auto const& analyzers = mAccessor.getValue<AttrType::analyzers>();
                for(size_t i = mSections.size(); i < analyzers.size(); ++i)
                {
                    auto& anlAcsr = mAccessor.getAccessor<AttrType::analyzers>(i);
                    auto section = std::make_unique<Section>(anlAcsr);
                    anlWeakAssert(section != nullptr);
                    if(section != nullptr)
                    {
                        addAndMakeVisible(section->thumbnail);
                        addAndMakeVisible(section->separator);
                        mSections.push_back(std::move(section));
                    }
                }
                mSections.resize(analyzers.size());
                for(size_t i = 0; i < mSections.size(); ++i)
                {
                    if(mSections[i] != nullptr)
                    {
                        mSections[i]->thumbnail.onRemove = [this, i]()
                        {
                            auto const& lanalyzers = mAccessor.getValue<AttrType::analyzers>();
                            using container_type = std::remove_const<std::remove_reference<decltype(analyzers)>::type>::type;
                            using pointer_type = container_type::value_type;
                            using value_type = pointer_type::element_type;
                            container_type copy;
                            for(auto const& analyzer : lanalyzers)
                            {
                                auto value = std::make_unique<value_type>(analyzer != nullptr ? *analyzer.get() : value_type{});
                                if(value != nullptr)
                                {
                                    copy.push_back(std::move(value));
                                }
                            }
                            copy.erase(copy.begin() + static_cast<long>(i));
                            mAccessor.setValue<AttrType::analyzers>(copy, NotificationType::synchronous);
                        };
                        
                        mSections[i]->thumbnail.onRelaunch = [this, i]()
                        {
                            auto const file = mAccessor.getValue<AttrType::file>();
                            auto* audioFormat = mAudioFormatManager.findFormatForFileExtension(file.getFileExtension());
                            if(audioFormat == nullptr)
                            {
                                return;
                            }
                            auto audioFormatReader = std::unique_ptr<juce::AudioFormatReader>(audioFormat->createReaderFor(file.createInputStream().release(), true));
                            if(audioFormatReader == nullptr)
                            {
                                return;
                            }
                            
                            mSections[i]->processor.perform(*audioFormatReader.get());
                        };
                    }
                }
                resized();
            }
                break;
        }
    };
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::ControlPanel::~ControlPanel()
{
    mAccessor.removeListener(mListener);
}

void Document::ControlPanel::resized()
{
    auto constexpr size = 100;
    auto bounds = getLocalBounds();
    for(auto& section : mSections)
    {
        if(section != nullptr)
        {
            auto lbounds = bounds.removeFromTop(size);
            section->separator.setBounds(lbounds.removeFromBottom(2));
            section->thumbnail.setBounds(lbounds);
        }
    }
}

void Document::ControlPanel::mouseDoubleClick(juce::MouseEvent const& event)
{
    if(!isEnabled())
    {
        return;
    }
    juce::ignoreUnused(event);
    juce::DialogWindow::LaunchOptions launchOption;
    launchOption.dialogTitle = juce::translate("New Analyzer...");
    launchOption.content.setNonOwned(&mPluginListTable);
    launchOption.componentToCentreAround = this;
    launchOption.dialogBackgroundColour = findColour(juce::ResizableWindow::backgroundColourId, true);
    launchOption.escapeKeyTriggersCloseButton = true;
    launchOption.useNativeTitleBar = false;
    launchOption.resizable = false;
    launchOption.useBottomRightCornerResizer = false;
    dialogWindow = launchOption.launchAsync();
    dialogWindow->runModalLoop();
}

ANALYSE_FILE_END
