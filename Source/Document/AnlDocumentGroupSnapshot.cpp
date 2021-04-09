#include "AnlDocumentGroupSnapshot.h"
#include "AnlDocumentTools.h"
#include "../Zoom/AnlZoomTools.h"
#include "../Track/AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Document::GroupSnapshot::GroupSnapshot(Accessor& accessor)
: GroupContainer<std::unique_ptr<Track::Snapshot>>(accessor)
, mAccessor(accessor)
{
    juce::ignoreUnused(mAccessor);
    setInterceptsMouseClicks(false, false);
    setSize(100, 80);
}

void Document::GroupSnapshot::resized()
{
    auto bounds = getLocalBounds();
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    auto const& group = getGroupContent();
    for(auto const& identifier : layout)
    {
        auto it = group.find(identifier);
        if(it != group.cend() && it->second != nullptr)
        {
            it->second->setBounds(bounds);
        }
    }
}

void Document::GroupSnapshot::updateStarted()
{
    removeAllChildren();
}

void Document::GroupSnapshot::updateEnded()
{
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    auto const& group = getGroupContent();
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

void Document::GroupSnapshot::removeFromGroup(std::unique_ptr<Track::Snapshot>& value)
{
    removeChildComponent(value.get());
}

std::unique_ptr<Track::Snapshot> Document::GroupSnapshot::createForGroup(Track::Accessor& trackAccessor)
{
    auto& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
    return std::make_unique<Track::Snapshot>(trackAccessor, timeZoomAcsr);
}

Document::GroupSnapshot::Overlay::Overlay(GroupSnapshot& groupSnapshot)
: mGroupSnapshot(groupSnapshot)
, mAccessor(mGroupSnapshot.mAccessor)
{
    addAndMakeVisible(mGroupSnapshot);
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
    mAccessor.getAcsr<AcsrType::transport>().addListener(mTransportListener, NotificationType::synchronous);
}

Document::GroupSnapshot::Overlay::~Overlay()
{
    mAccessor.getAcsr<AcsrType::transport>().removeListener(mTransportListener);
}

void Document::GroupSnapshot::Overlay::resized()
{
    mGroupSnapshot.setBounds(getLocalBounds());
}

void Document::GroupSnapshot::Overlay::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);
}

void Document::GroupSnapshot::Overlay::mouseMove(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
}

void Document::GroupSnapshot::Overlay::mouseEnter(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
}

void Document::GroupSnapshot::Overlay::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    setTooltip("");
}

void Document::GroupSnapshot::Overlay::updateTooltip(juce::Point<int> const& pt)
{
    if(!getLocalBounds().contains(pt))
    {
        setTooltip("");
        return;
    }
    
    auto const time = mAccessor.getAcsr<AcsrType::transport>().getAttr<Transport::AttrType::runningPlayhead>();
    auto tooltip = Format::secondsToString(time);
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    for(auto const& identifier : layout)
    {
        auto trackAcsr = Tools::getTrack(mAccessor, identifier);
        if(trackAcsr)
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
