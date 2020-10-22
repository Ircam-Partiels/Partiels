#pragma once

#include "AnlApplicationHeader.h"
#include "../Analyzer/AnlAnalyzerProcessor.h"
#include "../Plugin/AnlPluginListTable.h"
#include "../Document/AnlDocumentModel.h"
#include "../Document/AnlDocumentFileInfoPanel.h"

#include "../Document/AnlDocumentAudioReader.h"

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
        
        Document::AudioReader& getDocumentAudioReader() { return mDocumentAudioReader; }
    private:
        
        Header mHeader;
        
        Document::Model mDocumentModel;
        Document::Accessor mDocumentAccessor {mDocumentModel};
        Document::FileInfoPanel mDocumentFileInfoPanel;
        
        Document::AudioReader mDocumentAudioReader;
        JUCE_COMPILER_WARNING("don't put that here!")
        
        Analyzer::Model mAnalyzerModel;
        Analyzer::Accessor mAnalyzerAccessor {mAnalyzerModel};
        Analyzer::Processor mAnalyzerProcessor {mAnalyzerAccessor};
        
        PluginList::Table mPluginListTable;
        std::unique_ptr<juce::AudioFormatReader> mAudioFormatReader;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Interface)
    };
}

ANALYSE_FILE_END

