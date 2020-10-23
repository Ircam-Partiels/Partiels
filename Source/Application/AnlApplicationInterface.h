#pragma once

#include "../Analyzer/AnlAnalyzerProcessor.h"
#include "../Plugin/AnlPluginListTable.h"
#include "../Document/AnlDocumentModel.h"
#include "../Document/AnlDocumentTransport.h"
#include "../Document/AnlDocumentFileInfoPanel.h"

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
        
        Document::Transport mDocumentTransport;
        Tools::ColouredPanel mDocumentTransportSeparator;
        Document::FileInfoPanel mDocumentFileInfoPanel;
        Tools::ColouredPanel mHeaderSeparator;
        
        Analyzer::Model mAnalyzerModel;
        Analyzer::Accessor mAnalyzerAccessor {mAnalyzerModel};
        Analyzer::Processor mAnalyzerProcessor {mAnalyzerAccessor};
        JUCE_COMPILER_WARNING("remove from here");
        
        PluginList::Table mPluginListTable;
        std::unique_ptr<juce::AudioFormatReader> mAudioFormatReader;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Interface)
    };
}

ANALYSE_FILE_END

