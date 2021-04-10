#include "AnlTrackSection.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Section::Section(Accessor& accessor, Zoom::Accessor& timeZoomAcsr, Transport::Accessor& transportAcsr)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAcsr)
, mTransportAccessor(transportAcsr)
{
    mValueRuler.onDoubleClick = [&]()
    {
        auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
        auto const& range = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
    };
    
    mBinRuler.onDoubleClick = [&]()
    {
        auto& zoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
        auto const range = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
    };
    
    mThumbnail.onRemove = [&]()
    {
        if(onRemove != nullptr)
        {
            onRemove();
        }
    };
    
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType type)
    {
        switch(type)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::key:
                break;
            case AttrType::description:
            {
                mValueRuler.setVisible(Tools::getDisplayType(mAccessor) == Tools::DisplayType::segments);
                mValueScrollBar.setVisible(Tools::getDisplayType(mAccessor) == Tools::DisplayType::segments);
                mBinRuler.setVisible(Tools::getDisplayType(mAccessor) == Tools::DisplayType::grid);
                mBinScrollBar.setVisible(Tools::getDisplayType(mAccessor) == Tools::DisplayType::grid);
            }
                break;
            case AttrType::state:
                break;
            case AttrType::height:
            {
                setSize(getWidth(), acsr.getAttr<AttrType::height>() + 1);
            }
                break;
            case AttrType::colours:
            case AttrType::propertyState:
            case AttrType::zoomLink:
            case AttrType::results:
            case AttrType::graphics:
            case AttrType::time:
            case AttrType::warnings:
            case AttrType::processing:
                break;
        }
    };
    
    mResizerBar.onMoved = [&](int size)
    {
        mAccessor.setAttr<AttrType::height>(size, NotificationType::synchronous);
    };
    
    addChildComponent(mValueRuler);
    addChildComponent(mValueScrollBar);
    addChildComponent(mBinRuler);
    addChildComponent(mBinScrollBar);
    addAndMakeVisible(mThumbnailDecoration);
    addAndMakeVisible(mSnapshotDecoration);
    addAndMakeVisible(mPlotDecoration);
    addAndMakeVisible(mResizerBar);
    setSize(80, 100);
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::Section::~Section()
{
    mAccessor.removeListener(mListener);
}

juce::String Track::Section::getIdentifier() const
{
    return mAccessor.getAttr<AttrType::identifier>();
}

void Track::Section::resized()
{
    mResizerBar.setBounds(getLocalBounds().removeFromBottom(1).reduced(2, 0));
    
    auto bounds = getLocalBounds();
    mThumbnailDecoration.setBounds(bounds.removeFromLeft(48));
    mSnapshotDecoration.setBounds(bounds.removeFromLeft(48));
    
    mValueScrollBar.setBounds(bounds.removeFromRight(8));
    mBinScrollBar.setBounds(mValueScrollBar.getBounds());
    mValueRuler.setBounds(bounds.removeFromRight(16));
    mBinRuler.setBounds(mValueRuler.getBounds());
    mPlotDecoration.setBounds(bounds);
}

void Track::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::sectionColourId, true));
}

void Track::Section::mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel)
{
    if(!event.mods.isCommandDown())
    {
        Component::mouseWheelMove(event, wheel);
        return;
    }
    
    switch(Tools::getDisplayType(mAccessor))
    {
        case Tools::DisplayType::markers:
            break;
        case Tools::DisplayType::segments:
        {
            auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
            auto const visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            auto const offset = static_cast<double>(-wheel.deltaY) * visibleRange.getLength();
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange - offset, NotificationType::synchronous);
        }
            break;
        case Tools::DisplayType::grid:
        {
            auto& zoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
            auto const visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            auto const offset = static_cast<double>(-wheel.deltaY) * visibleRange.getLength();
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange - offset, NotificationType::synchronous);
        }
            break;
    }
}

void Track::Section::mouseMagnify(juce::MouseEvent const& event, float magnifyAmount)
{
    if(!event.mods.isCommandDown())
    {
        Component::mouseMagnify(event, magnifyAmount);
        return;
    }
    
    switch(Tools::getDisplayType(mAccessor))
    {
        case Tools::DisplayType::markers:
            break;
        case Tools::DisplayType::segments:
        {
            auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
            auto const globalRange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
            auto const amount = static_cast<double>(1.0f - magnifyAmount) / 5.0 * globalRange.getLength();
            auto const visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange.expanded(amount), NotificationType::synchronous);
        }
            break;
        case Tools::DisplayType::grid:
        {
            auto& zoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
            auto const globalRange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
            auto const amount = static_cast<double>(1.0f - magnifyAmount) / 5.0 * globalRange.getLength();
            auto const visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange.expanded(amount), NotificationType::synchronous);
        }
            break;
    }
}


ANALYSE_FILE_END
