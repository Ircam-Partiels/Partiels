#include "AnlAnalyzerSnapshot.h"
#include "AnlAnalyzerRenderer.h"

ANALYSE_FILE_BEGIN

Analyzer::Snapshot::Snapshot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
, mRenderer(accessor, Renderer::Type::frame)
{
    addChildComponent(mProcessingButton);
    mInformation.setEditable(false);
    mInformation.setInterceptsMouseClicks(false, false);
    addChildComponent(mInformation);
    
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::key:
            case AttrType::description:
            case AttrType::state:
            case AttrType::height:
            case AttrType::propertyState:
            case AttrType::results:
            case AttrType::warnings:
                break;
            case AttrType::processing:
            {
                auto const state = acsr.getAttr<AttrType::processing>();
                mProcessingButton.setActive(state);
                mProcessingButton.setVisible(state);
                mProcessingButton.setTooltip(state ? juce::translate("Processing analysis...") : juce::translate("Analysis finished!"));
            }
                break;
            case AttrType::colours:
            case AttrType::time:
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
    
    mRenderer.onUpdated = [this]()
    {
        repaint();
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::binZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
    mTimeZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
}

Analyzer::Snapshot::~Snapshot()
{
    mTimeZoomAccessor.removeListener(mZoomListener);
    mAccessor.getAccessor<AcsrType::binZoom>(0).removeListener(mZoomListener);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Analyzer::Snapshot::resized()
{
    mInformation.setBounds(getLocalBounds().removeFromRight(200).removeFromTop(80));
    mProcessingButton.setBounds(8, 8, 20, 20);
}

void Analyzer::Snapshot::paint(juce::Graphics& g)
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

ANALYSE_FILE_END
