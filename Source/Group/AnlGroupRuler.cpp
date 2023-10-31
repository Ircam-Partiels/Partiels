#include "AnlGroupRuler.h"
#include "../Track/AnlTrackRuler.h"

ANALYSE_FILE_BEGIN

Group::NavigationBar::NavigationBar(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
, mTransportAccessor(transportAccessor)
, mLayoutNotifier(mAccessor, [this]()
                  {
                      updateContent();
                  },
                  {Track::AttrType::identifier, Track::AttrType::channelsLayout, Track::AttrType::showInGroup})
{
}

void Group::NavigationBar::resized()
{
    if(mTrackNavigationBar != nullptr)
    {
        mTrackNavigationBar->setBounds(getLocalBounds());
    }
}

void Group::NavigationBar::updateContent()
{
    auto const zoomTrackAcsr = Tools::getZoomTrackAcsr(mAccessor);
    if(!zoomTrackAcsr.has_value())
    {
        mTrackNavigationBar.reset();
    }
    else if(mTrackIdentifier != zoomTrackAcsr.value().get().getAttr<Track::AttrType::identifier>())
    {
        mTrackIdentifier = zoomTrackAcsr.value().get().getAttr<Track::AttrType::identifier>();
        mTrackNavigationBar = std::make_unique<Track::NavigationBar>(zoomTrackAcsr.value().get(), mTimeZoomAccessor, mTransportAccessor);
        addAndMakeVisible(mTrackNavigationBar.get());
    }
    resized();
}

ANALYSE_FILE_END
