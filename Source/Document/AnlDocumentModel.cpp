#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

void Document::Accessor::setSanitizer(Sanitizer* santizer)
{
    mSanitizer = santizer;
}

template <>
void Document::Accessor::setAttr<Document::AttrType::file, juce::File>(juce::File const& value, NotificationType notification)
{
    ::Anl::Model::Accessor<Accessor, Container>::setAttr<AttrType::file, juce::File>(value, notification);
    if(mSanitizer != nullptr)
    {
        auto const range = mSanitizer->getGlobalRangeForFile(value);
        auto& zoomAcsr = getAccessor<AttrType::timeZoom>(0);
        zoomAcsr.setAttr<Zoom::AttrType::globalRange>(range, NotificationType::synchronous);
    }
}

ANALYSE_FILE_END
