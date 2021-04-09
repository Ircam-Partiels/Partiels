#include "AnlDocumentGroupPlot.h"
#include "AnlDocumentTools.h"
#include "../Zoom/AnlZoomTools.h"
#include "../Track/AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Document::GroupPlot::GroupPlot(Accessor& accessor)
: GroupContainer<std::unique_ptr<Track::Plot>>(accessor)
, mAccessor(accessor)
{
    juce::ignoreUnused(mAccessor);
    setInterceptsMouseClicks(false, false);
    setSize(100, 80);
}

void Document::GroupPlot::resized()
{
    auto const bounds = getLocalBounds();
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

void Document::GroupPlot::updateStarted()
{
    removeAllChildren();
}

void Document::GroupPlot::updateEnded()
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

void Document::GroupPlot::removeFromGroup(std::unique_ptr<Track::Plot>& value)
{
    removeChildComponent(value.get());
}

std::unique_ptr<Track::Plot> Document::GroupPlot::createForGroup(Track::Accessor& trackAccessor)
{
    auto& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
    auto& transportAcsr = mAccessor.getAcsr<AcsrType::transport>();
    return std::make_unique<Track::Plot>(trackAccessor, timeZoomAcsr, transportAcsr);
}

Document::GroupPlot::Overlay::Overlay(GroupPlot& groupPlot)
: mGroupPlot(groupPlot)
, mAccessor(mGroupPlot.mAccessor)
, mTransportPlayheadContainer(mAccessor.getAcsr<AcsrType::transport>(), mAccessor.getAcsr<AcsrType::timeZoom>())
{
    addAndMakeVisible(mGroupPlot);
    addAndMakeVisible(mTransportPlayheadContainer);
    mTransportPlayheadContainer.setInterceptsMouseClicks(false, false);
    addMouseListener(&mTransportPlayheadContainer, false);
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
    
    mAccessor.getAcsr<AcsrType::timeZoom>().addListener(mTimeZoomListener, NotificationType::synchronous);
}

Document::GroupPlot::Overlay::~Overlay()
{
    mAccessor.getAcsr<AcsrType::timeZoom>().removeListener(mTimeZoomListener);
}

void Document::GroupPlot::Overlay::resized()
{
    auto const bounds = getLocalBounds();
    mGroupPlot.setBounds(bounds);
    mTransportPlayheadContainer.setBounds(bounds);
}

void Document::GroupPlot::Overlay::mouseMove(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
}

void Document::GroupPlot::Overlay::mouseEnter(juce::MouseEvent const& event)
{
    updateTooltip({event.x, event.y});
}

void Document::GroupPlot::Overlay::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    setTooltip("");
}

void Document::GroupPlot::Overlay::updateTooltip(juce::Point<int> const& pt)
{
    if(!getLocalBounds().contains(pt))
    {
        setTooltip("");
        return;
    }
    
    auto const time = Zoom::Tools::getScaledValueFromWidth(mAccessor.getAcsr<AcsrType::timeZoom>(), *this, pt.x);
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
