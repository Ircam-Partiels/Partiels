#include "AnlGroupPlot.h"

ANALYSE_FILE_BEGIN

Group::Plot::Plot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
{
    addChildComponent(mProcessingButton);
    addAndMakeVisible(mZoomPlayhead);
    
    mListener.onAttrChanged = [=](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
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
    
    mAnalyzerListener.onAttrChanged = [this](Analyzer::Accessor const& acsr, Analyzer::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Analyzer::AttrType::identifier:
            case Analyzer::AttrType::name:
            case Analyzer::AttrType::key:
            case Analyzer::AttrType::description:
            case Analyzer::AttrType::state:
            case Analyzer::AttrType::height:
            case Analyzer::AttrType::results:
            case Analyzer::AttrType::propertyState:
            case Analyzer::AttrType::warnings:
            case Analyzer::AttrType::time:
                break;
            case Analyzer::AttrType::colours:
            {
                repaint();
            }
                break;
            case Analyzer::AttrType::processing:
            {
                auto const state = std::any_of(mRenderers.cbegin(), mRenderers.cend(), [](auto const& renderer)
                {
                    return std::get<0>(renderer).get().template getAttr<Analyzer::AttrType::processing>();
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
        auto& anlAcsr = std::get<0>(renderer).get();
        anlAcsr.getAccessor<Analyzer::AcsrType::binZoom>(0).removeListener(mZoomListener);
        anlAcsr.getAccessor<Analyzer::AcsrType::valueZoom>(0).removeListener(mZoomListener);
        anlAcsr.removeListener(mAnalyzerListener);
    }
    mTimeZoomAccessor.removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Group::Plot::addAnalyzer(Analyzer::Accessor& anlAcsr)
{
    auto newRenderer = std::make_unique<Analyzer::Renderer>(anlAcsr, Analyzer::Renderer::Type::range);
    anlStrongAssert(newRenderer != nullptr);
    if(newRenderer != nullptr)
    {
        newRenderer->onUpdated = [this]()
        {
            repaint();
        };
        
    }
    anlAcsr.addListener(mAnalyzerListener, NotificationType::synchronous);
    anlAcsr.getAccessor<Analyzer::AcsrType::valueZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
    anlAcsr.getAccessor<Analyzer::AcsrType::binZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
    mRenderers.emplace(anlAcsr, std::move(newRenderer));
    repaint();
}

void Group::Plot::removeAnalyzer(Analyzer::Accessor& anlAcsr)
{
    anlAcsr.getAccessor<Analyzer::AcsrType::binZoom>(0).removeListener(mZoomListener);
    anlAcsr.getAccessor<Analyzer::AcsrType::valueZoom>(0).removeListener(mZoomListener);
    anlAcsr.removeListener(mAnalyzerListener);
    mRenderers.erase(anlAcsr);
    repaint();
}

void Group::Plot::resized()
{
    mZoomPlayhead.setBounds(getLocalBounds().reduced(2));
    mProcessingButton.setBounds(8, 8, 20, 20);
}

void Group::Plot::paint(juce::Graphics& g)
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
    
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    for(auto lit = layout.crbegin(); lit != layout.crend(); ++lit)
    {
        auto const& identifier = *lit;
        auto it = std::find_if(mRenderers.cbegin(), mRenderers.cend(), [&](auto const& pair)
        {
            return pair.first.get().template getAttr<Analyzer::AttrType::identifier>() == identifier;
        });
        if(it != mRenderers.cend() && (*it).second != nullptr)
        {
            (*it).second->paint(g, bounds, mTimeZoomAccessor);
        }
    }
}

ANALYSE_FILE_END
