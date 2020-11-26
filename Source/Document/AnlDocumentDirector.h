#pragma once

#include "AnlDocumentModel.h"
#include "../Plugin/AnlPluginListTable.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Director
    : public Sanitizer
    {
    public:
        Director(Accessor& accessor, PluginList::Accessor& pluginAccessor, juce::AudioFormatManager const& audioFormatManager);
        ~Director() override = default;
        
        void loadAudioFile(juce::File const& file, AlertType alertType);
        void addAnalysis(AlertType alertType);
        
        // Sanitizer
        Zoom::range_type getGlobalRangeForFile(juce::File const& file) const;
    private:
        
        Accessor& mAccessor;
        juce::AudioFormatManager const& mAudioFormatManager;
        PluginList::Table mPluginListTable;
        juce::Component* mModalWindow = nullptr;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
}

ANALYSE_FILE_END
