#include "AnlDocumentPlot.h"

ANALYSE_FILE_BEGIN

Document::Plot::Plot(Accessor& accessor)
: mAccessor(accessor)
, mZoomPlayhead(mAccessor.getAccessor<AcsrType::timeZoom>(0), {2, 2, 2, 2})
{
    addChildComponent(mProcessingButton);
    addAndMakeVisible(mZoomPlayhead);
    
    mListener.onAttrChanged = [=](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::file:
            case AttrType::isLooping:
            case AttrType::gain:
            case AttrType::isPlaybackStarted:
                break;
            case AttrType::playheadPosition:
            {
                mZoomPlayhead.setPosition(acsr.getAttr<AttrType::playheadPosition>());
            }
                break;
            case AttrType::layoutHorizontal:
                break;
            case AttrType::layout:
            {
                repaint();
            }
                break;
        }
    };
    
    mListener.onAccessorInserted = [=](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::timeZoom:
                break;
            case AcsrType::analyzers:
            {
                auto& anlAcsr = mAccessor.getAccessor<AcsrType::analyzers>(index);
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
                mRenderers.emplace(mRenderers.begin() + static_cast<long>(index), anlAcsr, std::move(newRenderer));
                repaint();
            }
                break;
                
            default:
                break;
        }
    };
    
    mListener.onAccessorErased = [=](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::timeZoom:
                break;
            case AcsrType::analyzers:
            {
                anlStrongAssert(index < mRenderers.size());
                if(index >= mRenderers.size())
                {
                    return;
                }
                auto& anlAcsr = std::get<0>(mRenderers[index]).get();
                anlAcsr.getAccessor<Analyzer::AcsrType::binZoom>(0).removeListener(mZoomListener);
                anlAcsr.getAccessor<Analyzer::AcsrType::valueZoom>(0).removeListener(mZoomListener);
                anlAcsr.removeListener(mAnalyzerListener);
                mRenderers.erase(mRenderers.begin() + static_cast<long>(index));
                repaint();
            }
                break;
                
            default:
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
    mAccessor.getAccessor<AcsrType::timeZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
}

Document::Plot::~Plot()
{
    for(auto& renderer : mRenderers)
    {
        auto& anlAcsr = std::get<0>(renderer).get();
        anlAcsr.getAccessor<Analyzer::AcsrType::binZoom>(0).removeListener(mZoomListener);
        anlAcsr.getAccessor<Analyzer::AcsrType::valueZoom>(0).removeListener(mZoomListener);
        anlAcsr.removeListener(mAnalyzerListener);
    }
    mAccessor.getAccessor<AcsrType::timeZoom>(0).removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Document::Plot::resized()
{
    mZoomPlayhead.setBounds(getLocalBounds().reduced(2));
    mProcessingButton.setBounds(8, 8, 20, 20);
}

void Document::Plot::paint(juce::Graphics& g)
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
    
    
    auto const& timeZoomAcsr = mAccessor.getAccessor<AcsrType::timeZoom>(0);
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    for(auto lit = layout.crbegin(); lit != layout.crend(); ++lit)
    {
        auto const& identifier = *lit;
        auto it = std::find_if(mRenderers.cbegin(), mRenderers.cend(), [&](auto const& renderer)
        {
            return std::get<0>(renderer).get().template getAttr<Analyzer::AttrType::identifier>() == identifier;
        });
        if(it != mRenderers.cend() && std::get<1>(*it) != nullptr)
        {
            std::get<1>(*it)->paint(g, bounds, timeZoomAcsr);
        }
    }
}

ANALYSE_FILE_END
