#include "AnlDocumentPlot.h"

ANALYSE_FILE_BEGIN

Document::Plot::Plot(Accessor& accessor)
: mAccessor(accessor)
, mZoomPlayhead(mAccessor.getAccessor<AcsrType::timeZoom>(0), {2, 2, 2, 2})
{
    addChildComponent(mProcessingButton);
    addAndMakeVisible(mZoomPlayhead);
    addAndMakeVisible(mResizerBar);
    
    auto updateProcessingButton = [this]()
    {
        auto const state = std::any_of(mRenderers.cbegin(), mRenderers.cend(), [](auto const& renderer)
        {
            return std::get<0>(renderer).get().template getAttr<Track::AttrType::processing>() || std::get<1>(renderer)->isPreparing();
        });
        mProcessingButton.setActive(state);
        mProcessingButton.setVisible(state);
        mProcessingButton.setTooltip(state ? juce::translate("Processing analysis...") : juce::translate("Analysis finished!"));
        repaint();
    };
    
    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
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
            case AttrType::layoutVertical:
            {
                setSize(getWidth(), acsr.getAttr<AttrType::layoutVertical>());
            }
                break;
            case AttrType::layout:
            {
                repaint();
            }
                break;
        }
    };
    
    mListener.onAccessorInserted = [=, this](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::timeZoom:
                break;
            case AcsrType::analyzers:
            {
                auto& anlAcsr = mAccessor.getAccessor<AcsrType::analyzers>(index);
                auto newRenderer = std::make_unique<Track::Renderer>(anlAcsr, Track::Renderer::Type::range);
                anlStrongAssert(newRenderer != nullptr);
                if(newRenderer != nullptr)
                {
                    newRenderer->onUpdated = [=]()
                    {
                        updateProcessingButton();
                    };
                }
                auto it = mRenderers.emplace(mRenderers.begin() + static_cast<long>(index), anlAcsr, std::move(newRenderer));
                anlStrongAssert(it != mRenderers.cend());
                if(it != mRenderers.cend())
                {
                    anlAcsr.addListener(mAnalyzerListener, NotificationType::synchronous);
                    anlAcsr.getAccessor<Track::AcsrType::valueZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
                    anlAcsr.getAccessor<Track::AcsrType::binZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
                }
            }
                break;
                
            default:
                break;
        }
    };
    
    mListener.onAccessorErased = [=, this](Accessor const& acsr, AcsrType type, size_t index)
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
                anlAcsr.getAccessor<Track::AcsrType::binZoom>(0).removeListener(mZoomListener);
                anlAcsr.getAccessor<Track::AcsrType::valueZoom>(0).removeListener(mZoomListener);
                anlAcsr.removeListener(mAnalyzerListener);
                mRenderers.erase(mRenderers.begin() + static_cast<long>(index));
                repaint();
            }
                break;
                
            default:
                break;
        }
    };
    
    mZoomListener.onAttrChanged = [=, this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        for(auto& renderer : mRenderers)
        {
            if(&(std::get<0>(renderer).get().getAccessor<Track::AcsrType::valueZoom>(0)) == &acsr)
            {
                std::get<1>(renderer)->prepareRendering();
                updateProcessingButton();
            }
            else if(&(std::get<0>(renderer).get().getAccessor<Track::AcsrType::binZoom>(0)) == &acsr)
            {
                repaint();
            }
        }
    };
    
    mTimeZoomListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };
    
    mAnalyzerListener.onAttrChanged = [=, this](Track::Accessor const& acsr, Track::AttrType attribute)
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
            case Track::AttrType::propertyState:
            case Track::AttrType::warnings:
            case Track::AttrType::time:
                break;
            case Track::AttrType::results:
            case Track::AttrType::colours:
            {
                auto it = std::find_if(mRenderers.cbegin(), mRenderers.cend(), [&](auto const& renderer)
                {
                    return &(std::get<0>(renderer).get()) == &acsr;
                });
                anlStrongAssert(it != mRenderers.cend());
                if(it != mRenderers.cend())
                {
                    std::get<1>(*it)->prepareRendering();
                }
            }
                break;
            case Track::AttrType::processing:
            {
                updateProcessingButton();
            }
                break;
        }
    };
    
    mResizerBar.onMoved = [this](int size)
    {
        mAccessor.setAttr<AttrType::layoutVertical>(size + 2, NotificationType::synchronous);
    };
    
    setSize(100, 80);

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::timeZoom>(0).addListener(mTimeZoomListener, NotificationType::synchronous);
}

Document::Plot::~Plot()
{
    for(auto& renderer : mRenderers)
    {
        auto& anlAcsr = std::get<0>(renderer).get();
        anlAcsr.getAccessor<Track::AcsrType::binZoom>(0).removeListener(mZoomListener);
        anlAcsr.getAccessor<Track::AcsrType::valueZoom>(0).removeListener(mZoomListener);
        anlAcsr.removeListener(mAnalyzerListener);
    }
    mAccessor.getAccessor<AcsrType::timeZoom>(0).removeListener(mTimeZoomListener);
    mAccessor.removeListener(mListener);
}

void Document::Plot::resized()
{
    auto bounds = getLocalBounds();
    // Resizers
    {
        bounds.removeFromBottom(2);
        mResizerBar.setBounds(bounds.removeFromBottom(2));
    }
    mZoomPlayhead.setBounds(getLocalBounds().reduced(2));
    mProcessingButton.setBounds(8, 8, 20, 20);
}

void Document::Plot::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
    auto const bounds = getLocalBounds().withTrimmedBottom(2).reduced(2);
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
            return std::get<0>(renderer).get().template getAttr<Track::AttrType::identifier>() == identifier;
        });
        anlStrongAssert(it != mRenderers.cend());
        if(it != mRenderers.cend() && std::get<1>(*it) != nullptr)
        {
            if(lit == layout.crbegin())
            {
                auto const colours = std::get<0>(*it).get().getAttr<Track::AttrType::colours>();
                g.setColour(colours.background);
                g.fillRect(bounds);
            }
            
            std::get<1>(*it)->paint(g, bounds, timeZoomAcsr);
        }
    }
}

ANALYSE_FILE_END
