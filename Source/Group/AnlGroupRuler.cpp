#include "AnlGroupRuler.h"

ANALYSE_FILE_BEGIN

Group::SelectionBar::SelectionBar(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
, mTransportAccessor(transportAccessor)
, mLayoutNotifier(mAccessor, [this]()
                  {
                      updateLayout();
                  },
                  {Track::AttrType::identifier, Track::AttrType::channelsLayout, Track::AttrType::showInGroup})
{
    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::height:
            case AttrType::colour:
            case AttrType::expanded:
            case AttrType::layout:
            case AttrType::tracks:
            case AttrType::zoomid:
                break;
            case AttrType::focused:
            {
                colourChanged();
            }
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::SelectionBar::~SelectionBar()
{
    mAccessor.removeListener(mListener);
}

void Group::SelectionBar::resized()
{
    auto const bounds = getLocalBounds();
    auto const verticalRanges = Tools::getChannelVerticalRanges(mAccessor, getLocalBounds());
    for(auto const& verticalRange : verticalRanges)
    {
        anlWeakAssert(verticalRange.first < mSelectionBars.size());
        if(verticalRange.first < mSelectionBars.size())
        {
            auto& bar = mSelectionBars[verticalRange.first];
            if(bar != nullptr)
            {
                bar->setVisible(true);
                bar->setBounds(bounds.withTop(verticalRange.second.getStart()).withBottom(verticalRange.second.getEnd()));
            }
        }
    }
}

void Group::SelectionBar::colourChanged()
{
    auto const onColour = getLookAndFeel().findColour(Transport::SelectionBar::thumbCoulourId);
    auto const offColour = onColour.withAlpha(onColour.getFloatAlpha() * 0.5f);
    auto const& focused = mAccessor.getAttr<AttrType::focused>();

    for(auto channel = 0_z; channel < mSelectionBars.size() && channel < focused.size(); ++channel)
    {
        MiscWeakAssert(mSelectionBars[channel] != nullptr);
        if(mSelectionBars[channel] != nullptr)
        {
            auto const colour = focused.test(channel) ? onColour : offColour;
            mSelectionBars[channel]->setColour(Transport::SelectionBar::thumbCoulourId, colour);
        }
    }
}

void Group::SelectionBar::updateLayout()
{
    auto const channelsLayout = Tools::getChannelVisibilityStates(mAccessor);
    if(channelsLayout.size() < mSelectionBars.size())
    {
        mSelectionBars.resize(channelsLayout.size());
    }
    else
    {
        while(channelsLayout.size() > mSelectionBars.size())
        {
            auto bar = std::make_unique<Transport::SelectionBar>(mTransportAccessor, mTimeZoomAccessor);
            addAndMakeVisible(bar.get());
            mSelectionBars.push_back(std::move(bar));
        }
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
    resized();
}

ANALYSE_FILE_END
