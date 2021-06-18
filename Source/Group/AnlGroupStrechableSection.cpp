#include "AnlGroupStrechableSection.h"

ANALYSE_FILE_BEGIN

Group::StrechableSection::StrechableSection(Director& director, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr)
: mDirector(director)
, mTransportAccessor(transportAcsr)
, mTimeZoomAccessor(timeZoomAcsr)
, mLayoutNotifier(mAccessor, [this]()
                  {
                      updateContent();
                  })
{
    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::height:
            case AttrType::colour:
            case AttrType::focused:
            case AttrType::layout:
            case AttrType::tracks:
                break;
            case AttrType::expanded:
            {
                mConcertinaTable.setOpen(acsr.getAttr<AttrType::expanded>(), true);
            }
            break;
        }
    };

    mSection.onTrackInserted = [&](juce::String const& identifier, bool copy)
    {
        if(onTrackInserted != nullptr)
        {
            onTrackInserted(identifier, 0_z, copy);
        }
    };

    mDraggableTable.onComponentDropped = [&](juce::String const& identifier, size_t index, bool copy)
    {
        if(copy)
        {
            if(onTrackInserted != nullptr)
            {
                onTrackInserted(identifier, index, true);
            }
        }
        else
        {
            mDirector.startAction();
            auto layout = copy_with_erased(mAccessor.getAttr<AttrType::layout>(), identifier);
            layout.insert(layout.begin() + static_cast<long>(index), identifier);
            mAccessor.setAttr<AttrType::layout>(layout, NotificationType::synchronous);
            mDirector.endAction(ActionState::newTransaction, juce::translate("Move Track"));
        }
    };

    mBoundsListener.onComponentResized = [&](juce::Component& component)
    {
        juce::ignoreUnused(component);
        auto const height = mSection.getHeight() + mConcertinaTable.getHeight();
        setSize(getWidth(), height);
    };

    setFocusContainerType(juce::Component::FocusContainerType::keyboardFocusContainer);

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
    mBoundsListener.detachFrom(mSection);
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

juce::Rectangle<int> Group::StrechableSection::getPlotBounds(juce::String const& identifier) const
{
    if(identifier == mAccessor.getAttr<AttrType::identifier>())
    {
        return mSection.getPlotBounds();
    }
    if(Tools::hasTrackAcsr(mAccessor, identifier))
    {
        auto const& contents = mTrackSections.getContents();
        auto it = contents.find(identifier);
        if(it != contents.end() && it->second != nullptr)
        {
            return it->second->getPlotBounds();
        }
    }
    anlWeakAssert(false);
    return {};
}

std::unique_ptr<juce::ComponentTraverser> Group::StrechableSection::createFocusTraverser()
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
            if(auto parentFocusTraverser = getParentFocusTraverser())
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
            if(auto parentFocusTraverser = getParentFocusTraverser())
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

        std::unique_ptr<juce::ComponentTraverser> getParentFocusTraverser()
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

    return std::make_unique<FocusTraverser>(*this);
}

void Group::StrechableSection::updateContent()
{
    mTrackSections.updateContents(
        mAccessor,
        [this](Track::Accessor& trackAccessor)
        {
            auto const identifier = trackAccessor.getAttr<Track::AttrType::identifier>();
            auto& trackDirector = mDirector.getTrackDirector(identifier);
            return std::make_unique<Track::Section>(trackDirector, mTimeZoomAccessor, mTransportAccessor);
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

ANALYSE_FILE_END
