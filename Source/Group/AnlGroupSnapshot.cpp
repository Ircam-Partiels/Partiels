#include "AnlGroupSnapshot.h"
#include "../Zoom/AnlZoomTools.h"
#include "../Track/AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Group::Snapshot::Snapshot(Accessor& accessor, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr)
: mAccessor(accessor)
, mTransportAccessor(transportAcsr)
, mTimeZoomAccessor(timeZoomAcsr)
{
    juce::ignoreUnused(mTransportAccessor);
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
                break;
            case AttrType::layout:
            case AttrType::tracks:
            {
                removeAllChildren();
                mTrackSnapshots.updateContents(mAccessor,
                [this](Track::Accessor& trackAccessor)
                {
                    return std::make_unique<Track::Snapshot>(trackAccessor, mTimeZoomAccessor);
                },
                nullptr);
                
                auto const& layout = mAccessor.getAttr<AttrType::layout>();
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
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Transport::AttrType::playback:
            case Transport::AttrType::startPlayhead:
                break;
            case Transport::AttrType::runningPlayhead:
            {
                updateTooltip(getMouseXYRelative());
            }
                break;
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
    
    auto const time = mTransportAccessor.getAttr<Transport::AttrType::runningPlayhead>();
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
            auto const tip = Track::Tools::getResultText(trackAcsr->get(), time, static_cast<size_t>(std::floor(bin)));
            tooltip += "\n" + name + ": " + (tip.isEmpty() ? "-" : tip);
        }
    }
    setTooltip(tooltip);
}

ANALYSE_FILE_END
