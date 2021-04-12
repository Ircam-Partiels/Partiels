#include "AnlGroupStrechableSection.h"

ANALYSE_FILE_BEGIN

Group::StrechableSection::StrechableSection(Accessor& accessor, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr)
: TrackManager<std::unique_ptr<Track::Section>>(accessor)
, mAccessor(accessor)
, mTransportAccessor(transportAcsr)
, mTimeZoomAccessor(timeZoomAcsr)
{
    mListener.onAttrChanged = [this](Group::Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::height:
            case AttrType::colour:
            case AttrType::layout:
            case AttrType::tracks:
                break;
            case AttrType::expanded:
            {
                mConcertinaTable.setOpen(mAccessor.getAttr<AttrType::expanded>(), true);
            }
                break;
}
    };
    
    mDraggableTable.onComponentDragged = [&](size_t previousIndex, size_t nextIndex)
    {
        auto layout = mAccessor.getAttr<AttrType::layout>();
        auto const identifier = layout[previousIndex];
        std::erase(layout, identifier);
        layout.insert(layout.begin() + static_cast<long>(nextIndex), identifier);
        
        std::erase_if(layout, [&](auto const& trackIdentifier)
        {
            return !hasTrack(trackIdentifier);
        });
        mAccessor.setAttr<AttrType::layout>(layout, NotificationType::synchronous);
    };
    
    mBoundsListener.onComponentResized = [&](juce::Component& component)
    {
        juce::ignoreUnused(component);
        auto const height = mSection.getHeight() + mConcertinaTable.getHeight();
        setSize(getWidth(), height);
    };
    mBoundsListener.attachTo(mSection);
    mBoundsListener.attachTo(mConcertinaTable);
    mConcertinaTable.setComponents({mDraggableTable});
    
    addAndMakeVisible(mSection);
    addAndMakeVisible(mConcertinaTable);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::StrechableSection::~StrechableSection()
{
    mBoundsListener.detachFrom(mConcertinaTable);
    mAccessor.removeListener(mListener);
}

void Group::StrechableSection::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    mSection.setBounds(bounds.removeFromTop(mSection.getHeight()));
    mConcertinaTable.setBounds(bounds.removeFromTop(mConcertinaTable.getHeight()));
}

void Group::StrechableSection::updateContentsEnded()
{
    auto const& groupContent = getGroupContents();
    std::vector<ConcertinaTable::ComponentRef> components;
    components.reserve(groupContent.size());
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    for(auto const& identifier : layout)
    {
        auto it = groupContent.find(identifier);
        if(it != groupContent.cend())
        {
            components.push_back(*it->second.get());
        }
    }
    mDraggableTable.setComponents(components);
    mSection.setVisible(!components.empty());
    resized();
}

void Group::StrechableSection::removeFromContents(std::unique_ptr<Track::Section>& content)
{
    juce::ignoreUnused(content);
}

std::unique_ptr<Track::Section> Group::StrechableSection::createForContents(Track::Accessor& trackAccessor)
{
    auto newSection = std::make_unique<Track::Section>(trackAccessor, mTimeZoomAccessor, mTransportAccessor);
    anlStrongAssert(newSection != nullptr);
    if(newSection != nullptr)
    {
        newSection->onRemove = [&]()
        {
            if(onRemoveTrack != nullptr)
            {
                onRemoveTrack(trackAccessor.getAttr<Track::AttrType::identifier>());
            }
        };
    }
    return newSection;
}

ANALYSE_FILE_END
