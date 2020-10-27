#pragma once

#include "AnlDocumentModel.h"
#include "../Plugin/AnlPluginListTable.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class AnalyzerPanel
    : public juce::Component
    {
    public:
        using Attribute = Model::Attribute;
        using Signal = Model::Signal;
        
        AnalyzerPanel(Accessor& accessor, PluginList::Accessor& pluginListAccessor);
        ~AnalyzerPanel() override;
        
        void resized() override;
        void mouseDoubleClick(juce::MouseEvent const& event) override;
    private:
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        PluginList::Table mPluginListTable;
        struct Section
        {
            juce::TextButton close {juce::CharPointer_UTF8("×")};
            juce::Label name;
            juce::Label results;
        };
        
        std::vector<std::unique_ptr<Section>> mSections;
        juce::DialogWindow* dialogWindow;
        
        
//        Analyzer::Model mAnalyzerModel;
//        Analyzer::Accessor mAnalyzerAccessor {mAnalyzerModel};
//        Analyzer::Processor mAnalyzerProcessor {mAnalyzerAccessor};
//        JUCE_COMPILER_WARNING("remove from here");
        
        JUCE_LEAK_DETECTOR(AnalyzerPanel)
    };
}

ANALYSE_FILE_END
