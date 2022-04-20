#pragma once

#include "AnlDocumentDirector.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class ReaderLayoutPanel
    : public juce::Component
    , public juce::DragAndDropContainer
    {
    public:
        ReaderLayoutPanel(Director& director);
        ~ReaderLayoutPanel() override;

        // juce::Component
        void resized() override;

        void warnBeforeClosing();

        class WindowContainer
        : public FloatingWindowContainer
        {
        public:
            WindowContainer(ReaderLayoutPanel& readerLayoutPanel);
            ~WindowContainer() override;

        private:
            ReaderLayoutPanel& mReaderLayoutPanel;
            juce::TooltipWindow mTooltip;
        };

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};

        AudioFileLayoutTable mAudioFileLayoutTable{mDirector.getAudioFormatManager(), AudioFileLayoutTable::SupportMode::supportLayoutMono, AudioFileLayout::ChannelLayout::split};
        ColouredPanel mSeparator;
        juce::TextButton mApplyButton{juce::translate("Apply"), juce::translate("Apply the new audio files layout to the document")};
        juce::TextButton mResetButton{juce::translate("Reset"), juce::translate("Reset to the current audio files layout of the document")};
        ColouredPanel mInfoSeparator;
        AudioFileInfoPanel mFileInfoPanel;

        JUCE_LEAK_DETECTOR(ReaderLayoutPanel)
    };
} // namespace Document

ANALYSE_FILE_END
