#include "AnlTrackRuler.h"

ANALYSE_FILE_BEGIN

Track::Ruler::Ruler(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
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
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::focused:
            case AttrType::graphics:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::unit:
            case AttrType::grid:
            case AttrType::showInGroup:
                break;
            case AttrType::description:
            case AttrType::results:
            case AttrType::channelsLayout:
            {
                auto const FrameType = Tools::getFrameType(mAccessor);
                if(mFrameType != FrameType)
                {
                    mRulers.clear();
                    mFrameType = FrameType;
                }
                if(mFrameType == Track::FrameType::label)
                {
                    return;
                }
                auto const channelsLayout = acsr.getAttr<AttrType::channelsLayout>();
                while(channelsLayout.size() < mRulers.size())
                {
                    mRulers.pop_back();
                }
                auto& zoomAcsr = mFrameType == Track::FrameType::value ? mAccessor.getAcsr<AcsrType::valueZoom>() : mAccessor.getAcsr<AcsrType::binZoom>();
                while(channelsLayout.size() > mRulers.size())
                {
                    auto ruler = std::make_unique<Zoom::Ruler>(zoomAcsr, Zoom::Ruler::Orientation::vertical);
                    anlWeakAssert(ruler != nullptr);
                    if(ruler != nullptr)
                    {
                        ruler->onMouseDown = [&, rulerPtr = ruler.get()](juce::MouseEvent const& event)
                        {
                            if(event.mods.isCtrlDown())
                            {
                                auto rangeEditor = mFrameType == Track::FrameType::value ? Tools::createValueRangeEditor(mAccessor) : Tools::createBinRangeEditor(mAccessor);
                                if(rangeEditor == nullptr)
                                {
                                    return false;
                                }
                                auto const point = juce::Desktop::getMousePosition();
                                auto const rulerBounds = rulerPtr->getScreenBounds();
                                auto const width = rulerBounds.getWidth();
                                auto const bounds = rulerBounds.withY(point.getY() - width / 2).withHeight(width);
                                juce::CallOutBox::launchAsynchronously(std::move(rangeEditor), bounds, nullptr).setArrowSize(0.0f);
                                return false;
                            }
                            return true;
                        };
                        ruler->onDoubleClick = [&]([[maybe_unused]] juce::MouseEvent const& event)
                        {
                            auto const& range = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
                            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
                        };
                        addChildComponent(ruler.get());
                    }
                    mRulers.push_back(std::move(ruler));
                }
                colourChanged();
                resized();
            }
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::Ruler::~Ruler()
{
    mAccessor.removeListener(mListener);
}

void Track::Ruler::resized()
{
    if(mFrameType == Track::FrameType::label)
    {
        return;
    }

    for(auto& ruler : mRulers)
    {
        anlWeakAssert(ruler != nullptr);
        if(ruler != nullptr)
        {
            ruler->setVisible(false);
        }
    }
    auto const bounds = getLocalBounds();
    auto const verticalRanges = Tools::getChannelVerticalRanges(mAccessor, getLocalBounds());
    for(auto const& verticalRange : verticalRanges)
    {
        anlWeakAssert(verticalRange.first < mRulers.size());
        if(verticalRange.first < mRulers.size())
        {
            auto& ruler = mRulers[verticalRange.first];
            if(ruler != nullptr)
            {
                ruler->setVisible(true);
                ruler->setBounds(bounds.withTop(verticalRange.second.getStart()).withBottom(verticalRange.second.getEnd()));
            }
        }
    }
}

void Track::Ruler::paint(juce::Graphics& g)
{
    Tools::paintChannels(mAccessor, g, getLocalBounds(), findColour(Decorator::ColourIds::normalBorderColourId), [&](juce::Rectangle<int>, size_t)
                         {
                         });
}

void Track::Ruler::colourChanged()
{
    for(auto& ruler : mRulers)
    {
        anlWeakAssert(ruler != nullptr);
        if(ruler != nullptr)
        {
            ruler->setColour(Zoom::Ruler::ColourIds::gridColourId, findColour(Decorator::ColourIds::normalBorderColourId));
        }
    }
}

Track::ScrollBar::ScrollBar(Accessor& accessor)
: mAccessor(accessor)
{
    addChildComponent(mValueScrollBar);
    addChildComponent(mBinScrollBar);

    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType type)
    {
        switch(type)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::file:
            case AttrType::edit:
            case AttrType::key:
            case AttrType::input:
            case AttrType::state:
            case AttrType::height:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::unit:
            case AttrType::channelsLayout:
            case AttrType::zoomLink:
            case AttrType::graphics:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::zoomAcsr:
            case AttrType::grid:
            case AttrType::focused:
            case AttrType::showInGroup:
                break;
            case AttrType::description:
            case AttrType::results:
            {
                auto const FrameType = Tools::getFrameType(acsr);
                mValueScrollBar.setVisible(FrameType == Track::FrameType::value);
                mBinScrollBar.setVisible(FrameType == Track::FrameType::vector);
            }
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::ScrollBar::~ScrollBar()
{
    mAccessor.removeListener(mListener);
}

void Track::ScrollBar::resized()
{
    auto const bounds = getLocalBounds();
    mValueScrollBar.setBounds(bounds);
    mBinScrollBar.setBounds(bounds);
}

Track::SelectionBar::SelectionBar(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
, mTransportAccessor(transportAccessor)
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::file:
            case AttrType::key:
            case AttrType::input:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::graphics:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::unit:
            case AttrType::grid:
            case AttrType::description:
            case AttrType::results:
            case AttrType::edit:
            case AttrType::showInGroup:
                break;
            case AttrType::focused:
            {
                colourChanged();
            }
            break;
            case AttrType::channelsLayout:
            {
                auto const channelsLayout = acsr.getAttr<AttrType::channelsLayout>();
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
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::SelectionBar::~SelectionBar()
{
    mAccessor.removeListener(mListener);
}

void Track::SelectionBar::resized()
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

void Track::SelectionBar::colourChanged()
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

ANALYSE_FILE_END
