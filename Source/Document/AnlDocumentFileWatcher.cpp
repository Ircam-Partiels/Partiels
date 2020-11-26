#include "AnlDocumentFileWatcher.h"
#include "AnlDocumentAudioReader.h"

ANALYSE_FILE_BEGIN

Document::FileWatcher::FileWatcher(Accessor& accessor, juce::AudioFormatManager const& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
{
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch (attribute)
        {
            case AttrType::file:
            {
                stopTimer();
                auto const file = mAccessor.getAttr<AttrType::file>();
                if(file != juce::File())
                {
                    auto audioReader = createAudioFormatReader(acsr, mAudioFormatManager, false);
                    if(audioReader != nullptr)
                    {
                        JUCE_COMPILER_WARNING("I'm not sure that's the best place to do it");
                        auto& zoomAccessor = mAccessor.getAccessor<AttrType::timeZoom>(0);
                        auto const durationInMs = audioReader->sampleRate > 0.0 ? static_cast<double>(audioReader->lengthInSamples) / audioReader->sampleRate : 0.0;
                        zoomAccessor.setAttr<Zoom::AttrType::globalRange>(Zoom::range_type{0.0, durationInMs}, NotificationType::synchronous);
                    }
                    mModificationTime = file.getLastModificationTime();
                    startTimer(200);                    
                }
            }
                break;
            case isLooping:
            case gain:
            case isPlaybackStarted:
            case playheadPosition:
            case timeZoom:
            case layout:
            case analyzers:
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
        auto constexpr icon = juce::AlertWindow::AlertIconType::WarningIcon;
        auto const title = juce::translate("Audio file has been moved or deleted!");
        auto const message = juce::translate("The audio file FLNM has been moved or deleted. Would you like to restore  it?").replace("FLNM", file.getFullPathName());
        if(juce::AlertWindow::showOkCancelBox(icon, title, message))
        {
            auto const audioFormatWildcard = mAudioFormatManager.getWildcardForAllFormats();
            juce::FileChooser fc(juce::translate("Restore the audio file..."), file, audioFormatWildcard);
            if(!fc.browseForFileToOpen())
            {
                return;
            }
            mAccessor.setAttr<AttrType::file>(fc.getResult(), NotificationType::synchronous);
        }
    }
    auto const time = file.getLastModificationTime();
    if(time > mModificationTime)
    {
        auto constexpr icon = juce::AlertWindow::AlertIconType::WarningIcon;
        auto const title = juce::translate("Audio file  has been modified!");
        auto const message = juce::translate("The audio file FLNM has been modified. Would you like to reload it?").replace("FLNM", file.getFullPathName());
        if(juce::AlertWindow::showOkCancelBox(icon, title, message))
        {
            JUCE_COMPILER_WARNING("What to do?")
        }
    }
}

ANALYSE_FILE_END
