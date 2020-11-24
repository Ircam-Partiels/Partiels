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
                        // I'm noy sure that's the best place to do it
                        auto& zoomAccessor = mAccessor.getAccessor<AttrType::timeZoom>(0);
                        zoomAccessor.setValue<Zoom::AttrType::globalRange>(Zoom::range_type{0.0, audioReader->sampleRate > 0.0 ? static_cast<double>(audioReader->lengthInSamples) / audioReader->sampleRate : 0.0}, NotificationType::synchronous);
                    }
                    mModificationTime = file.getLastModificationTime();
                    startTimer(200);                    
                }
            }
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
    auto const time = file.getLastModificationTime();
    if(time > mModificationTime)
    {
        
    }
}

ANALYSE_FILE_END
