#include "AnlTrackSection.h"

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
                auto const& output = acsr.getAttr<AttrType::description>().output;
                if(output.hasFixedBinCount)
                {
                    switch(output.binCount)
                    {
                        case 0_z:
                        {
                            mBinRuler.setVisible(false);
                            mBinScrollBar.setVisible(false);
                            
                            mValueRuler.setVisible(false);
                            mValueScrollBar.setVisible(false);
                        }
                            break;
                        case 1_z:
                        {
                            mBinRuler.setVisible(false);
                            mBinScrollBar.setVisible(false);
                            
                            mValueRuler.setVisible(true);
                            mValueScrollBar.setVisible(true);
                        }
                            break;
                        default:
                        {
                            mBinRuler.setVisible(true);
                            mBinScrollBar.setVisible(true);
                            
                            mValueRuler.setVisible(false);
                            mValueScrollBar.setVisible(false);
                        }
                            break;
                    }
                }
                else
                {
                    mBinRuler.setVisible(false);
                    mBinScrollBar.setVisible(false);
                    
                    mValueRuler.setVisible(true);
                    mValueScrollBar.setVisible(true);
                }
            }
                break;
            case AttrType::state:
                break;
            case AttrType::height:
            {
                setSize(getWidth(), acsr.getAttr<AttrType::height>() + 2);
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
    
    auto onResizerMoved = [&](int size)
    {
        mAccessor.setAttr<AttrType::height>(size, NotificationType::synchronous);
    };
    mResizerBarLeft.onMoved = onResizerMoved;
    mResizerBarRight.onMoved = onResizerMoved;
    
    addChildComponent(mValueRuler);
    addChildComponent(mValueScrollBar);
    addChildComponent(mBinRuler);
    addChildComponent(mBinScrollBar);
    addAndMakeVisible(mThumbnailDecoration);
    addAndMakeVisible(mSnapshotDecoration);
    addAndMakeVisible(mPlotDecoration);
    addAndMakeVisible(mResizerBarLeft);
    addAndMakeVisible(mResizerBarRight);
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
    auto bounds = getLocalBounds();
    auto resizersBounds = bounds.removeFromBottom(2);
    
    mThumbnailDecoration.setBounds(bounds.removeFromLeft(48));
    mSnapshotDecoration.setBounds(bounds.removeFromLeft(48));
    mResizerBarLeft.setBounds(resizersBounds.removeFromLeft(bounds.getX()).reduced(2, 0));
    
    mValueScrollBar.setBounds(bounds.removeFromRight(8));
    mBinScrollBar.setBounds(mValueScrollBar.getBounds());
    mValueRuler.setBounds(bounds.removeFromRight(16));
    mBinRuler.setBounds(mValueRuler.getBounds());
    mPlotDecoration.setBounds(bounds);
    mResizerBarRight.setBounds(resizersBounds.reduced(2, 0));
}

void Track::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::sectionColourId, true));
}

ANALYSE_FILE_END
