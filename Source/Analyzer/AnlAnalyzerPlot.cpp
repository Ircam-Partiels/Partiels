#include "AnlAnalyzerPlot.h"

ANALYSE_FILE_BEGIN

Analyzer::Plot::Plot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
, mRenderer(accessor)
{
    mInformation.setEditable(false);
    mInformation.setInterceptsMouseClicks(false, false);
    addChildComponent(mInformation);
    addAndMakeVisible(mZoomPlayhead);
    
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::key:
            case AttrType::description:
            case AttrType::state:
            case AttrType::height:
            case AttrType::results:
            case AttrType::propertyState:
            case AttrType::warnings:
                break;
            case AttrType::colours:
            {
                repaint();
            }
                break;
            case AttrType::time:
            {
                mZoomPlayhead.setPosition(acsr.getAttr<AttrType::time>());
            }
                break;
            case AttrType::processing:
            {
                if(acsr.getAttr<AttrType::processing>())
                {
                    mInformation.setVisible(true);
                    mInformation.setText("processing...", juce::NotificationType::dontSendNotification);
                }
                else
                {
                    mInformation.setText("", juce::NotificationType::dontSendNotification);
                }
            }
                break;
        }
    };
    
    mZoomListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };
    
    mRenderer.onUpdated = [this]()
    {
        repaint();
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::binZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
    mTimeZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
}

Analyzer::Plot::~Plot()
{
    mTimeZoomAccessor.removeListener(mZoomListener);
    mAccessor.getAccessor<AcsrType::binZoom>(0).removeListener(mZoomListener);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Analyzer::Plot::resized()
{
    mInformation.setBounds(getLocalBounds().removeFromRight(200).removeFromTop(80));
    mZoomPlayhead.setBounds(getLocalBounds().reduced(2));
}

void Analyzer::Plot::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
    auto const bounds = getLocalBounds().reduced(2);
    juce::Path path;
    path.addRoundedRectangle(bounds.expanded(1), 4.0f);
    g.setColour(findColour(ColourIds::borderColourId));
    g.strokePath(path, juce::PathStrokeType(1.0f));
    path.clear();
    path.addRoundedRectangle(bounds, 4.0f);
    g.reduceClipRegion(path);
    
    mRenderer.paintRange(g, bounds, mTimeZoomAccessor);
}

void Analyzer::Plot::mouseMove(juce::MouseEvent const& event)
{
//    juce::ignoreUnused(event);
//    repaint();
//    auto const& results = mAccessor.getAttr<AttrType::results>();
//    if(results.empty() && getWidth() > 0)
//    {
//        return;
//    }
//    juce::String text;
//    
//    auto const timeRange = mZoomAccessor.getAttr<Zoom::AttrType::visibleRange>();
//    auto const time = static_cast<double>(event.x) / static_cast<double>(getWidth()) * timeRange.getLength() + timeRange.getStart();
//    auto const rtr = Vamp::RealTime::fromSeconds(time);
//    text += Format::secondsToString(time) + "\n";
//    if(results.front().values.empty())
//    {
//        
//    }
//    else if(results.front().values.size() == 1)
//    {
//        for(size_t i = 0; i < results.size(); i++)
//        {
//            if(results[i].timestamp >= rtr)
//            {
//                text += juce::String(results[i].values[0]) + results[i].label;
//                break;
//            }
//        }
//    }
//    else
//    {
//        auto const valueRange = mAccessor.getAccessor<AcsrType::binZoom>(0).getAttr<Zoom::AttrType::visibleRange>();
//        for(size_t i = 0; i < results.size(); i++)
//        {
//            if(results[i].timestamp >= rtr)
//            {
//                auto const index = std::max(std::min((1.0 - static_cast<double>(event.y) / static_cast<double>(getHeight())), 1.0), 0.0) * valueRange.getLength() + valueRange.getStart();
//                text += juce::String(static_cast<long>(index)) + "\n";
//                text += juce::String(results[i].values[static_cast<size_t>(index)] / valueRange.getLength()) + results[i].label;
//                break;
//            }
//        }
//    }
//    mInformation.setText(text, juce::NotificationType::dontSendNotification);
}

void Analyzer::Plot::mouseEnter(juce::MouseEvent const& event)
{
    mInformation.setVisible(true);
    mouseMove(event);
}

void Analyzer::Plot::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    mInformation.setVisible(false);
}

ANALYSE_FILE_END
