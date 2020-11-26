#include "AnlDocumentDirector.h"
#include "AnlDocumentAudioReader.h"

ANALYSE_FILE_BEGIN

Document::Director::Director(Accessor& accessor, juce::AudioFormatManager const& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
{
}

Document::Director::~Director()
{
}

void Document::Director::loadAudioFile(juce::File const& file, AlertType alertType)
{
    auto const fileExtension = file.getFileExtension();
    if(mAudioFormatManager.getWildcardForAllFormats().contains(fileExtension))
    {
        mAccessor.setAttr<AttrType::file>(file, NotificationType::synchronous);
        auto reader = createAudioFormatReader(mAccessor, mAudioFormatManager, alertType);
        if(reader != nullptr)
        {
            auto const sampleRate = reader->sampleRate;
            auto const duration = sampleRate > 0.0 ? static_cast<double>(reader->lengthInSamples) / sampleRate : 0.0;
            Zoom::range_type const range = {0.0, duration};
            auto& zoomAcsr = mAccessor.getAccessor<AttrType::timeZoom>(0);
            zoomAcsr.setAttr<Zoom::AttrType::globalRange>(range, NotificationType::synchronous);
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
        }
    }
    else if(alertType == AlertType::window)
    {
        auto constexpr icon = juce::AlertWindow::AlertIconType::WarningIcon;
        auto const title = juce::translate("File format unsupported");
        auto const message = juce::translate("The file extension FLEX is not supported by the audio format manager.").replace("FLEX", fileExtension);
        juce::AlertWindow::showMessageBox(icon, title, message);
    }
}

ANALYSE_FILE_END
