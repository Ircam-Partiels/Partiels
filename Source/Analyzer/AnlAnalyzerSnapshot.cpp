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
    
    auto updateProcessingButton = [this]()
    {
        auto const state = mRenderer.isPreparing() || mAccessor.getAttr<AttrType::processing>();
        mProcessingButton.setActive(state);
        mProcessingButton.setVisible(state);
        mProcessingButton.setTooltip(state ? juce::translate("Processing analysis...") : juce::translate("Analysis finished!"));
    };
    
    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
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
            {
                mRenderer.prepareRendering();
                updateProcessingButton();
            };
                break;
            case AttrType::warnings:
                break;
            case AttrType::processing:
            {
                updateProcessingButton();
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
    
    mRenderer.onUpdated = [=, this]()
    {
        updateProcessingButton();
        repaint();
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).addListener(mValueZoomListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::binZoom>(0).addListener(mBinZoomListener, NotificationType::synchronous);
    mTimeZoomAccessor.addListener(mTimeZoomListener, NotificationType::synchronous);
}

Analyzer::Snapshot::~Snapshot()
{
    mTimeZoomAccessor.removeListener(mTimeZoomListener);
    mAccessor.getAccessor<AcsrType::binZoom>(0).removeListener(mBinZoomListener);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).removeListener(mValueZoomListener);
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
