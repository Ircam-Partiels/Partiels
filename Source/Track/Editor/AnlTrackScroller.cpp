#include "AnlTrackScroller.h"
#include "../AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Scroller::Scroller(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor, juce::ApplicationCommandManager& commandManager)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
, mTransportAccessor(transportAccessor)
, mCommandManager(commandManager)
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::file:
            case AttrType::edit:
            case AttrType::key:
            case AttrType::input:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomValueMode:
            case AttrType::extraThresholds:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::focused:
            case AttrType::graphics:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::lineWidth:
            case AttrType::unit:
            case AttrType::labelLayout:
            case AttrType::showInGroup:
            case AttrType::oscIdentifier:
            case AttrType::sendViaOsc:
            case AttrType::grid:
            case AttrType::hasPluginColourMap:
            case AttrType::sampleRate:
            case AttrType::zoomLogScale:
                break;
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::description:
            case AttrType::results:
            case AttrType::channelsLayout:
            {
                auto const channelsLayout = acsr.getAttr<AttrType::channelsLayout>();
                while(channelsLayout.size() < mMouseScrollers.size())
                {
                    mMouseScrollers.pop_back();
                }
                while(channelsLayout.size() > mMouseScrollers.size())
                {
                    auto scroller = std::make_unique<MouseScroller>();
                    anlWeakAssert(scroller != nullptr);
                    if(scroller != nullptr)
                    {
                        addChildComponent(scroller.get());
                    }
                    mMouseScrollers.push_back(std::move(scroller));
                }
                applicationCommandListChanged();
                resized();
                break;
            }
        }
    };
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mCommandManager.addListener(this);
}

Track::Scroller::~Scroller()
{
    mCommandManager.removeListener(this);
    mAccessor.removeListener(mListener);
}

void Track::Scroller::resized()
{
    auto const bounds = getLocalBounds();
    for(auto& scroller : mMouseScrollers)
    {
        anlWeakAssert(scroller != nullptr);
        if(scroller != nullptr)
        {
            scroller->setVisible(false);
        }
    }
    auto const verticalRanges = Tools::getChannelVerticalRanges(mAccessor, bounds);
    for(auto const& verticalRange : verticalRanges)
    {
        anlWeakAssert(verticalRange.first < mMouseScrollers.size());
        if(verticalRange.first < mMouseScrollers.size())
        {
            auto& scoller = mMouseScrollers[verticalRange.first];
            if(scoller != nullptr)
            {
                scoller->setVisible(true);
                scoller->setBounds(bounds.withTop(verticalRange.second.getStart()).withBottom(verticalRange.second.getEnd()));
            }
        }
    }
}

void Track::Scroller::mouseMagnify(juce::MouseEvent const& event, float magnifyAmount)
{
    for(auto& scroller : mMouseScrollers)
    {
        anlWeakAssert(scroller != nullptr);
        if(scroller != nullptr && scroller->isVisible())
        {
            auto const relEvent = event.getEventRelativeTo(scroller.get());
            if(scroller->contains(relEvent.position))
            {
                scroller->mouseMagnify(relEvent, magnifyAmount);
                return;
            }
        }
    }
}

void Track::Scroller::mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel)
{
    for(auto& scroller : mMouseScrollers)
    {
        anlWeakAssert(scroller != nullptr);
        if(scroller != nullptr && scroller->isVisible())
        {
            auto const relEvent = event.getEventRelativeTo(scroller.get());
            if(scroller->contains(relEvent.position))
            {
                scroller->mouseWheelMove(relEvent, wheel);
                return;
            }
        }
    }
}

void Track::Scroller::applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info)
{
    switch(info.commandID)
    {
        case ApplicationCommandIDs::viewTimeZoomAnchorOnPlayhead:
        {
            auto zoomAcsr = Tools::getVerticalZoomAccessor(mAccessor, true);
            auto const useTransport = info.commandFlags & juce::ApplicationCommandInfo::CommandFlags::isTicked;
            auto transportAcsrRef = useTransport ? optional_ref<Transport::Accessor>(std::ref(mTransportAccessor)) : optional_ref<Transport::Accessor>{};
            for(auto& scoller : mMouseScrollers)
            {
                anlWeakAssert(scoller != nullptr);
                if(scoller != nullptr)
                {
                    scoller->setAccessors(mTimeZoomAccessor, zoomAcsr, transportAcsrRef);
                }
            }
            break;
        }
        default:
            break;
    }
}

void Track::Scroller::applicationCommandListChanged()
{
    Utils::notifyListener(mCommandManager, *this, {ApplicationCommandIDs::viewTimeZoomAnchorOnPlayhead});
}

ANALYSE_FILE_END
