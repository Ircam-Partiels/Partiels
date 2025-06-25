#include "AnlTrackSection.h"
#include "AnlTrackExporter.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Section::Section(Director& director, juce::ApplicationCommandManager& commandManager, Zoom::Accessor& timeZoomAcsr, Transport::Accessor& transportAcsr, ResizerFn resizerFn)
: mDirector(director)
, mTimeZoomAccessor(timeZoomAcsr)
, mTransportAccessor(transportAcsr)
, mPlot(mDirector.getAccessor(), mTimeZoomAccessor)
, mEditor(mDirector, mTimeZoomAccessor, mTransportAccessor, commandManager, mPlot, nullptr, true)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType type)
    {
        switch(type)
        {
            case AttrType::identifier:
            case AttrType::file:
            case AttrType::key:
            case AttrType::input:
            case AttrType::description:
            case AttrType::results:
            case AttrType::edit:
            case AttrType::state:
            case AttrType::font:
            case AttrType::lineWidth:
            case AttrType::unit:
            case AttrType::labelLayout:
            case AttrType::channelsLayout:
            case AttrType::showInGroup:
            case AttrType::sendViaOsc:
            case AttrType::zoomValueMode:
            case AttrType::zoomLink:
            case AttrType::graphics:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::zoomAcsr:
            case AttrType::extraThresholds:
            case AttrType::grid:
            case AttrType::hasPluginColourMap:
            case AttrType::sampleRate:
            case AttrType::zoomLogScale:
            case AttrType::name:
            case AttrType::colours:
                break;
            case AttrType::height:
            {
                setSize(getWidth(), acsr.getAttr<AttrType::height>());
                break;
            }
            case AttrType::focused:
            {
                mThumbnailDecoration.setHighlighted(Tools::isSelected(acsr, true));
                mEditor.setFocusInfo(acsr.getAttr<AttrType::focused>());
                break;
            }
        }
    };

    MiscWeakAssert(resizerFn != nullptr);
    mResizerBar.onMouseDrag = [=, this](juce::MouseEvent const& event)
    {
        auto const size = mResizerBar.getNewPosition();
        if(resizerFn != nullptr)
        {
            resizerFn(mAccessor.getAttr<AttrType::identifier>(), size);
        }
        else
        {
            mAccessor.setAttr<AttrType::height>(size, NotificationType::synchronous);
        }
        if(auto* viewport = findParentComponentOfClass<juce::Viewport>())
        {
            if(viewport->canScrollVertically())
            {
                auto const point = event.getEventRelativeTo(viewport).getPosition();
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
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::Section::~Section()
{
    mAccessor.removeListener(mListener);
}

juce::Rectangle<int> Track::Section::getPlotBounds() const
{
    return getLocalArea(&mPlot, mPlot.getBounds());
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
    auto bounds = getLocalBounds().withTrimmedLeft(getThumbnailOffset());
    mThumbnailDecoration.setBounds(bounds.removeFromLeft(getThumbnailWidth()));
    mSnapshotDecoration.setBounds(bounds.removeFromLeft(getSnapshotWidth()));
    mScrollBar.setBounds(bounds.removeFromRight(getScrollBarWidth()));
    mRulerDecoration.setBounds(bounds.removeFromRight(getRulerWidth()));
    mPlotDecoration.setBounds(bounds);
}

void Track::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
}

ANALYSE_FILE_END
