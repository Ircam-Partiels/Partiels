#include "AnlTrackSection.h"
#include "AnlTrackResults.h"

ANALYSE_FILE_BEGIN

Track::Section::Container::Container(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, juce::Component& content, bool showPlayhead)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
, mContent(content)
, mZoomPlayhead(timeZoomAccessor)
{
    addAndMakeVisible(mContent);
    addChildComponent(mProcessingButton);
    mInformation.setEditable(false);
    mInformation.setInterceptsMouseClicks(false, false);
    addChildComponent(mInformation);
    if(showPlayhead)
    {
        addAndMakeVisible(mZoomPlayhead);
    }
    
    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::key:
            case AttrType::description:
            case AttrType::state:
            case AttrType::height:
            case AttrType::propertyState:
            case AttrType::warnings:
            case AttrType::results:
            case AttrType::graphics:
            case AttrType::colours:
                break;
            case AttrType::time:
            {
                if(mZoomPlayhead.isVisible())
                {
                    mZoomPlayhead.setPosition(acsr.getAttr<AttrType::time>());                    
                }
            }
                break;
            case AttrType::processing:
            {
                auto const state = mAccessor.getAttr<AttrType::processing>();
                mProcessingButton.setActive(state);
                mProcessingButton.setVisible(state);
                mProcessingButton.setTooltip(state ? juce::translate("Processing analysis...") : juce::translate("Analysis finished!"));
                repaint();
            }
                break;
        }
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::Section::Container::~Container()
{
    mAccessor.removeListener(mListener);
}

void Track::Section::Container::resized()
{
    auto bounds = getLocalBounds();
    mContent.setBounds(bounds);
    mInformation.setBounds(bounds.removeFromRight(200).removeFromTop(80));
    mProcessingButton.setBounds(8, 8, 20, 20);
}

void Track::Section::Container::paint(juce::Graphics& g)
{
    auto const& colours = mAccessor.getAttr<AttrType::colours>();
    g.setColour(colours.background);
    g.fillRect(getLocalBounds());
}

void Track::Section::Container::mouseMove(juce::MouseEvent const& event)
{
    auto const& resultsPtr = mAccessor.getAttr<AttrType::results>();
    if(resultsPtr == nullptr)
    {
        mInformation.setText("", juce::NotificationType::dontSendNotification);
        return;
    }
    
    auto const& results = *resultsPtr;
    if(results.empty() || getWidth() <= 0 || getHeight() <= 0)
    {
        mInformation.setText("", juce::NotificationType::dontSendNotification);
        return;
    }
    
    auto const timeRange = mTimeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>();
    auto const time = static_cast<double>(event.x) / static_cast<double>(getWidth()) * timeRange.getLength() + timeRange.getStart();
    juce::String text = Format::secondsToString(time) + "\n";
    auto it = Results::getResultAt(results, time);
    if(it != results.cend() && it->values.size() == 1)
    {
        text += juce::String(it->values[0]) + it->label;
    }
    else if(it != results.cend() && it->values.size() > 1)
    {
        auto const binRange = mAccessor.getAccessor<AcsrType::binZoom>(0).getAttr<Zoom::AttrType::visibleRange>();
        if(binRange.isEmpty())
        {
            mInformation.setText("", juce::NotificationType::dontSendNotification);
            return;
        }
        auto const index = std::max(std::min((1.0 - static_cast<double>(event.y) / static_cast<double>(getHeight())), 1.0), 0.0) * binRange.getLength() + binRange.getStart();
        text += juce::String(static_cast<long>(index)) + " ";
        
        auto const& description = mAccessor.getAttr<AttrType::description>();
        if(index < description.output.binNames.size())
        {
            text += juce::String(description.output.binNames[static_cast<size_t>(index)]);
        }
        text += "\n";
        if(index < it->values.size())
        {
            text += juce::String(it->values[static_cast<size_t>(index)] / binRange.getLength()) + it->label;
        }
        
    }
    mInformation.setText(text, juce::NotificationType::dontSendNotification);
}

void Track::Section::Container::mouseEnter(juce::MouseEvent const& event)
{
    mInformation.setVisible(true);
    mouseMove(event);
}

void Track::Section::Container::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    mInformation.setVisible(false);
}

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
    addAndMakeVisible(mThumbnail);
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
        mThumbnail.setBounds(leftSide.removeFromLeft(48));
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
