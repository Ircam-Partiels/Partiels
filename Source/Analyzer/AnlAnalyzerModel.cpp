#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

std::shared_ptr<juce::Image const> Analyzer::Accessor::getImage() const
{
    return mImageManager.getInstance();
}

void Analyzer::Accessor::setImage(std::shared_ptr<juce::Image> image, NotificationType notification)
{
    if(image != mImageManager.getInstance())
    {
        mImageManager.setInstance(image);
        sendSignal(SignalType::image, {static_cast<bool>(image)}, notification);
    }
}

ANALYSE_FILE_END
