#include "AnlTrackSelector.h"
#include "../AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Selector::Selector(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor, juce::ApplicationCommandManager& commandManager)
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
            case AttrType::key:
            case AttrType::input:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomValueMode:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::extraThresholds:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::graphics:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::unit:
            case AttrType::labelLayout:
            case AttrType::showInGroup:
            case AttrType::sendViaOsc:
            case AttrType::grid:
            case AttrType::description:
            case AttrType::results:
            case AttrType::edit:
            case AttrType::hasPluginColourMap:
            case AttrType::focused:
            case AttrType::sampleRate:
            case AttrType::zoomLogScale:
                break;
            case AttrType::channelsLayout:
            {
                auto const channelsLayout = acsr.getAttr<AttrType::channelsLayout>();
                if(channelsLayout.size() < mSelectionBars.size())
                {
                    mSelectionBars.resize(channelsLayout.size());
                }
                while(channelsLayout.size() > mSelectionBars.size())
                {
                    auto bar = std::make_unique<Transport::SelectionBar>(mTransportAccessor, mTimeZoomAccessor);
                    addAndMakeVisible(bar.get());
                    mSelectionBars.push_back(std::move(bar));
                }
                for(auto& bar : mSelectionBars)
                {
                    anlWeakAssert(bar != nullptr);
                    if(bar != nullptr)
                    {
                        bar->setVisible(false);
                    }
                }
                colourChanged();
                applicationCommandListChanged();
                resized();
            }
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mCommandManager.addListener(this);
}

Track::Selector::~Selector()
{
    mCommandManager.removeListener(this);
    mAccessor.removeListener(mListener);
}

void Track::Selector::resized()
{
    auto const bounds = getLocalBounds();
    auto const verticalRanges = Tools::getChannelVerticalRanges(mAccessor, getLocalBounds());
    for(auto const& verticalRange : verticalRanges)
    {
        anlWeakAssert(verticalRange.first < mSelectionBars.size());
        if(verticalRange.first < mSelectionBars.size())
        {
            auto& bar = mSelectionBars[verticalRange.first];
            MiscWeakAssert(bar != nullptr);
            if(bar != nullptr)
            {
                bar->setVisible(true);
                bar->setBounds(bounds.withTop(verticalRange.second.getStart()).withBottom(verticalRange.second.getEnd()));
            }
        }
    }
}

void Track::Selector::colourChanged()
{
    auto const onColour = getLookAndFeel().findColour(Transport::SelectionBar::thumbCoulourId);
    auto const offColour = onColour.withAlpha(onColour.getFloatAlpha() * 0.5f);
    for(auto channel = 0_z; channel < mSelectionBars.size() && channel < mFocusInfo.size(); ++channel)
    {
        auto& bar = mSelectionBars[channel];
        MiscWeakAssert(bar != nullptr);
        if(bar != nullptr)
        {
            auto const colour = mFocusInfo.test(channel) ? onColour : offColour;
            bar->setColour(Transport::SelectionBar::thumbCoulourId, colour);
        }
    }
}

void Track::Selector::applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info)
{
    switch(info.commandID)
    {
        case ApplicationCommandIDs::frameToggleDrawing:
        {
            auto const isDrawingMode = info.commandFlags & juce::ApplicationCommandInfo::CommandFlags::isTicked;
            for(auto& bar : mSelectionBars)
            {
                anlWeakAssert(bar != nullptr);
                if(bar != nullptr)
                {
                    bar->setDefaultMouseCursor(isDrawingMode ? juce::MouseCursor::IBeamCursor : juce::MouseCursor::NormalCursor);
                }
            }
            break;
        }
        default:
            break;
    }
}

void Track::Selector::applicationCommandListChanged()
{
    Utils::notifyListener(mCommandManager, *this, {ApplicationCommandIDs::frameToggleDrawing});
}

void Track::Selector::setFocusInfo(FocusInfo const& info)
{
    mFocusInfo = info;
    colourChanged();
}

ANALYSE_FILE_END
