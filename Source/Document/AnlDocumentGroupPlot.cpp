#include "AnlDocumentGroupPlot.h"

ANALYSE_FILE_BEGIN

Document::GroupPlot::GroupPlot(Accessor& accessor)
: mAccessor(accessor)
, mZoomPlayhead(mAccessor.getAccessor<AcsrType::timeZoom>(0), {})
{
    addChildComponent(mProcessingButton);
    addAndMakeVisible(mZoomPlayhead);
    
    auto updateLayout = [&]()
    {
        auto const& layout = mAccessor.getAttr<AttrType::layout>();
        auto const& trackAcsrs = mAccessor.getAccessors<AcsrType::tracks>();
        std::erase_if(mPlots, [&](auto const& pair)
        {
            return std::binary_search(layout.cbegin(), layout.cend(), pair.first) ||
            std::none_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
            {
                return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == pair.first;
            });
        });
        
        removeAllChildren();
        for(auto const& identifier : layout)
        {
            auto plotIt = mPlots.find(identifier);
            if(plotIt == mPlots.cend())
            {
                auto trackIt = std::find_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                {
                    return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
                });
                if(trackIt != trackAcsrs.cend())
                {
                    auto plot = std::make_unique<Track::Plot>(*trackIt, mAccessor.getAccessor<AcsrType::timeZoom>(0));
                    anlStrongAssert(plot != nullptr);
                    if(plot != nullptr)
                    {
                        addAndMakeVisible(plot.get(), 0);
                        mPlots[identifier] = std::move(plot);
                    }
                }
            }
            else
            {
                addAndMakeVisible(plotIt->second.get(), 0);
            }
        }

        addAndMakeVisible(mProcessingButton, -1);
        addAndMakeVisible(mZoomPlayhead, -1);
        resized();
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
                //setSize(getWidth(), acsr.getAttr<AttrType::layoutVertical>());
            }
                break;
            case AttrType::layout:
            {
                updateLayout();
            }
                break;
        }
    };
    
    mListener.onAccessorInserted = [=](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr, index);
        switch(type)
        {
            case AcsrType::timeZoom:
                break;
            case AcsrType::tracks:
            {
                updateLayout();
            }
                break;
        }
    };
    
    mListener.onAccessorErased = [=](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr, index);
        switch(type)
        {
            case AcsrType::timeZoom:
                break;
            case AcsrType::tracks:
            {
                updateLayout();
            }
                break;
        }
    };
    
    setSize(100, 80);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::GroupPlot::~GroupPlot()
{
    mAccessor.removeListener(mListener);
}

void Document::GroupPlot::resized()
{
    auto bounds = getLocalBounds();
    mProcessingButton.setBounds(8, 8, 20, 20);
    
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    for(auto const& identifier : layout)
    {
        auto it = mPlots.find(identifier);
        if(it != mPlots.cend() && it->second != nullptr)
        {
            it->second->setBounds(bounds);
        }
    }
}

ANALYSE_FILE_END
