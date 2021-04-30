#include "AnlDocumentFileWatcher.h"
#include "AnlDocumentAudioReader.h"

ANALYSE_FILE_BEGIN

Document::FileWatcher::FileWatcher(Accessor& accessor, juce::AudioFormatManager const& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::file:
            {
                stopTimer();
                auto const file = acsr.getAttr<AttrType::file>();
                if(file.existsAsFile())
                {
                    mModificationTime = file.getLastModificationTime();
                    startTimer(200);
                }
            }
            break;
            case AttrType::layout:
                break;
        }
    };
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::FileWatcher::~FileWatcher()
{
    mAccessor.removeListener(mListener);
}

void Document::FileWatcher::timerCallback()
{
    auto const file = mAccessor.getAttr<AttrType::file>();
    if(!file.existsAsFile())
    {
        stopTimer();
        if(AlertWindow::showOkCancel(AlertWindow::MessageType::warning, "Audio file cannt be found!", "The audio file FILENAME has been moved or deleted. Would you like to restore  it?", {{"FILENAME", file.getFullPathName()}}))
        {
            auto const audioFormatWildcard = mAudioFormatManager.getWildcardForAllFormats();
            juce::FileChooser fc(juce::translate("Restore the audio file..."), file, audioFormatWildcard);
            if(!fc.browseForFileToOpen())
            {
                return;
            }
            mAccessor.setAttr<AttrType::file>(fc.getResult(), NotificationType::synchronous);
        }
        startTimer(200);
    }
    auto const time = file.getLastModificationTime();
    if(time != mModificationTime)
    {
        mModificationTime = time;
        stopTimer();
        if(AlertWindow::showOkCancel(AlertWindow::MessageType::warning, "Audio file  has been modified!", "The audio file FILENAME has been modified. Would you like to reload it?", {{"FILENAME", file.getFullPathName()}}))
        {
            mAccessor.setAttr<AttrType::file>(juce::File{}, NotificationType::synchronous);
            mAccessor.setAttr<AttrType::file>(file, NotificationType::synchronous);
        }
        startTimer(200);
    }
}

ANALYSE_FILE_END
