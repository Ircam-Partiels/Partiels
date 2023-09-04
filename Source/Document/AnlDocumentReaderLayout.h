#pragma once

#include "AnlDocumentDirector.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class ReaderLayoutContent
    : public juce::Component
    , public juce::DragAndDropContainer
    {
    public:
        ReaderLayoutContent(Director& director);
        ~ReaderLayoutContent() override;

        // juce::Component
        void resized() override;

        void warnBeforeClosing();

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};

        AudioFileLayoutTable mAudioFileLayoutTable{mDirector.getAudioFormatManager(), AudioFileLayoutTable::SupportMode::channelLayoutMono, AudioFileLayout::ChannelLayout::split};
        ColouredPanel mSeparator;
        juce::TextButton mApplyButton{juce::translate("Apply"), juce::translate("Apply the new audio files layout to the document")};
        juce::TextButton mResetButton{juce::translate("Reset"), juce::translate("Reset to the current audio files layout of the document")};
        ColouredPanel mInfoSeparator;
        AudioFileInfoPanel mFileInfoPanel;

        JUCE_LEAK_DETECTOR(ReaderLayoutContent)
    };
} // namespace Document

ANALYSE_FILE_END
