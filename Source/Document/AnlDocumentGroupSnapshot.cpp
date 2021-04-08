#include "AnlDocumentGroupSnapshot.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

Document::GroupSnapshot::GroupSnapshot(Accessor& accessor)
: mAccessor(accessor)
{
    auto updateLayout = [&]()
    {
        auto const& layout = mAccessor.getAttr<AttrType::layout>();
        std::erase_if(mSnapshots, [&](auto const& pair)
        {
            return !std::binary_search(layout.cbegin(), layout.cend(), pair.first) || !Tools::hasTrack(mAccessor, pair.first);
        });
        
        removeAllChildren();
        auto& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
        for(auto const& identifier : layout)
        {
            auto plotIt = mSnapshots.find(identifier);
            if(plotIt == mSnapshots.cend())
            {
                auto trackAcsr = Tools::getTrack(mAccessor, identifier);
                if(trackAcsr)
                {
                    auto plot = std::make_unique<Track::Snapshot>(*trackAcsr, timeZoomAcsr);
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
            case AcsrType::transport:
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
            case AcsrType::transport:
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

Document::GroupSnapshot::Overlay::Overlay(GroupSnapshot& snapshot)
: mSnapshot(snapshot)
, mAccessor(mSnapshot.mAccessor)
{
    addAndMakeVisible(mSnapshot);
    addChildComponent(mProcessingButton);
    setInterceptsMouseClicks(true, true);
    
    mListener.onAttrChanged = [](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::file:
            case AttrType::layoutHorizontal:
            case AttrType::layoutVertical:
            case AttrType::expanded:
            case AttrType::layout:
                break;
        }
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::GroupSnapshot::Overlay::~Overlay()
{
    mAccessor.removeListener(mListener);
}

void Document::GroupSnapshot::Overlay::resized()
{
    auto bounds = getLocalBounds();
    mSnapshot.setBounds(bounds);
    mProcessingButton.setBounds(8, 8, 20, 20);
}

void Document::GroupSnapshot::Overlay::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);
}

void Document::GroupSnapshot::Overlay::mouseMove(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
}

void Document::GroupSnapshot::Overlay::mouseEnter(juce::MouseEvent const& event)
{
    mouseMove(event);
}

void Document::GroupSnapshot::Overlay::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
}

ANALYSE_FILE_END
