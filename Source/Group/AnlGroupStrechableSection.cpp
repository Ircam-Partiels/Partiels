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
                        return std::make_unique<Track::Section>(trackAccessor, mTimeZoomAccessor, mTransportAccessor);
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

void Group::StrechableSection::moveKeyboardFocusTo(juce::String const& identifier)
{
    auto const isOpen = mConcertinaTable.isOpen();
    if(mAccessor.getAttr<AttrType::identifier>() == identifier || !isOpen)
    {
        mSection.grabKeyboardFocus();
    }
    else
    {
        anlStrongAssert(Tools::hasTrackAcsr(mAccessor, identifier));
        auto const& contents = mTrackSections.getContents();
        auto it = contents.find(identifier);
        anlStrongAssert(it != contents.end());
        if(it != contents.end() && it->second != nullptr)
        {
            it->second->grabKeyboardFocus();
        }
    }
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
            if(auto* content = getNextTrackOrOverview(current))
            {
                return content;
            }
            if(auto* parentFocusTraverser = getParentFocusTraverser())
            {
                return parentFocusTraverser->getNextComponent(&mStrechableSection);
            }
            return &mStrechableSection.mSection;
        }

        juce::Component* getPreviousComponent(juce::Component* current) override
        {
            if(auto* content = getPreviousTrackOrOverview(current))
            {
                return content;
            }
            if(auto* parentFocusTraverser = getParentFocusTraverser())
            {
                return parentFocusTraverser->getPreviousComponent(&mStrechableSection);
            }
            return &mStrechableSection.mSection;
        }

        bool isOverview(juce::Component* component) const
        {
            return &mStrechableSection.mSection == component || mStrechableSection.mSection.isParentOf(component);
        }

        juce::Component* getNextTrackOrOverview(juce::Component* component)
        {
            if(component == nullptr)
            {
                return &mStrechableSection.mSection;
            }
            auto const isOpen = mStrechableSection.mConcertinaTable.isOpen();
            auto const contents = mStrechableSection.mDraggableTable.getComponents();
            if(!isOpen || contents.empty())
            {
                return nullptr;
            }
            if(isOverview(component) && *contents.begin() != nullptr)
            {
                return contents.begin()->getComponent();
            }
            auto it = std::find_if(contents.begin(), contents.end(), [&](auto const& content)
                                   {
                                       return content != nullptr && (content == component || content->isParentOf(component));
                                   });
            while(it != contents.end() && std::next(it) != contents.end())
            {
                it = std::next(it);
                if(*it != nullptr)
                {
                    return it->getComponent();
                }
            }
            return nullptr;
        }

        juce::Component* getPreviousTrackOrOverview(juce::Component* component)
        {
            auto const isOpen = mStrechableSection.mConcertinaTable.isOpen();
            auto const contents = mStrechableSection.mDraggableTable.getComponents();
            if(!isOpen || contents.empty() || isOverview(component))
            {
                return nullptr;
            }
            auto it = std::find_if(contents.rbegin(), contents.rend(), [&](auto const& content)
                                   {
                                       return content != nullptr && (content == component || content->isParentOf(component));
                                   });
            if(it == contents.rend())
            {
                return &mStrechableSection.mSection;
            }
            while(it != contents.rend())
            {
                it = std::next(it);
                if(*it != nullptr)
                {
                    return it->getComponent();
                }
            }
            return nullptr;
        }

        juce::KeyboardFocusTraverser* getParentFocusTraverser()
        {
            auto* parent = mStrechableSection.getParentComponent();
            if(parent != nullptr)
            {
                return parent->createFocusTraverser();
            }
            return nullptr;
        }

    private:
        StrechableSection& mStrechableSection;
    };

    return std::make_unique<FocusTraverser>(*this).release();
}

ANALYSE_FILE_END
