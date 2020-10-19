#pragma once

#include "AnlApplicationHeader.h"
#include "../Model/AnlModelAnalyzer.h"
#include "../View/AnlViewAnalyzer.h"
#include "../Plugin/AnlPluginListTable.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Interface
    : public juce::Component
    , public juce::ApplicationCommandTarget
    {
    public:
        
        Interface();
        ~Interface() override = default;
        
        // juce::Component
        void resized() override;
        
        // juce::ApplicationCommandTarget
        juce::ApplicationCommandTarget* getNextCommandTarget() override;
        void getAllCommands(juce::Array<juce::CommandID>& commands) override;
        void getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result) override;
        bool perform(juce::ApplicationCommandTarget::InvocationInfo const& info) override;
        
    private:
        
        Header mHeader;
        
        Analyzer::Model mAnalyzerModel;
        Analyzer::Accessor mAnalyzerAccessor {mAnalyzerModel};
        Analyzer::View mAnalyzerView {mAnalyzerAccessor};
        
        PluginList::Table mPluginListTable;
        std::unique_ptr<juce::AudioFormatReader> mAudioFormatReader;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Interface)
    };
}

ANALYSE_FILE_END

