#include "AnlTrackSection.h"
#include "AnlTrackExporter.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Section::Section(Director& director, juce::ApplicationCommandManager& commandManager, Zoom::Accessor& timeZoomAcsr, Transport::Accessor& transportAcsr)
: mDirector(director)
, mTimeZoomAccessor(timeZoomAcsr)
, mTransportAccessor(transportAcsr)
, mPlotOverlay(mPlot, commandManager)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType type)
    {
        switch(type)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::file:
            case AttrType::key:
            case AttrType::input:
            case AttrType::description:
            case AttrType::results:
            case AttrType::edit:
            case AttrType::state:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::unit:
            case AttrType::channelsLayout:
            case AttrType::showInGroup:
            case AttrType::zoomValueMode:
            case AttrType::zoomLink:
            case AttrType::graphics:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::zoomAcsr:
            case AttrType::extraThresholds:
            case AttrType::grid:
            case AttrType::hasPluginColourMap:
                break;
            case AttrType::height:
            {
                setSize(getWidth(), acsr.getAttr<AttrType::height>() + 1);
            }
            break;
            case AttrType::focused:
            {
                mThumbnailDecoration.setHighlighted(Tools::isSelected(acsr));
            }
            break;
        }
    };

    mResizerBar.onMoved = [&](int size)
    {
        mAccessor.setAttr<AttrType::height>(size, NotificationType::synchronous);
        if(auto* viewport = findParentComponentOfClass<juce::Viewport>())
        {
            if(viewport->canScrollVertically())
            {
                auto const point = viewport->getLocalPoint(this, juce::Point<int>{0, size + 1});
                viewport->autoScroll(point.x, point.y, 20, 10);
            }
        }
    };

    addAndMakeVisible(mScrollBar);
    addAndMakeVisible(mRulerDecoration);
    addAndMakeVisible(mThumbnailDecoration);
    addAndMakeVisible(mSnapshotDecoration);
    addAndMakeVisible(mPlotDecoration);
    addAndMakeVisible(mResizerBar);
    setSize(80, 100);
    commandManager.registerAllCommandsForTarget(std::addressof(mPlotOverlay));
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::Section::~Section()
{
    mAccessor.removeListener(mListener);
}

juce::Rectangle<int> Track::Section::getPlotBounds() const
{
    return mPlot.getBounds();
}

juce::String Track::Section::getIdentifier() const
{
    return mAccessor.getAttr<AttrType::identifier>();
}

void Track::Section::setResizable(bool state)
{
    mResizerBar.setEnabled(state);
}

void Track::Section::resized()
{
    mResizerBar.setBounds(getLocalBounds().removeFromBottom(2).reduced(2, 0));

    auto bounds = getLocalBounds();
    bounds.removeFromLeft(24);
    mThumbnailDecoration.setBounds(bounds.removeFromLeft(24));
    mSnapshotDecoration.setBounds(bounds.removeFromLeft(36));
    mScrollBar.setBounds(bounds.removeFromRight(8));
    mRulerDecoration.setBounds(bounds.removeFromRight(16));
    mPlotDecoration.setBounds(bounds);
}

void Track::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
}

ANALYSE_FILE_END
