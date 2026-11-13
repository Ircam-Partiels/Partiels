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

        //! @brief Asks the user to apply or discard the pending changes before closing.
        //! @details If there are no pending changes, the callback is called immediately with true.
        //! Otherwise, the user can apply or discard the changes (true) or cancel the closing (false).
        //! @param callback The function called with true if the content can be closed or false otherwise.
        void askToClose(std::function<void(bool)> callback);

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
