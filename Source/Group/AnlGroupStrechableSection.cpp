#include "AnlGroupStrechableSection.h"

ANALYSE_FILE_BEGIN

Group::StrechableSection::StrechableSection(Accessor& accessor, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr)
: mAccessor(accessor)
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
            case AttrType::focused:
                break;
            case AttrType::layout:
            case AttrType::tracks:
            {
                mTrackSections.updateContents(
                    mAccessor,
                    [this](Track::Accessor& trackAccessor)
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
                    },
                    nullptr);

                auto const& contents = mTrackSections.getContents();
                std::vector<ConcertinaTable::ComponentRef> components;
                components.reserve(contents.size());
                auto const& layout = mAccessor.getAttr<AttrType::layout>();
                for(auto const& identifier : layout)
                {
                    auto it = contents.find(identifier);
                    if(it != contents.cend())
                    {
                        components.push_back(*it->second.get());
                    }
                }
                mDraggableTable.setComponents(components);
                resized();
            }
            break;
            case AttrType::expanded:
            {
                mConcertinaTable.setOpen(acsr.getAttr<AttrType::expanded>(), true);
            }
            break;
        }
    };

    mSection.onRemove = [&]()
    {
        if(onRemoveGroup != nullptr)
        {
            onRemoveGroup();
        }
    };

    mSection.onTrackInserted = [&](juce::String const& identifier)
    {
        if(onTrackInserted != nullptr)
        {
            onTrackInserted(identifier);
        }
    };

    mDraggableTable.onComponentDropped = [&](juce::String const& identifier, size_t index)
    {
        auto layout = mAccessor.getAttr<AttrType::layout>();
        std::erase(layout, identifier);
        layout.insert(layout.begin() + static_cast<long>(index), identifier);
        mAccessor.setAttr<AttrType::layout>(layout, NotificationType::synchronous);
    };

    mBoundsListener.onComponentResized = [&](juce::Component& component)
    {
        juce::ignoreUnused(component);
        auto const height = mSection.getHeight() + mConcertinaTable.getHeight();
        setSize(getWidth(), height);
    };

    setFocusContainer(true);

    mBoundsListener.attachTo(mSection);
    mBoundsListener.attachTo(mConcertinaTable);
    mConcertinaTable.setComponents({mDraggableTable});
    mConcertinaTable.setOpen(mAccessor.getAttr<AttrType::expanded>(), false);

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

juce::KeyboardFocusTraverser* Group::StrechableSection::createFocusTraverser()
{
    class FocusTraverser
    : public juce::KeyboardFocusTraverser
    {
    public:
        FocusTraverser(StrechableSection& strechableSection)
        : mStrechableSection(strechableSection)
        {
        }

        ~FocusTraverser() override = default;

        juce::Component* getNextComponent(juce::Component* current) override
        {
            auto const isOpen = mStrechableSection.mConcertinaTable.isOpen();
            auto const wcontents = mStrechableSection.mDraggableTable.getComponents();
            if(current == nullptr || !isOpen || wcontents.empty())
            {
                auto* parent = mStrechableSection.getParentComponent();
                if(parent != nullptr)
                {
                    if(auto* parentFocusTraverser = parent->createFocusTraverser())
                    {
                        return parentFocusTraverser->getNextComponent(&mStrechableSection);
                    }
                }
                return juce::KeyboardFocusTraverser::getNextComponent(&mStrechableSection);
            }
            auto& section = mStrechableSection.mSection;
            if(current == &section || section.isParentOf(current))
            {
                auto next = wcontents.begin()->getComponent();
                if(next != nullptr)
                {
                    return next;
                }
            }
            else
            {
                auto it = std::find_if(wcontents.begin(), wcontents.end(), [&](auto const& wcontent)
                                       {
                                           auto* content = wcontent.getComponent();
                                           return content != nullptr && (content == current || content->isParentOf(current));
                                       });
                if(it != wcontents.end() && std::next(it) != wcontents.end())
                {
                    return std::next(it)->getComponent();
                }
            }
            auto* parent = mStrechableSection.getParentComponent();
            if(parent != nullptr)
            {
                if(auto* parentFocusTraverser = parent->createFocusTraverser())
                {
                    return parentFocusTraverser->getNextComponent(&mStrechableSection);
                }
            }
            return juce::KeyboardFocusTraverser::getNextComponent(&mStrechableSection);
        }

    private:
        StrechableSection& mStrechableSection;
    };

    return std::make_unique<FocusTraverser>(*this).release();
}

ANALYSE_FILE_END
