#pragma once

#include "AnlDocumentModel.h"
#include "../Layout/AnlLayout.h"
#include "../Plugin/AnlPluginListTable.h"
#include "../Analyzer/AnlAnalyzerThumbnail.h"
#include "../Analyzer/AnlAnalyzerProcessor.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class ControlPanel
    : public juce::Component
    {
    public:
        ControlPanel(Accessor& accessor, PluginList::Accessor& pluginListAccessor, juce::AudioFormatManager const& audioFormatManager);
        ~ControlPanel() override;
        
        void resized() override;
    private:
        
        Accessor& mAccessor;
        juce::AudioFormatManager const& mAudioFormatManager;
        Accessor::Listener mListener;
        PluginList::Table mPluginListTable;
        
        struct Section
        {
            Section(Analyzer::Accessor& acsr, std::function<void(Analyzer::Accessor&)> fn)
            : accessor(acsr)
            , onKeyChanged(fn)
            {
                listener.onChanged = [&](Analyzer::Accessor const& acsr, Analyzer::AttrType attribute)
                {
                    if(attribute == Analyzer::AttrType::key && acsr.getAttr<Analyzer::AttrType::key>() != "")
                    {
                        
                        juce::MessageManager::callAsync([&]()
                        {
                            if(onKeyChanged)
                            {
                                onKeyChanged(accessor);
                            }
                        });
                        
                    }
                };
                accessor.addListener(listener, NotificationType::synchronous);
            }
            
            ~Section()
            {
                accessor.removeListener(listener);
            }
            
            Analyzer::Accessor& accessor;
            Analyzer::Accessor::Listener listener;
            Analyzer::Thumbnail thumbnail {accessor};
            Tools::ColouredPanel separator;
            
            std::function<void(Analyzer::Accessor&)> onKeyChanged = nullptr;
        };
        
        std::vector<std::unique_ptr<Section>> mSections;
        juce::TextButton mAddButton {juce::translate("+")};
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlPanel)
    };
}

ANALYSE_FILE_END
