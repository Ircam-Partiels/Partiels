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
            case AttrType::key:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::focused:
            case AttrType::graphics:
            case AttrType::colours:
            case AttrType::grid:
                break;
            case AttrType::description:
            case AttrType::results:
            case AttrType::channelsLayout:
            {
                auto const displayType = Tools::getDisplayType(mAccessor);
                if(mDisplayType != displayType)
                {
                    mRulers.clear();
                    mDisplayType = displayType;
                }
                if(mDisplayType == Tools::DisplayType::markers)
                {
                    return;
                }
                auto const channelsLayout = acsr.getAttr<AttrType::channelsLayout>();
                while(channelsLayout.size() < mRulers.size())
                {
                    mRulers.pop_back();
                }
                auto& zoomAcsr = mDisplayType == Tools::DisplayType::points ? mAccessor.getAcsr<AcsrType::valueZoom>() : mAccessor.getAcsr<AcsrType::binZoom>();
                while(channelsLayout.size() > mRulers.size())
                {
                    auto ruler = std::make_unique<Zoom::Ruler>(zoomAcsr, Zoom::Ruler::Orientation::vertical);
                    anlWeakAssert(ruler != nullptr);
                    if(ruler != nullptr)
                    {
                        ruler->onDoubleClick = [&]()
                        {
                            auto const& range = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
                            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
                        };
                        addChildComponent(ruler.get());
                    }
                    mRulers.push_back(std::move(ruler));
                }
                lookAndFeelChanged();
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
    Tools::paintChannels(mAccessor, g, getLocalBounds(), [&](juce::Rectangle<int>, size_t)
                         {
                         });
}

void Track::Ruler::lookAndFeelChanged()
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
            case AttrType::key:
            case AttrType::state:
            case AttrType::height:
            case AttrType::colours:
            case AttrType::channelsLayout:
            case AttrType::zoomLink:
            case AttrType::graphics:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::zoomAcsr:
            case AttrType::grid:
            case AttrType::focused:
                break;
            case AttrType::description:
            case AttrType::results:
            {
                auto const displayType = Tools::getDisplayType(acsr);
                mValueScrollBar.setVisible(displayType == Tools::DisplayType::points);
                mBinScrollBar.setVisible(displayType == Tools::DisplayType::columns);
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

ANALYSE_FILE_END
