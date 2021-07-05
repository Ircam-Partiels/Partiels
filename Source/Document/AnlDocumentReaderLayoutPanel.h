#pragma once

#include "AnlDocumentDirector.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class ReaderLayoutPanel
    : public FloatingWindowContainer
    , public juce::DragAndDropContainer
    {
    public:
        ReaderLayoutPanel(Director& director);
        ~ReaderLayoutPanel() override;

        // juce::Component
        void resized() override;

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};

        AudioFileLayoutTable mAudioFileLayoutTable{mDirector.getAudioFormatManager(), AudioFileLayoutTable::SupportMode::supportLayoutMono, AudioFileLayout::ChannelLayout::split};
        ColouredPanel mSeparator;
        juce::TextButton mApplyButton{juce::translate("Apply"), juce::translate("Apply the new audio reader layout to the document")};
        juce::TextButton mResetButton{juce::translate("Reset"), juce::translate("Reset to the current audio reader layout of the document")};
        ColouredPanel mInfoSeparator;
        AudioFileInfoPanel mFileInfoPanel;
        juce::TooltipWindow mTooltipWindow{this};

        JUCE_LEAK_DETECTOR(ReaderLayoutPanel)
    };
} // namespace Document

ANALYSE_FILE_END
