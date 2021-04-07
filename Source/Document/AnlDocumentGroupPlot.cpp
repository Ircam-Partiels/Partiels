#include "AnlDocumentGroupPlot.h"
#include "../Zoom/AnlZoomTools.h"
#include "../Track/AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Document::GroupPlot::GroupPlot(Accessor& accessor)
: mAccessor(accessor)
{
    auto updateLayout = [&]()
    {
        auto const& layout = mAccessor.getAttr<AttrType::layout>();
        auto const& trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
        std::erase_if(mPlots, [&](auto const& pair)
        {
            return std::binary_search(layout.cbegin(), layout.cend(), pair.first) ||
            std::none_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
            {
                return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == pair.first;
            });
        });
        
        removeAllChildren();
        auto& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
        auto& transportAcsr = mAccessor.getAcsr<AcsrType::transport>();
        for(auto const& identifier : layout)
        {
            auto plotIt = mPlots.find(identifier);
            if(plotIt == mPlots.cend())
            {
                auto trackIt = std::find_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                {
                    return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
                });
                if(trackIt != trackAcsrs.cend())
                {
                    auto plot = std::make_unique<Track::Plot>(*trackIt, timeZoomAcsr, transportAcsr);
                    anlStrongAssert(plot != nullptr);
                    if(plot != nullptr)
                    {
                        addAndMakeVisible(plot.get(), 0);
                        mPlots[identifier] = std::move(plot);
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

Document::GroupPlot::~GroupPlot()
{
    mAccessor.removeListener(mListener);
}

void Document::GroupPlot::resized()
{
    auto bounds = getLocalBounds();
    
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    for(auto const& identifier : layout)
    {
        auto it = mPlots.find(identifier);
        if(it != mPlots.cend() && it->second != nullptr)
        {
            it->second->setBounds(bounds);
        }
    }
}

Document::GroupPlot::Overlay::Overlay(GroupPlot& groupPlot)
: mGroupPlot(groupPlot)
, mAccessor(mGroupPlot.mAccessor)
, mTransportPlayheadContainer(mAccessor.getAcsr<AcsrType::transport>(), mAccessor.getAcsr<AcsrType::timeZoom>())
{
    addAndMakeVisible(mGroupPlot);
    mTooltip.setEditable(false);
    mTooltip.setJustificationType(juce::Justification::topLeft);
    mTooltip.setInterceptsMouseClicks(false, false);
    mTooltip.setComponentEffect(&mDropShadowEffect);
    addChildComponent(mTooltip);
    addAndMakeVisible(mTransportPlayheadContainer);
    mTransportPlayheadContainer.setInterceptsMouseClicks(false, false);
    addMouseListener(&mTransportPlayheadContainer, false);
    setInterceptsMouseClicks(true, true);
}

void Document::GroupPlot::Overlay::resized()
{
    auto const bounds = getLocalBounds();
    mGroupPlot.setBounds(bounds);
    mTransportPlayheadContainer.setBounds(bounds);
    mTooltip.setBounds(0, 0, 200, getHeight());
}

void Document::GroupPlot::Overlay::mouseMove(juce::MouseEvent const& event)
{
    auto const time = Zoom::Tools::getScaledValueFromWidth(mAccessor.getAcsr<AcsrType::timeZoom>(), *this, event.x);
    
    auto const& tracks = mAccessor.getAcsrs<AcsrType::tracks>();
    auto const& layout = mAccessor.getAttr<AttrType::layout>();

    auto text = Format::secondsToString(time);
    for(auto it = layout.crbegin(); it != layout.crend(); ++it)
    {
        auto const identifier = *it;
        auto trackIt = std::find_if(tracks.cbegin(), tracks.cend(), [&](auto const& trackAcsr)
        {
            return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
        });
        if(trackIt != tracks.cend())
        {
            auto const& trackAcsr = trackIt->get();
            auto const name = trackAcsr.getAttr<Track::AttrType::name>();
            
            auto const bin = Zoom::Tools::getScaledValueFromHeight(trackAcsr.getAcsr<Track::AcsrType::binZoom>(), *this, event.y);
            auto const tip = Format::secondsToString(time) + " • "+ Track::Tools::getResultText(trackAcsr, time, static_cast<size_t>(std::floor(bin)));
            text += "\n" + tip;
        }
    }

//    auto const tip = Format::secondsToString(time) + " • "+  getTooltip();
//    setTooltip(name + ": " + tip);
    mTooltip.setText(text, juce::NotificationType::dontSendNotification);
}

void Document::GroupPlot::Overlay::mouseEnter(juce::MouseEvent const& event)
{
    mTooltip.setVisible(true);
    mouseMove(event);
}

void Document::GroupPlot::Overlay::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    mTooltip.setVisible(false);
}

ANALYSE_FILE_END
