#include "AnlAnalyzerPlot.h"
#include "AnlAnalyzerResults.h"

ANALYSE_FILE_BEGIN

Analyzer::Plot::Plot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
, mRenderer(accessor, Renderer::Type::range)
{
    addChildComponent(mProcessingButton);
    mInformation.setEditable(false);
    mInformation.setInterceptsMouseClicks(false, false);
    addChildComponent(mInformation);
    addAndMakeVisible(mZoomPlayhead);
    
    auto updateProcessingButton = [this]()
    {
        auto const state = mRenderer.isPreparing() || mAccessor.getAttr<AttrType::processing>();
        mProcessingButton.setActive(state);
        mProcessingButton.setVisible(state);
        mProcessingButton.setTooltip(state ? juce::translate("Processing analysis...") : juce::translate("Analysis finished!"));
        repaint();
    };
    
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
                break;
            case AttrType::results:
            case AttrType::colours:
            {
                mRenderer.prepareRendering();
                updateProcessingButton();
            }
                break;
            case AttrType::time:
            {
                mZoomPlayhead.setPosition(acsr.getAttr<AttrType::time>());
            }
                break;
            case AttrType::processing:
            {
                updateProcessingButton();
            }
                break;
        }
    };
    
    mValueZoomListener.onAttrChanged = [=, this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        mRenderer.prepareRendering();
        updateProcessingButton();
    };
    
    mBinZoomListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };
    
    mTimeZoomListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };
    
    mRenderer.onUpdated = [=]()
    {
        updateProcessingButton();
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).addListener(mValueZoomListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::binZoom>(0).addListener(mBinZoomListener, NotificationType::synchronous);
    mTimeZoomAccessor.addListener(mTimeZoomListener, NotificationType::synchronous);
}

Analyzer::Plot::~Plot()
{
    mTimeZoomAccessor.removeListener(mTimeZoomListener);
    mAccessor.getAccessor<AcsrType::binZoom>(0).removeListener(mBinZoomListener);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).removeListener(mValueZoomListener);
    mAccessor.removeListener(mListener);
}

void Analyzer::Plot::resized()
{
    mInformation.setBounds(getLocalBounds().removeFromRight(200).removeFromTop(80));
    mZoomPlayhead.setBounds(getLocalBounds().reduced(2));
    mProcessingButton.setBounds(8, 8, 20, 20);
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
    
    mRenderer.paint(g, bounds, mTimeZoomAccessor);
}

void Analyzer::Plot::mouseMove(juce::MouseEvent const& event)
{
    auto const& results = mAccessor.getAttr<AttrType::results>();
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
