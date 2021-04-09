#pragma once

#include "AnlApplicationCommandTarget.h"
#include "../Document/AnlDocumentModel.h"
#include "../Document/AnlDocumentFileInfoPanel.h"
#include "../Document/AnlDocumentSection.h"
#include "../Transport/AnlTransportDisplay.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Interface
    : public juce::Component
    , public juce::FileDragAndDropTarget
    , public CommandTarget
    {
    public:
        Interface();
        ~Interface() override;
        
        // juce::Component
        void resized() override;
        void lookAndFeelChanged() override;
        void parentHierarchyChanged() override;
        
        // juce::FileDragAndDropTarget
        bool isInterestedInFileDrag(juce::StringArray const& files) override;
        void fileDragEnter(juce::StringArray const& files, int x, int y) override;
        void fileDragExit(juce::StringArray const& files) override;
        void filesDropped(juce::StringArray const& files, int x, int y) override;
    private:
        Document::Accessor::Listener mDocumentListener;
        
        // Header
        ResizerBar mFileInfoResizer {ResizerBar::Orientation::vertical, false, {4, 320}};
        Document::FileInfoPanel mDocumentFileInfoPanel;
        Transport::Display mTransportDisplay;
        
//        juce::ImageButton mNavigate;
//        juce::ImageButton mInspect;
//        juce::ImageButton mEdit;
        
        juce::TextButton mLoad {juce::translate("Load File")};
        
        // Main
        Document::Section mDocumentSection;
        
        //Footer
        Tooltip::Display mToolTipDisplay;
        Tooltip::BubbleWindow mToolTipBubbleWindow;
    };
}

ANALYSE_FILE_END

