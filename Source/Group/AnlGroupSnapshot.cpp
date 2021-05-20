#include "AnlGroupSnapshot.h"
#include "../Track/AnlTrackTools.h"
#include "../Zoom/AnlZoomTools.h"

ANALYSE_FILE_BEGIN

Group::Snapshot::Snapshot(Accessor& accessor, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr)
: mAccessor(accessor)
, mTransportAccessor(transportAcsr)
, mTimeZoomAccessor(timeZoomAcsr)
{
    setInterceptsMouseClicks(false, false);
    setSize(100, 80);

    mListener.onAttrChanged = [this](class Accessor const& acsr, AttrType attribute)
    {
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
                mTrackSnapshots.updateContents(
                    mAccessor,
                    [this](Track::Accessor& trackAccessor)
                    {
                        return std::make_unique<Track::Snapshot>(trackAccessor, mTimeZoomAccessor, mTransportAccessor);
                    },
                    nullptr);

                auto const& layout = acsr.getAttr<AttrType::layout>();
                auto const& group = mTrackSnapshots.getContents();
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

Group::Snapshot::~Snapshot()
{
    mAccessor.removeListener(mListener);
}

void Group::Snapshot::resized()
{
    auto bounds = getLocalBounds();
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    auto const& group = mTrackSnapshots.getContents();
    for(auto const& identifier : layout)
    {
        auto it = group.find(identifier);
        if(it != group.cend() && it->second != nullptr)
        {
            it->second->setBounds(bounds);
        }
    }
}

Group::Snapshot::Overlay::Overlay(Snapshot& snapshot)
: mSnapshot(snapshot)
, mAccessor(mSnapshot.mAccessor)
, mTransportAccessor(mSnapshot.mTransportAccessor)
{
    addAndMakeVisible(mSnapshot);
    setInterceptsMouseClicks(true, true);

    mTransportListener.onAttrChanged = [this](Transport::Accessor const& acsr, Transport::AttrType attribute)
    {
        switch(attribute)
        {
            case Transport::AttrType::startPlayhead:
            {
                if(!acsr.getAttr<Transport::AttrType::playback>())
                {
                    updateTooltip(getMouseXYRelative());
                }
            }
            break;
            case Transport::AttrType::runningPlayhead:
            {
                if(acsr.getAttr<Transport::AttrType::playback>())
                {
                    updateTooltip(getMouseXYRelative());
                }
            }
            break;
            case Transport::AttrType::playback:
            case Transport::AttrType::looping:
            case Transport::AttrType::loopRange:
            case Transport::AttrType::gain:
                break;
        }
    };

    mTransportAccessor.addListener(mTransportListener, NotificationType::synchronous);
}

Group::Snapshot::Overlay::~Overlay()
{
    mTransportAccessor.removeListener(mTransportListener);
}

void Group::Snapshot::Overlay::resized()
{
    mSnapshot.setBounds(getLocalBounds());
}

void Group::Snapshot::Overlay::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);
}

void Group::Snapshot::Overlay::mouseMove(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
}

void Group::Snapshot::Overlay::mouseEnter(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
}

void Group::Snapshot::Overlay::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    setTooltip("");
}

void Group::Snapshot::Overlay::updateTooltip(juce::Point<int> const& pt)
{
    if(!getLocalBounds().contains(pt))
    {
        setTooltip("");
        return;
    }
    auto const isPlaying = mTransportAccessor.getAttr<Transport::AttrType::playback>();
    auto const time = isPlaying ? mTransportAccessor.getAttr<Transport::AttrType::runningPlayhead>() : mTransportAccessor.getAttr<Transport::AttrType::startPlayhead>();
    auto tooltip = Format::secondsToString(time);
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    for(auto const& identifier : layout)
    {
        auto trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
        if(trackAcsr.has_value())
        {
            auto const name = trackAcsr->get().getAttr<Track::AttrType::name>();
            auto const tip = Track::Tools::getValueTootip(trackAcsr->get(), mSnapshot.mTimeZoomAccessor, *this, pt.y, time);
            tooltip += "\n" + name + ": " + (tip.isEmpty() ? "-" : tip);
        }
    }
    setTooltip(tooltip);
}

ANALYSE_FILE_END
