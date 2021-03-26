#include "AnlDocumentGroupSnapshot.h"

ANALYSE_FILE_BEGIN

Document::GroupSnapshot::GroupSnapshot(Accessor& accessor)
: mAccessor(accessor)
{
    auto updateLayout = [&]()
    {
        auto const& layout = mAccessor.getAttr<AttrType::layout>();
        auto const& trackAcsrs = mAccessor.getAccessors<AcsrType::tracks>();
        std::erase_if(mSnapshots, [&](auto const& pair)
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
            auto plotIt = mSnapshots.find(identifier);
            if(plotIt == mSnapshots.cend())
            {
                auto trackIt = std::find_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                {
                    return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
                });
                if(trackIt != trackAcsrs.cend())
                {
                    auto plot = std::make_unique<Track::Snapshot>(*trackIt, mAccessor.getAccessor<AcsrType::timeZoom>(0));
                    anlStrongAssert(plot != nullptr);
                    if(plot != nullptr)
                    {
                        addAndMakeVisible(plot.get(), 0);
                        mSnapshots[identifier] = std::move(plot);
                    }
                }
            }
            else
            {
                addAndMakeVisible(plotIt->second.get(), 0);
            }
        }
        
        resized();
    };
    
    mListener.onAttrChanged = [=](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::file:
            case AttrType::isLooping:
            case AttrType::gain:
            case AttrType::isPlaybackStarted:
            case AttrType::playheadPosition:
            case AttrType::layoutHorizontal:
            case AttrType::layoutVertical:
            case AttrType::expanded:
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

Document::GroupSnapshot::~GroupSnapshot()
{
    mAccessor.removeListener(mListener);
}

void Document::GroupSnapshot::resized()
{
    auto bounds = getLocalBounds();
    
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    for(auto const& identifier : layout)
    {
        auto it = mSnapshots.find(identifier);
        if(it != mSnapshots.cend() && it->second != nullptr)
        {
            it->second->setBounds(bounds);
        }
    }
}

ANALYSE_FILE_END
