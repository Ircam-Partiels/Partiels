#include "AnlTrackSection.h"

ANALYSE_FILE_BEGIN

Track::Section::Section(Accessor& accessor, Zoom::Accessor& timeZoomAcsr, juce::Component& separator)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAcsr)
, mSeparator(separator)
{
    mValueRuler.onDoubleClick = [&]()
    {
        auto& zoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
        auto const& range = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
    };
    
    mBinRuler.onDoubleClick = [&]()
    {
        auto& zoomAcsr = mAccessor.getAccessor<AcsrType::binZoom>(0);
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
                auto const description = acsr.getAttr<AttrType::description>();
                switch(description.output.binCount)
                {
                    case 0_z:
                    {
                        mBinRuler.setVisible(false);
                        mBinScrollBar.setVisible(false);
                        
                        mValueRuler.setVisible(true);
                        mValueScrollBar.setVisible(true);
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
            case AttrType::results:
            case AttrType::graphics:
            case AttrType::time:
            case AttrType::warnings:
            case AttrType::processing:
                break;
        }
    };
    
    mBoundsListener.onComponentMoved = [&](juce::Component& component)
    {
        anlStrongAssert(&component == &mSeparator);
        if(&component == &mSeparator)
        {
            resized();
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
    
    mBoundsListener.attachTo(mSeparator);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::Section::~Section()
{
    mAccessor.removeListener(mListener);
    mBoundsListener.detachFrom(mSeparator);
}

juce::String Track::Section::getIdentifier() const
{
    return mAccessor.getAttr<AttrType::identifier>();
}

void Track::Section::resized()
{
    auto bounds = getLocalBounds();
    auto const leftSize = mSeparator.getScreenX() - getScreenX();
    auto const rightSize = getWidth() - leftSize - mSeparator.getWidth();
    
    // Resizers
    {
        auto resizersBounds = bounds.removeFromBottom(2);
        mResizerBarLeft.setBounds(resizersBounds.removeFromLeft(leftSize).reduced(4, 0));
        mResizerBarRight.setBounds(resizersBounds.removeFromRight(rightSize).reduced(4, 0));
    }
    
    // Thumbnail and Snapshot
    {
        auto leftSide = bounds.removeFromLeft(leftSize);
        mThumbnailDecoration.setBounds(leftSide.removeFromLeft(48));
        mSnapshotDecoration.setBounds(leftSide.withTrimmedLeft(1));
    }
    
    // Plot, Rulers and Scrollbars
    {
        auto rightSide = bounds.removeFromRight(rightSize);
        auto const scrollbarBounds = rightSide.removeFromRight(8).reduced(0, 4);
        mValueScrollBar.setBounds(scrollbarBounds);
        mBinScrollBar.setBounds(scrollbarBounds);
        auto const rulerBounds = rightSide.removeFromRight(16).reduced(0, 4);
        mValueRuler.setBounds(rulerBounds);
        mBinRuler.setBounds(rulerBounds);
        mPlotDecoration.setBounds(rightSide);
    }
}

void Track::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::sectionColourId, true));
}

ANALYSE_FILE_END
