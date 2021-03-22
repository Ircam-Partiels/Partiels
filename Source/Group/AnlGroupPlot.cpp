#include "AnlGroupPlot.h"

ANALYSE_FILE_BEGIN

Group::Plot::Plot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
{
    addChildComponent(mProcessingButton);
    addAndMakeVisible(mZoomPlayhead);
    
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::name:
                break;
            case AttrType::height:
                break;
            case AttrType::layout:
            {
                repaint();
            }
                break;
        }
    };
    
    mZoomListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };
    
    mTrackListener.onAttrChanged = [this](Track::Accessor const& acsr, Track::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Track::AttrType::identifier:
            case Track::AttrType::name:
            case Track::AttrType::key:
            case Track::AttrType::description:
            case Track::AttrType::state:
            case Track::AttrType::height:
            case Track::AttrType::results:
            case Track::AttrType::propertyState:
            case Track::AttrType::warnings:
            case Track::AttrType::time:
                break;
            case Track::AttrType::colours:
            {
                repaint();
            }
                break;
            case Track::AttrType::processing:
            {
                auto const state = std::any_of(mRenderers.cbegin(), mRenderers.cend(), [](auto const& renderer)
                {
                    return std::get<0>(renderer).get().template getAttr<Track::AttrType::processing>();
                });
                mProcessingButton.setActive(state);
                mProcessingButton.setVisible(state);
                mProcessingButton.setTooltip(state ? juce::translate("Processing analysis...") : juce::translate("Analysis finished!"));
            }
                break;
        }
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mTimeZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
}

Group::Plot::~Plot()
{
    for(auto& renderer : mRenderers)
    {
        auto& trackAcsr = std::get<0>(renderer).get();
        trackAcsr.getAccessor<Track::AcsrType::binZoom>(0).removeListener(mZoomListener);
        trackAcsr.getAccessor<Track::AcsrType::valueZoom>(0).removeListener(mZoomListener);
        trackAcsr.removeListener(mTrackListener);
    }
    mTimeZoomAccessor.removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Group::Plot::addTrack(Track::Accessor& trackAcsr)
{
    repaint();
}

void Group::Plot::removeTrack(Track::Accessor& trackAcsr)
{
    trackAcsr.getAccessor<Track::AcsrType::binZoom>(0).removeListener(mZoomListener);
    trackAcsr.getAccessor<Track::AcsrType::valueZoom>(0).removeListener(mZoomListener);
    trackAcsr.removeListener(mTrackListener);
    mRenderers.erase(trackAcsr);
    repaint();
}

void Group::Plot::resized()
{
    mZoomPlayhead.setBounds(getLocalBounds().reduced(2));
    mProcessingButton.setBounds(8, 8, 20, 20);
}

void Group::Plot::paint(juce::Graphics& g)
{
}

ANALYSE_FILE_END
