#pragma once

#include "../Document/AnlDocumentFileInfoPanel.h"
#include "../Document/AnlDocumentModel.h"
#include "../Document/AnlDocumentSection.h"
#include "../Transport/AnlTransportDisplay.h"
#include "AnlApplicationCommandTarget.h"

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

        void moveKeyboardFocusTo(juce::String const& identifier);

        // juce::Component
        void resized() override;

        // juce::FileDragAndDropTarget
        bool isInterestedInFileDrag(juce::StringArray const& files) override;
        void fileDragEnter(juce::StringArray const& files, int x, int y) override;
        void fileDragExit(juce::StringArray const& files) override;
        void filesDropped(juce::StringArray const& files, int x, int y) override;

    private:
        Document::Accessor::Listener mDocumentListener;

        // Header
        Transport::Display mTransportDisplay;
        juce::TextButton mLoad{juce::translate("Load File")};

        // Main
        Document::Section mDocumentSection;

        //Footer
        Tooltip::Display mToolTipDisplay;
    };
} // namespace Application

ANALYSE_FILE_END
