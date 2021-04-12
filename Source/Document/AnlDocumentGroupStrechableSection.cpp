#include "AnlDocumentGroupStrechableSection.h"

ANALYSE_FILE_BEGIN

Document::GroupStrechableSection::GroupStrechableSection(Accessor& accessor)
: GroupContainer<std::unique_ptr<Track::Section>>(accessor)
, mAccessor(accessor)
{
    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::file:
            case AttrType::layout:
                break;
            case AttrType::layoutVertical:
            {
                resized();
            }
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
        
        auto const trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
        std::erase_if(layout, [&](auto const& trackIdentifier)
        {
            return std::none_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
            {
                return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == trackIdentifier;
            });
        });
        anlWeakAssert(layout.size() == trackAcsrs.size());
        mAccessor.setAttr<AttrType::layout>(layout, NotificationType::synchronous);
    };
    
    mBoundsListener.onComponentResized = [&](juce::Component& component)
    {
        juce::ignoreUnused(component);
        auto const height = mGroupSection.getHeight() + mConcertinaTable.getHeight();
        setSize(getWidth(), height);
    };
    mBoundsListener.attachTo(mGroupSection);
    mBoundsListener.attachTo(mConcertinaTable);
    mConcertinaTable.setComponents({mDraggableTable});
    
    addAndMakeVisible(mGroupSection);
    addAndMakeVisible(mConcertinaTable);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::GroupStrechableSection::~GroupStrechableSection()
{
    mBoundsListener.detachFrom(mConcertinaTable);
    mAccessor.removeListener(mListener);
}

void Document::GroupStrechableSection::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    mGroupSection.setBounds(bounds.removeFromTop(mGroupSection.getHeight()));
    mConcertinaTable.setBounds(bounds.removeFromTop(mConcertinaTable.getHeight()));
}

void Document::GroupStrechableSection::updateEnded()
{
    auto const& groupContent = getGroupContent();
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
    mGroupSection.setVisible(!components.empty());
    resized();
}

void Document::GroupStrechableSection::removeFromGroup(std::unique_ptr<Track::Section>& value)
{
    juce::ignoreUnused(value);
}

std::unique_ptr<Track::Section> Document::GroupStrechableSection::createForGroup(Track::Accessor& trackAccessor)
{
    auto& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
    auto& transport = mAccessor.getAcsr<AcsrType::transport>();
    auto newSection = std::make_unique<Track::Section>(trackAccessor, timeZoomAcsr, transport);
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
