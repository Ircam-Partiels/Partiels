#include "AnlGroupPlot.h"
#include "../Track/AnlTrackRenderer.h"
#include "../Track/AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Group::Plot::Plot(Accessor& accessor, Zoom::Accessor& timeZoomAcsr)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAcsr)
, mLayoutNotifier(accessor, [this]()
                  {
                      updateContent();
                  })
{
    mTrackListener.onAttrChanged = [this]([[maybe_unused]] Track::Accessor const& acsr, Track::AttrType attribute)
    {
        switch(attribute)
        {
            case Track::AttrType::identifier:
            case Track::AttrType::name:
            case Track::AttrType::file:
            case Track::AttrType::fileDescription:
            case Track::AttrType::key:
            case Track::AttrType::input:
            case Track::AttrType::description:
            case Track::AttrType::state:
            case Track::AttrType::height:
            case Track::AttrType::zoomValueMode:
            case Track::AttrType::zoomLink:
            case Track::AttrType::zoomAcsr:
            case Track::AttrType::warnings:
            case Track::AttrType::processing:
            case Track::AttrType::focused:
            case Track::AttrType::hasPluginColourMap:
            case Track::AttrType::oscIdentifier:
            case Track::AttrType::sendViaOsc:
                break;
            case Track::AttrType::grid:
            case Track::AttrType::results:
            case Track::AttrType::edit:
            case Track::AttrType::graphics:
            case Track::AttrType::graphicsSettings:
            case Track::AttrType::channelsLayout:
            case Track::AttrType::showInGroup:
            case Track::AttrType::extraThresholds:
            case Track::AttrType::zoomLogScale:
            case Track::AttrType::sampleRate:
            {
                repaint();
            }
            break;
        }
    };

    mZoomListener.onAttrChanged = [this]([[maybe_unused]] Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::minimumLength:
            case Zoom::AttrType::anchor:
                break;
            case Zoom::AttrType::visibleRange:
            {
                repaint();
            }
            break;
        }
    };

    mGridListener.onAttrChanged = [this]([[maybe_unused]] Zoom::Grid::Accessor const& acsr, [[maybe_unused]] Zoom::Grid::AttrType attribute)
    {
        repaint();
    };

    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::height:
            case AttrType::colour:
            case AttrType::expanded:
            case AttrType::layout:
            case AttrType::tracks:
            case AttrType::focused:
                break;
            case AttrType::referenceid:
            {
                repaint();
            }
            break;
        }
    };

    setInterceptsMouseClicks(false, false);
    setCachedComponentImage(new LowResCachedComponentImage(*this));
    setSize(100, 80);
    mTimeZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::Plot::~Plot()
{
    mAccessor.removeListener(mListener);
    mTimeZoomAccessor.removeListener(mZoomListener);
    for(auto& trackAcsr : mTrackAccessors.getContents())
    {
        trackAcsr.second.get().getAcsr<Track::AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
        trackAcsr.second.get().getAcsr<Track::AcsrType::binZoom>().removeListener(mZoomListener);
        trackAcsr.second.get().getAcsr<Track::AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
        trackAcsr.second.get().getAcsr<Track::AcsrType::valueZoom>().removeListener(mZoomListener);
        trackAcsr.second.get().removeListener(mTrackListener);
    }
}

void Group::Plot::paint(juce::Graphics& g)
{
    auto const bounds = getLocalBounds();
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    auto const referenceTrackAcsr = Tools::getReferenceTrackAcsr(mAccessor);
    for(auto it = layout.crbegin(); it != layout.crend(); ++it)
    {
        auto const trackAcsr = Tools::getTrackAcsr(mAccessor, *it);
        if(trackAcsr.has_value() && trackAcsr.value().get().getAttr<Track::AttrType::showInGroup>())
        {
            auto const isSelected = referenceTrackAcsr.has_value() && std::addressof(trackAcsr.value().get()) == std::addressof(referenceTrackAcsr.value().get());
            auto const colour = isSelected ? findColour(Decorator::ColourIds::normalBorderColourId) : juce::Colours::transparentBlack;
            Track::Renderer::paint(trackAcsr.value().get(), mTimeZoomAccessor, g, bounds, trackAcsr.value().get().getAttr<Track::AttrType::channelsLayout>(), colour, Zoom::Grid::Justification::none);
        }
    }
}

void Group::Plot::updateContent()
{
    mTrackAccessors.updateContents(
        mAccessor,
        [this](Track::Accessor& trackAccessor)
        {
            trackAccessor.addListener(mTrackListener, NotificationType::synchronous);
            trackAccessor.getAcsr<Track::AcsrType::valueZoom>().addListener(mZoomListener, NotificationType::synchronous);
            trackAccessor.getAcsr<Track::AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
            trackAccessor.getAcsr<Track::AcsrType::binZoom>().addListener(mZoomListener, NotificationType::synchronous);
            trackAccessor.getAcsr<Track::AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
            return std::ref(trackAccessor);
        },
        [this](std::reference_wrapper<Track::Accessor>& content)
        {
            content.get().getAcsr<Track::AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
            content.get().getAcsr<Track::AcsrType::binZoom>().removeListener(mZoomListener);
            content.get().getAcsr<Track::AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
            content.get().getAcsr<Track::AcsrType::valueZoom>().removeListener(mZoomListener);
            content.get().removeListener(mTrackListener);
        });
    repaint();
}

ANALYSE_FILE_END
