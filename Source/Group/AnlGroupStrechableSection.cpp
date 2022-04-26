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
            case AttrType::zoomid:
                break;
            case AttrType::expanded:
            {
                mConcertinaTable.setOpen(acsr.getAttr<AttrType::expanded>(), true);
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

    mComponentListener.onComponentResized = [&](juce::Component& component)
    {
        juce::ignoreUnused(component);
        auto const height = mSection.getHeight() + mConcertinaTable.getHeight();
        setSize(getWidth(), height);
    };

    mComponentListener.attachTo(mSection);
    mComponentListener.attachTo(mConcertinaTable);
    mConcertinaTable.setComponents({mDraggableTable});
    mConcertinaTable.setOpen(mAccessor.getAttr<AttrType::expanded>(), false);

    addAndMakeVisible(mSection);
    addAndMakeVisible(mConcertinaTable);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::StrechableSection::~StrechableSection()
{
    mSection.removeMouseListener(this);
    mComponentListener.detachFrom(mConcertinaTable);
    mComponentListener.detachFrom(mSection);
    mAccessor.removeListener(mListener);
}

void Group::StrechableSection::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    mSection.setBounds(bounds.removeFromTop(mSection.getHeight()));
    mConcertinaTable.setBounds(bounds.removeFromTop(mConcertinaTable.getHeight()));
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

void Group::StrechableSection::setResizable(bool state)
{
    if(mIsResizable != state)
    {
        mIsResizable = state;
        mSection.setResizable(state);
        for(auto& trackSection : mTrackSections.getContents())
        {
            if(trackSection.second != nullptr)
            {
                trackSection.second->setResizable(state);
            }
        }
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
            auto trackSection = std::make_unique<Track::Section>(trackDirector, mTimeZoomAccessor, mTransportAccessor);
            if(trackSection != nullptr)
            {
                trackSection->setResizable(mIsResizable);
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
            components.push_back(*it->second.get());
        }
    }
    mDraggableTable.setComponents(components);
    resized();
}

ANALYSE_FILE_END
