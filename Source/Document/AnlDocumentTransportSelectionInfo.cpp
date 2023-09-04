#include "AnlDocumentTransportSelectionInfo.h"

ANALYSE_FILE_BEGIN

Document::TransportSelectionInfo::TransportSelectionInfo(Transport::Accessor& accessor, Zoom::Accessor& zoomAcsr)
: mTransportAccessor(accessor)
, mZoomAccessor(zoomAcsr)
, mStart(juce::translate("Start"), juce::translate("The start position of the time selection"), [this](double time)
         {
             mTransportAccessor.setAttr<Transport::AttrType::selection>(mTransportAccessor.getAttr<Transport::AttrType::selection>().withStart(time), NotificationType::synchronous);
         })
, mEnd(juce::translate("End"), juce::translate("The end position of the time selection"), [this](double time)
       {
           mTransportAccessor.setAttr<Transport::AttrType::selection>(mTransportAccessor.getAttr<Transport::AttrType::selection>().withEnd(time), NotificationType::synchronous);
       })
, mLength(juce::translate("Length"), juce::translate("The length of the time selection"), [this](double time)
          {
              mTransportAccessor.setAttr<Transport::AttrType::selection>(mTransportAccessor.getAttr<Transport::AttrType::selection>().withLength(time), NotificationType::synchronous);
          })
{
    mTransportListener.onAttrChanged = [&]([[maybe_unused]] Transport::Accessor const& acsr, Transport::AttrType attribute)
    {
        switch(attribute)
        {
            case Transport::AttrType::playback:
            case Transport::AttrType::startPlayhead:
            case Transport::AttrType::runningPlayhead:
            case Transport::AttrType::looping:
            case Transport::AttrType::loopRange:
            case Transport::AttrType::gain:
            case Transport::AttrType::stopAtLoopEnd:
            case Transport::AttrType::autoScroll:
            case Transport::AttrType::markers:
            case Transport::AttrType::magnetize:
                break;
            case Transport::AttrType::selection:
            {
                updated();
            }
            break;
        }
    };

    mZoomListener.onAttrChanged = [this]([[maybe_unused]] Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            {
                updated();
            }
            break;
            case Zoom::AttrType::minimumLength:
            case Zoom::AttrType::visibleRange:
            case Zoom::AttrType::anchor:
                break;
        }
    };

    addAndMakeVisible(mStart);
    addAndMakeVisible(mEnd);
    addAndMakeVisible(mLength);

    mTransportAccessor.addListener(mTransportListener, NotificationType::synchronous);
    mZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
}

Document::TransportSelectionInfo::~TransportSelectionInfo()
{
    mZoomAccessor.removeListener(mZoomListener);
    mTransportAccessor.removeListener(mTransportListener);
}

void Document::TransportSelectionInfo::resized()
{
    auto bounds = getLocalBounds();
    auto const height = bounds.getHeight() / 3;
    mLength.setBounds(bounds.removeFromBottom(height));
    mEnd.setBounds(bounds.removeFromBottom(height));
    mStart.setBounds(bounds.removeFromBottom(height));
}

void Document::TransportSelectionInfo::updated()
{
    auto const globalRange = mZoomAccessor.getAttr<Zoom::AttrType::globalRange>();
    auto const selectionRange = mTransportAccessor.getAttr<Transport::AttrType::selection>();
    mStart.entry.setRange(globalRange, juce::NotificationType::dontSendNotification);
    mStart.entry.setTime(selectionRange.getStart(), juce::NotificationType::dontSendNotification);
    mEnd.entry.setRange({selectionRange.getStart(), globalRange.getEnd()}, juce::NotificationType::dontSendNotification);
    mEnd.entry.setTime(selectionRange.getEnd(), juce::NotificationType::dontSendNotification);
    mLength.entry.setRange({0.0, globalRange.getEnd() - selectionRange.getStart()}, juce::NotificationType::dontSendNotification);
    mLength.entry.setTime(selectionRange.getLength(), juce::NotificationType::dontSendNotification);
}

ANALYSE_FILE_END
