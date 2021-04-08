#include "AnlDocumentGroupSnapshot.h"
#include "AnlDocumentTools.h"
#include "../Zoom/AnlZoomTools.h"
#include "../Track/AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Document::GroupSnapshot::GroupSnapshot(Accessor& accessor)
: mAccessor(accessor)
{
    setInterceptsMouseClicks(false, false);
    auto updateLayout = [&]()
    {
        auto const& layout = mAccessor.getAttr<AttrType::layout>();
        std::erase_if(mSnapshots, [&](auto const& pair)
        {
            return !std::binary_search(layout.cbegin(), layout.cend(), pair.first) || !Tools::hasTrack(mAccessor, pair.first);
        });
        
        removeAllChildren();
        auto& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
        for(auto const& identifier : layout)
        {
            auto plotIt = mSnapshots.find(identifier);
            if(plotIt == mSnapshots.cend())
            {
                auto trackAcsr = Tools::getTrack(mAccessor, identifier);
                if(trackAcsr)
                {
                    auto plot = std::make_unique<Track::Snapshot>(*trackAcsr, timeZoomAcsr);
                    anlStrongAssert(plot != nullptr);
                    if(plot != nullptr)
                    {
                        addAndMakeVisible(plot.get(), 0);
                        mSnapshots[identifier] = std::move(plot);
                    }
                }
            }
            else
            {
                addAndMakeVisible(plotIt->second.get(), 0);
            }
        }
        
        resized();
    };
    
    mListener.onAttrChanged = [=](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::file:
            case AttrType::layoutHorizontal:
            case AttrType::layoutVertical:
            case AttrType::expanded:
                break;
            case AttrType::layout:
            {
                updateLayout();
            }
                break;
        }
    };
    
    mListener.onAccessorInserted = [=](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr, index);
        switch(type)
        {
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
            case AcsrType::tracks:
            {
                updateLayout();
            }
                break;
        }
    };
    
    mListener.onAccessorErased = [=](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr, index);
        switch(type)
        {
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
            case AcsrType::tracks:
            {
                updateLayout();
            }
                break;
        }
    };
    
    setSize(100, 80);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::GroupSnapshot::~GroupSnapshot()
{
    mAccessor.removeListener(mListener);
}

void Document::GroupSnapshot::resized()
{
    auto bounds = getLocalBounds();
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    for(auto const& identifier : layout)
    {
        auto it = mSnapshots.find(identifier);
        if(it != mSnapshots.cend() && it->second != nullptr)
        {
            it->second->setBounds(bounds);
        }
    }
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
