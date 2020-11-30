#include "AnlDocumentPlayhead.h"

ANALYSE_FILE_BEGIN

Document::Playhead::Playhead(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch (attribute)
        {
            case AttrType::playheadPosition:
            {
                repaint();
            }
                break;
            case AttrType::gain:
            case AttrType::isPlaybackStarted:
            case AttrType::isLooping:
            case AttrType::file:
            case AttrType::timeZoom:
            case AttrType::layout:
            case AttrType::layoutHorizontal:
            case AttrType::analyzers:
                break;
        }
    };
    
    mZoomListener.onChanged = [&](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::minimumLength:
                break;
            case Zoom::AttrType::visibleRange:
            {
                repaint();
            }
                break;
        }
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAccessor<AttrType::timeZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
}

Document::Playhead::~Playhead()
{
    mAccessor.getAccessor<AttrType::timeZoom>(0).removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Document::Playhead::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId, true));
    auto const time = mAccessor.getAttr<AttrType::playheadPosition>();
    auto const range = mAccessor.getAccessor<AttrType::timeZoom>(0).getAttr<Zoom::AttrType::visibleRange>();
    if(range.isEmpty())
    {
        return;
    }
    auto const position = (time - range.getStart()) / range.getLength() * static_cast<double>(getWidth());
    g.setColour(findColour(ColourIds::playheadColourId, true));
    g.drawVerticalLine(static_cast<int>(position), 0.0f, static_cast<float>(getHeight()));
}

ANALYSE_FILE_END
