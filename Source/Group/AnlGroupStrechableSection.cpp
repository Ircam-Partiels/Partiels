#include "AnlGroupStrechableSection.h"

ANALYSE_FILE_BEGIN

Group::StrechableSection::StrechableSection(Director& director, juce::ApplicationCommandManager& commandManager, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr, ResizerFn resizerFn)
: mDirector(director)
, mTransportAccessor(transportAcsr)
, mTimeZoomAccessor(timeZoomAcsr)
, mApplicationCommandManager(commandManager)
, mResizerFn(resizerFn)
, mSection(director, commandManager, transportAcsr, timeZoomAcsr, resizerFn)
, mLayoutNotifier(mAccessor, [this]()
                  {
                      updateContent();
                  })
{
    mConcertinaTable.setComponents({mDraggableTable});
    mConcertinaTable.setOpen(mAccessor.getAttr<AttrType::expanded>(), false);

    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::height:
            case AttrType::colour:
            case AttrType::focused:
            case AttrType::layout:
            case AttrType::tracks:
            case AttrType::referenceid:
                break;
            case AttrType::expanded:
            {
                if(!mCanAnimate)
                {
                    mConcertinaTable.setOpen(acsr.getAttr<AttrType::expanded>(), false);
                    if(onLayoutChanged != nullptr)
                    {
                        onLayoutChanged();
                    }
                }
                else
                {
                    if(acsr.getAttr<AttrType::expanded>())
                    {
                        if(onLayoutChanged != nullptr)
                        {
                            onLayoutChanged();
                        }
                        startTimer(100);
                    }
                    else
                    {
                        mConcertinaTable.setOpen(false, true);
                    }
                }
                updateResizable();
            }
            break;
        }
    };

    mSection.onTrackInserted = [this](juce::String const& identifier, bool copy)
    {
        if(onTrackInserted != nullptr)
        {
            onTrackInserted(identifier, 0_z, copy);
        }
    };
    mSection.addMouseListener(this, false);

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
            mDirector.startAction(false);
            auto layout = copy_with_erased(mAccessor.getAttr<AttrType::layout>(), identifier);
            layout.insert(layout.begin() + static_cast<long>(index), identifier);
            mAccessor.setAttr<AttrType::layout>(layout, NotificationType::synchronous);
            mDirector.endAction(false, ActionState::newTransaction, juce::translate("Move Track"));
        }
    };

    mComponentListener.onComponentResized = [&]([[maybe_unused]] juce::Component& component)
    {
        auto const height = mSection.getHeight() + mConcertinaTable.getHeight();
        setSize(getWidth(), height);
    };

    mComponentListener.attachTo(mSection);
    mComponentListener.attachTo(mConcertinaTable);

    addAndMakeVisible(mSection);
    addAndMakeVisible(mConcertinaTable);
    mConcertinaTable.addChangeListener(this);
    mAccessor.addListener(mListener, NotificationType::synchronous);

    auto const height = mSection.getHeight() + mConcertinaTable.getHeight();
    setSize(200, height);
}

Group::StrechableSection::~StrechableSection()
{
    mSection.removeMouseListener(this);
    mComponentListener.detachFrom(mConcertinaTable);
    mComponentListener.detachFrom(mSection);
    mAccessor.removeListener(mListener);
    mConcertinaTable.removeChangeListener(this);
}

void Group::StrechableSection::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    mSection.setBounds(bounds.removeFromTop(mSection.getHeight()));
    mConcertinaTable.setBounds(bounds.removeFromTop(mConcertinaTable.getHeight()));
}

void Group::StrechableSection::changeListenerCallback([[maybe_unused]] juce::ChangeBroadcaster* source)
{
    MiscWeakAssert(source == std::addressof(mConcertinaTable));
    if(!mConcertinaTable.isAnimating() && !mConcertinaTable.isOpen())
    {
        if(onLayoutChanged != nullptr)
        {
            onLayoutChanged();
        }
    }
}

void Group::StrechableSection::timerCallback()
{
    stopTimer();
    mConcertinaTable.setOpen(mAccessor.getAttr<AttrType::expanded>(), true);
}

bool Group::StrechableSection::isStretching() const
{
    return mConcertinaTable.isAnimating();
}

juce::Component const& Group::StrechableSection::getSection(juce::String const& identifier) const
{
    if(identifier == mAccessor.getAttr<AttrType::identifier>())
    {
        return mSection;
    }
    if(Tools::hasTrackAcsr(mAccessor, identifier))
    {
        auto const& contents = mTrackSections.getContents();
        auto it = contents.find(identifier);
        if(it != contents.end() && it->second != nullptr)
        {
            return *(it->second);
        }
    }
    MiscWeakAssert(false);
    return *this;
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

void Group::StrechableSection::setLastItemResizable(bool state)
{
    if(mIsResizable != state)
    {
        mIsResizable = state;
        updateResizable();
    }
}

void Group::StrechableSection::setCanAnimate(bool state)
{
    mCanAnimate = state;
    if(!state)
    {
        mConcertinaTable.setOpen(mConcertinaTable.isOpen(), false);
    }
}

void Group::StrechableSection::updateContent()
{
    mTrackSections.updateContents(
        mAccessor,
        [this](Track::Accessor& trackAccessor)
        {
            auto const identifier = trackAccessor.getAttr<Track::AttrType::identifier>();
            auto& trackDirector = mDirector.getTrackDirector(identifier);
            auto trackSection = std::make_unique<Track::Section>(trackDirector, mApplicationCommandManager, mTimeZoomAccessor, mTransportAccessor, mResizerFn);
            MiscWeakAssert(trackSection != nullptr);
            if(trackSection != nullptr)
            {
                trackSection->addMouseListener(this, false);
            }
            return trackSection;
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
            components.push_back(std::ref(*it->second.get()));
        }
    }
    mDraggableTable.setComponents(components);
    updateResizable();

    resized();
    if(onLayoutChanged != nullptr)
    {
        onLayoutChanged();
    }
}

void Group::StrechableSection::updateResizable()
{
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    auto const& contents = mTrackSections.getContents();
    auto const hasTrackVisible = mAccessor.getAttr<AttrType::expanded>() && !layout.empty();
    mSection.setResizable(hasTrackVisible || mIsResizable);
    for(auto const& identifier : layout)
    {
        auto it = contents.find(identifier);
        if(it != contents.cend())
        {
            it->second->setResizable(true);
        }
    }
    if(!layout.empty())
    {
        auto it = contents.find(layout.back());
        if(it != contents.cend())
        {
            it->second->setResizable(hasTrackVisible && mIsResizable);
        }
    }
}

ANALYSE_FILE_END
