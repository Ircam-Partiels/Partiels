#include "AnlGroupPlot.h"
#include "../Zoom/AnlZoomTools.h"
#include "../Track/AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Group::Plot::Plot(Accessor& accessor, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr)
: mAccessor(accessor)
, mTransportAccessor(transportAcsr)
, mTimeZoomAccessor(timeZoomAcsr)
{
    juce::ignoreUnused(mAccessor);
    juce::ignoreUnused(mTransportAccessor);
    juce::ignoreUnused(mTimeZoomAccessor);
    setInterceptsMouseClicks(false, false);
    setSize(100, 80);
    
    mListener.onAttrChanged = [this](class Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::height:
            case AttrType::colour:
            case AttrType::expanded:
            case AttrType::focused:
                break;
            case AttrType::layout:
            case AttrType::tracks:
            {
                removeAllChildren();
                mTrackPlots.updateContents(mAccessor,
                [this](Track::Accessor& trackAccessor)
                {
                    return std::make_unique<Track::Plot>(trackAccessor, mTimeZoomAccessor, mTransportAccessor);
                }, nullptr);
                
                auto const& layout = mAccessor.getAttr<AttrType::layout>();
                auto const& group = mTrackPlots.getContents();
                for(auto const& identifier : layout)
                {
                    auto it = group.find(identifier);
                    if(it != group.cend() && it->second != nullptr)
                    {
                        addAndMakeVisible(it->second.get(), 0);
                    }
                }
                resized();
            }
                break;
        }
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::Plot::~Plot()
{
    mAccessor.removeListener(mListener);
}

void Group::Plot::resized()
{
    auto const bounds = getLocalBounds();
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    auto const& contents = mTrackPlots.getContents();
    for(auto const& identifier : layout)
    {
        auto it = contents.find(identifier);
        if(it != contents.cend() && it->second != nullptr)
        {
            it->second->setBounds(bounds);
        }
    }
}

Group::Plot::Overlay::Overlay(Plot& plot)
: mPlot(plot)
, mAccessor(mPlot.mAccessor)
, mTimeZoomAccessor(mPlot.mTimeZoomAccessor)
, mTransportPlayheadBar(mPlot.mTransportAccessor, mTimeZoomAccessor)
{
    addAndMakeVisible(mPlot);
    addAndMakeVisible(mTransportPlayheadBar);
    mTransportPlayheadBar.setInterceptsMouseClicks(false, false);
    addMouseListener(&mTransportPlayheadBar, false);
    setInterceptsMouseClicks(true, true);
    
    mTimeZoomListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::minimumLength:
                break;
            case Zoom::AttrType::visibleRange:
            {
                updateTooltip(getMouseXYRelative());
            }
                break;
            case Zoom::AttrType::anchor:
                break;
        }
    };
    
    mTimeZoomAccessor.addListener(mTimeZoomListener, NotificationType::synchronous);
}

Group::Plot::Overlay::~Overlay()
{
    mTimeZoomAccessor.removeListener(mTimeZoomListener);
}

void Group::Plot::Overlay::resized()
{
    auto const bounds = getLocalBounds();
    mPlot.setBounds(bounds);
    mTransportPlayheadBar.setBounds(bounds);
}

void Group::Plot::Overlay::mouseMove(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
}

void Group::Plot::Overlay::mouseEnter(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
}

void Group::Plot::Overlay::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    setTooltip("");
}

void Group::Plot::Overlay::updateTooltip(juce::Point<int> const& pt)
{
    if(!getLocalBounds().contains(pt))
    {
        setTooltip("");
        return;
    }
    auto const& globalRange = mTimeZoomAccessor.getAttr<Zoom::AttrType::globalRange>();
    auto const& visibleRange = mTimeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>();
    auto const epsilon = 3.0 / static_cast<double>(getWidth()) * visibleRange.getLength();
    auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, pt.x);
    auto tooltip = Format::secondsToString(time);
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    for(auto const& identifier : layout)
    {
        auto trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
        if(trackAcsr.has_value())
        {
            auto const name = trackAcsr->get().getAttr<Track::AttrType::name>();
            auto const& binZoomAcsr = trackAcsr->get().getAcsr<Track::AcsrType::binZoom>();
            auto const bin = Zoom::Tools::getScaledValueFromHeight(binZoomAcsr, *this, pt.y);
            auto const tip = Track::Tools::getResultText(trackAcsr->get(), globalRange, time, static_cast<size_t>(std::floor(bin)), epsilon);
            tooltip += "\n" + name + ": " + (tip.isEmpty() ? "-" : tip);
        }
    }
    setTooltip(tooltip);
}

ANALYSE_FILE_END
