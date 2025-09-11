#include "AnlTrackPlot.h"
#include "AnlTrackRenderer.h"

ANALYSE_FILE_BEGIN

Track::Plot::Plot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
{
    setInterceptsMouseClicks(false, false);
    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::file:
            case AttrType::name:
            case AttrType::key:
            case AttrType::input:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomValueMode:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::focused:
            case AttrType::showInGroup:
            case AttrType::sendViaOsc:
            case AttrType::hasPluginColourMap:
                break;
            case AttrType::description:
            case AttrType::grid:
            case AttrType::results:
            case AttrType::edit:
            case AttrType::graphics:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::lineWidth:
            case AttrType::unit:
            case AttrType::labelLayout:
            case AttrType::extraThresholds:
            case AttrType::channelsLayout:
            case AttrType::zoomLogScale:
            case AttrType::sampleRate:
            {
                repaint();
                break;
            }
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
                break;
            }
        }
    };

    mGridListener.onAttrChanged = [this]([[maybe_unused]] Zoom::Grid::Accessor const& acsr, [[maybe_unused]] Zoom::Grid::AttrType attribute)
    {
        repaint();
    };

    setCachedComponentImage(new LowResCachedComponentImage(*this));
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::valueZoom>().addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::binZoom>().addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
    mTimeZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
}

Track::Plot::~Plot()
{
    mTimeZoomAccessor.removeListener(mZoomListener);
    mAccessor.getAcsr<AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
    mAccessor.getAcsr<AcsrType::binZoom>().removeListener(mZoomListener);
    mAccessor.getAcsr<AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
    mAccessor.getAcsr<AcsrType::valueZoom>().removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Track::Plot::paint(juce::Graphics& g)
{
    Renderer::paint(mAccessor, mTimeZoomAccessor, g, getLocalBounds(), mAccessor.getAttr<AttrType::channelsLayout>(), findColour(Decorator::ColourIds::normalBorderColourId), Zoom::Grid::Justification::none);
}

ANALYSE_FILE_END
