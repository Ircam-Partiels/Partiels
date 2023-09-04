#pragma once

#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class TransportSelectionInfo
    : public juce::Component
    {
    public:
        TransportSelectionInfo(Transport::Accessor& accessor, Zoom::Accessor& zoomAcsr);
        ~TransportSelectionInfo() override;

        // juce::Component
        void resized() override;

    private:
        void updated();

        Transport::Accessor& mTransportAccessor;
        Transport::Accessor::Listener mTransportListener{typeid(*this).name()};
        Zoom::Accessor& mZoomAccessor;
        Zoom::Accessor::Listener mZoomListener{typeid(*this).name()};

        PropertyHMSmsField mStart;
        PropertyHMSmsField mEnd;
        PropertyHMSmsField mLength;
    };
} // namespace Document

ANALYSE_FILE_END
