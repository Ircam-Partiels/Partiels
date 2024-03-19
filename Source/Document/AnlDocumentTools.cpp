#include "AnlDocumentTools.h"
#include "../Group/AnlGroupTools.h"

ANALYSE_FILE_BEGIN

bool Document::Tools::hasItem(Accessor const& accessor, juce::String const& identifier)
{
    return hasGroupAcsr(accessor, identifier) || (hasTrackAcsr(accessor, identifier) && isTrackInGroup(accessor, identifier));
}

bool Document::Tools::hasTrackAcsr(Accessor const& accessor, juce::String const& identifier)
{
    auto const trackAcsrs = accessor.getAcsrs<AcsrType::tracks>();
    return std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                       {
                           return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
                       });
}

bool Document::Tools::hasGroupAcsr(Accessor const& accessor, juce::String const& identifier)
{
    auto const groupAcsrs = accessor.getAcsrs<AcsrType::groups>();
    return std::any_of(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](auto const& groupAcsr)
                       {
                           return groupAcsr.get().template getAttr<Group::AttrType::identifier>() == identifier;
                       });
}

bool Document::Tools::isTrackInGroup(Accessor const& accessor, juce::String const& identifier)
{
    auto const groupAcsrs = accessor.getAcsrs<AcsrType::groups>();
    return std::any_of(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](auto const& groupAcsr)
                       {
                           return Group::Tools::hasTrackAcsr(groupAcsr.get(), identifier);
                       });
}

Track::Accessor const& Document::Tools::getTrackAcsr(Accessor const& accessor, juce::String const& identifier)
{
    auto const trackAcsrs = accessor.getAcsrs<AcsrType::tracks>();
    auto it = std::find_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                           {
                               return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
                           });
    anlStrongAssert(it != trackAcsrs.cend());
    return it->get();
}

Group::Accessor const& Document::Tools::getGroupAcsr(Accessor const& accessor, juce::String const& identifier)
{
    auto const groupAcsrs = accessor.getAcsrs<AcsrType::groups>();
    auto it = std::find_if(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](auto const& groupAcsr)
                           {
                               return groupAcsr.get().template getAttr<Group::AttrType::identifier>() == identifier;
                           });
    anlStrongAssert(it != groupAcsrs.cend());
    return it->get();
}

Group::Accessor const& Document::Tools::getGroupAcsrForTrack(Accessor const& accessor, juce::String const& identifier)
{
    auto const groupAcsrs = accessor.getAcsrs<AcsrType::groups>();
    auto it = std::find_if(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](auto const& groupAcsr)
                           {
                               return Group::Tools::hasTrackAcsr(groupAcsr.get(), identifier);
                           });
    anlStrongAssert(it != groupAcsrs.cend());
    return it->get();
}

Track::Accessor& Document::Tools::getTrackAcsr(Accessor& accessor, juce::String const& identifier)
{
    auto const trackAcsrs = accessor.getAcsrs<AcsrType::tracks>();
    auto it = std::find_if(trackAcsrs.begin(), trackAcsrs.end(), [&](auto const& trackAcsr)
                           {
                               return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
                           });
    anlStrongAssert(it != trackAcsrs.end());
    return it->get();
}

Group::Accessor& Document::Tools::getGroupAcsr(Accessor& accessor, juce::String const& identifier)
{
    auto const groupAcsrs = accessor.getAcsrs<AcsrType::groups>();
    auto it = std::find_if(groupAcsrs.begin(), groupAcsrs.end(), [&](auto const& groupAcsr)
                           {
                               return groupAcsr.get().template getAttr<Group::AttrType::identifier>() == identifier;
                           });
    anlStrongAssert(it != groupAcsrs.end());
    return it->get();
}

Group::Accessor& Document::Tools::getGroupAcsrForTrack(Accessor& accessor, juce::String const& identifier)
{
    auto const groupAcsrs = accessor.getAcsrs<AcsrType::groups>();
    auto it = std::find_if(groupAcsrs.begin(), groupAcsrs.end(), [&](auto const& groupAcsr)
                           {
                               return Group::Tools::hasTrackAcsr(groupAcsr.get(), identifier);
                           });
    anlStrongAssert(it != groupAcsrs.end());
    return it->get();
}

size_t Document::Tools::getTrackPosition(Accessor const& accessor, juce::String const& identifier)
{
    auto const& groupAcsr = getGroupAcsrForTrack(accessor, identifier);
    auto const& layout = groupAcsr.getAttr<Group::AttrType::layout>();
    auto const it = std::find(layout.cbegin(), layout.cend(), identifier);
    anlStrongAssert(it != layout.cend());
    return static_cast<size_t>(std::distance(layout.cbegin(), it));
}

size_t Document::Tools::getGroupPosition(Accessor const& accessor, juce::String const& identifier)
{
    auto const& layout = accessor.getAttr<AttrType::layout>();
    auto const it = std::find(layout.cbegin(), layout.cend(), identifier);
    anlWeakAssert(it != layout.cend());
    return static_cast<size_t>(std::distance(layout.cbegin(), it));
}

size_t Document::Tools::getItemPosition(Accessor const& accessor, juce::String const& identifier)
{
    auto index = 0_z;
    auto const documentLayout = accessor.getAttr<AttrType::layout>();
    for(auto const& groupId : documentLayout)
    {
        if(hasGroupAcsr(accessor, groupId))
        {
            if(groupId == identifier)
            {
                return index;
            }
            ++index;
            auto const& groupAcsr = getGroupAcsr(accessor, groupId);
            auto const& groupLayout = groupAcsr.getAttr<Group::AttrType::layout>();
            for(auto const& trackId : groupLayout)
            {
                if(trackId == identifier)
                {
                    return index;
                }
                ++index;
            }
        }
    }
    return index;
}

void Document::Tools::resizeItem(Accessor& accessor, juce::String const& itemIdentifier, int newHeight, int parentHeight)
{
    MiscWeakAssert(hasItem(accessor, itemIdentifier));
    if(!hasItem(accessor, itemIdentifier))
    {
        return;
    }
    static auto constexpr minHeight = 23;
    static auto constexpr maxHeight = 2000;
    newHeight = std::clamp(newHeight, minHeight, maxHeight);
    if(accessor.getAttr<AttrType::autoresize>())
    {
        class LayoutInfo
        {
        public:
            LayoutInfo(juce::String const& r, int p)
            : mReferenceIdentifier(r)
            , mParentHeight(p)
            {
            }

            void update(juce::String const& currentIdentifier, int currentHeight)
            {
                if(mReferenceIdentifier == currentIdentifier)
                {
                    MiscWeakAssert(mIsBefore);
                    mIsBefore = false;
                    mReferenceHeight = currentHeight;
                }
                else if(mIsBefore)
                {
                    mPreviousHeight += currentHeight;
                }
                else
                {
                    ++mFollowingItems;
                }
            }

            int getPreviousHeight() const
            {
                return mPreviousHeight;
            }

            int getSavedReferenceHeight() const
            {
                return mReferenceHeight;
            }

            int getFollowingItems() const
            {
                return mFollowingItems;
            }

            int getMaxReferenceHeight() const
            {
                return mParentHeight - (mPreviousHeight + mFollowingItems * minHeight);
            }

        private:
            juce::String const mReferenceIdentifier;
            int const mParentHeight;
            bool mIsBefore = true;
            int mPreviousHeight = 0;
            int mReferenceHeight = 0;
            int mFollowingItems = 0;
        };

        LayoutInfo info(itemIdentifier, parentHeight);
        auto const& documentLayout = accessor.getAttr<AttrType::layout>();
        for(auto const& groupIdentifier : documentLayout)
        {
            if(hasGroupAcsr(accessor, groupIdentifier))
            {
                auto const& groupAcsr = getGroupAcsr(accessor, groupIdentifier);
                info.update(groupIdentifier, groupAcsr.getAttr<Group::AttrType::height>());
                if(groupAcsr.getAttr<Group::AttrType::expanded>())
                {
                    auto const groupLayout = copy_with_erased_if(groupAcsr.getAttr<Group::AttrType::layout>(), [&](auto const& trackIdentifier)
                                                                 {
                                                                     return !Tools::hasTrackAcsr(accessor, trackIdentifier);
                                                                 });
                    for(auto const& trackIdentifier : groupLayout)
                    {
                        auto const& trackAcsr = Tools::getTrackAcsr(accessor, trackIdentifier);
                        info.update(trackIdentifier, trackAcsr.getAttr<Track::AttrType::height>());
                    }
                }
            }
        }
        if(info.getFollowingItems() == 0)
        {
            return;
        }

        newHeight = std::clamp(newHeight, minHeight, info.getMaxReferenceHeight());
        if(hasGroupAcsr(accessor, itemIdentifier))
        {
            getGroupAcsr(accessor, itemIdentifier).setAttr<Group::AttrType::height>(newHeight, NotificationType::synchronous);
        }
        else if(hasTrackAcsr(accessor, itemIdentifier))
        {
            getTrackAcsr(accessor, itemIdentifier).setAttr<Track::AttrType::height>(newHeight, NotificationType::synchronous);
        }
        auto remainder = 0.0f;
        bool isBefore = true;
        auto const previousFollowingHeight = parentHeight - info.getPreviousHeight() - info.getSavedReferenceHeight();
        auto remainingHeight = parentHeight - info.getPreviousHeight() - newHeight;
        auto const followingRatio = static_cast<float>(remainingHeight) / static_cast<float>(previousFollowingHeight);
        for(auto const& groupIdentifier : documentLayout)
        {
            if(hasGroupAcsr(accessor, groupIdentifier))
            {
                auto& groupAcsr = getGroupAcsr(accessor, groupIdentifier);
                if(!isBefore)
                {
                    auto const scaledHeight = static_cast<float>(groupAcsr.getAttr<Group::AttrType::height>()) * followingRatio + remainder;
                    auto const effectiveHeight = std::clamp(static_cast<int>(std::round(scaledHeight)), minHeight, remainingHeight);
                    remainder = scaledHeight - static_cast<float>(effectiveHeight);
                    remainingHeight -= effectiveHeight;
                    groupAcsr.setAttr<Group::AttrType::height>(effectiveHeight, NotificationType::synchronous);
                }
                if(groupIdentifier == itemIdentifier)
                {
                    isBefore = false;
                }
                if(groupAcsr.getAttr<Group::AttrType::expanded>())
                {
                    auto const groupLayout = copy_with_erased_if(groupAcsr.getAttr<Group::AttrType::layout>(), [&](auto const& trackIdentifier)
                                                                 {
                                                                     return !Tools::hasTrackAcsr(accessor, trackIdentifier);
                                                                 });
                    for(auto const& trackIdentifier : groupLayout)
                    {
                        auto& trackAcsr = Tools::getTrackAcsr(accessor, trackIdentifier);
                        if(!isBefore)
                        {
                            auto const scaledHeight = static_cast<float>(trackAcsr.getAttr<Track::AttrType::height>()) * followingRatio + remainder;
                            auto const effectiveHeight = std::clamp(static_cast<int>(std::round(scaledHeight)), minHeight, remainingHeight);
                            remainder = scaledHeight - static_cast<float>(effectiveHeight);
                            remainingHeight -= effectiveHeight;
                            trackAcsr.setAttr<Track::AttrType::height>(effectiveHeight, NotificationType::synchronous);
                        }
                        if(trackIdentifier == itemIdentifier)
                        {
                            isBefore = false;
                        }
                    }
                }
            }
        }
    }
    else if(hasGroupAcsr(accessor, itemIdentifier))
    {
        getGroupAcsr(accessor, itemIdentifier).setAttr<Group::AttrType::height>(newHeight, NotificationType::synchronous);
    }
    else if(hasTrackAcsr(accessor, itemIdentifier))
    {
        getTrackAcsr(accessor, itemIdentifier).setAttr<Track::AttrType::height>(newHeight, NotificationType::synchronous);
    }
}

void Document::Tools::resizeItems(Accessor& accessor, bool preserveRatio, int parentHeight)
{
    static auto constexpr minHeight = 23;
    int previousHeight = 0;
    int numItems = 0;
    auto const& documentLayout = accessor.getAttr<AttrType::layout>();
    for(auto const& groupIdentifier : documentLayout)
    {
        if(hasGroupAcsr(accessor, groupIdentifier))
        {
            auto const& groupAcsr = getGroupAcsr(accessor, groupIdentifier);
            previousHeight += groupAcsr.getAttr<Group::AttrType::height>();
            ++numItems;
            if(groupAcsr.getAttr<Group::AttrType::expanded>())
            {
                auto const groupLayout = copy_with_erased_if(groupAcsr.getAttr<Group::AttrType::layout>(), [&](auto const& trackIdentifier)
                                                             {
                                                                 return !Tools::hasTrackAcsr(accessor, trackIdentifier);
                                                             });
                for(auto const& trackIdentifier : groupLayout)
                {
                    auto const& trackAcsr = Tools::getTrackAcsr(accessor, trackIdentifier);
                    previousHeight += trackAcsr.getAttr<Track::AttrType::height>();
                    ++numItems;
                }
            }
        }
    }

    auto remainder = 0.0f;
    auto remainingHeight = parentHeight;
    if(preserveRatio)
    {
        auto const followingRatio = static_cast<float>(parentHeight) / static_cast<float>(previousHeight);
        for(auto const& groupIdentifier : documentLayout)
        {
            if(hasGroupAcsr(accessor, groupIdentifier))
            {
                auto& groupAcsr = getGroupAcsr(accessor, groupIdentifier);
                {
                    auto const scaledHeight = static_cast<float>(groupAcsr.getAttr<Group::AttrType::height>()) * followingRatio + remainder;
                    auto const effectiveHeight = std::clamp(static_cast<int>(std::round(scaledHeight)), minHeight, remainingHeight);
                    remainder = scaledHeight - static_cast<float>(effectiveHeight);
                    remainingHeight -= effectiveHeight;
                    groupAcsr.setAttr<Group::AttrType::height>(effectiveHeight, NotificationType::synchronous);
                }
                if(groupAcsr.getAttr<Group::AttrType::expanded>())
                {
                    auto const groupLayout = copy_with_erased_if(groupAcsr.getAttr<Group::AttrType::layout>(), [&](auto const& trackIdentifier)
                                                                 {
                                                                     return !Tools::hasTrackAcsr(accessor, trackIdentifier);
                                                                 });
                    for(auto const& trackIdentifier : groupLayout)
                    {
                        auto& trackAcsr = Tools::getTrackAcsr(accessor, trackIdentifier);
                        {
                            auto const scaledHeight = static_cast<float>(trackAcsr.getAttr<Track::AttrType::height>()) * followingRatio + remainder;
                            auto const effectiveHeight = std::clamp(static_cast<int>(std::round(scaledHeight)), minHeight, remainingHeight);
                            remainder = scaledHeight - static_cast<float>(effectiveHeight);
                            remainingHeight -= effectiveHeight;
                            trackAcsr.setAttr<Track::AttrType::height>(effectiveHeight, NotificationType::synchronous);
                        }
                    }
                }
            }
        }
    }
    else
    {
        auto const itemHeight = static_cast<float>(parentHeight) / static_cast<float>(numItems);
        for(auto const& groupIdentifier : documentLayout)
        {
            if(hasGroupAcsr(accessor, groupIdentifier))
            {
                auto& groupAcsr = getGroupAcsr(accessor, groupIdentifier);
                {
                    auto const scaledHeight = itemHeight + remainder;
                    auto const effectiveHeight = std::clamp(static_cast<int>(std::round(scaledHeight)), minHeight, remainingHeight);
                    remainder = scaledHeight - static_cast<float>(effectiveHeight);
                    remainingHeight -= effectiveHeight;
                    groupAcsr.setAttr<Group::AttrType::height>(effectiveHeight, NotificationType::synchronous);
                }
                if(groupAcsr.getAttr<Group::AttrType::expanded>())
                {
                    auto const groupLayout = copy_with_erased_if(groupAcsr.getAttr<Group::AttrType::layout>(), [&](auto const& trackIdentifier)
                                                                 {
                                                                     return !Tools::hasTrackAcsr(accessor, trackIdentifier);
                                                                 });
                    for(auto const& trackIdentifier : groupLayout)
                    {
                        auto& trackAcsr = Tools::getTrackAcsr(accessor, trackIdentifier);
                        {
                            auto const scaledHeight = itemHeight + remainder;
                            auto const effectiveHeight = std::clamp(static_cast<int>(std::round(scaledHeight)), minHeight, remainingHeight);
                            remainder = scaledHeight - static_cast<float>(effectiveHeight);
                            remainingHeight -= effectiveHeight;
                            trackAcsr.setAttr<Track::AttrType::height>(effectiveHeight, NotificationType::synchronous);
                        }
                    }
                }
            }
        }
    }
}

std::unique_ptr<juce::Component> Document::Tools::createTimeRangeEditor(Accessor& accessor)
{
    class RangeEditor
    : public juce::Component
    {
    public:
        RangeEditor(Zoom::Accessor& zoomAccessor)
        : mAccessor(zoomAccessor)
        , mName("Name", "Time")
        , mPropertyStart("Start", "The start time of the visible range", [&](double time)
                         {
                             auto const range = mAccessor.getAttr<Zoom::AttrType::visibleRange>().withStart(time);
                             mAccessor.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
                         })
        , mPropertyEnd("End", "The end time of the visible range", [&](double time)
                       {
                           auto const range = mAccessor.getAttr<Zoom::AttrType::visibleRange>().withEnd(time);
                           mAccessor.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
                       })
        {
            setWantsKeyboardFocus(true);
            addAndMakeVisible(mName);
            addAndMakeVisible(mPropertyStart);
            addAndMakeVisible(mPropertyEnd);
            mListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
            {
                switch(attribute)
                {
                    case Zoom::AttrType::globalRange:
                    {
                        auto const range = acsr.getAttr<Zoom::AttrType::globalRange>();
                        mPropertyStart.entry.setRange(range, juce::NotificationType::dontSendNotification);
                        mPropertyEnd.entry.setRange(range, juce::NotificationType::dontSendNotification);
                    }
                    break;
                    case Zoom::AttrType::visibleRange:
                    {
                        auto const range = acsr.getAttr<Zoom::AttrType::visibleRange>();
                        mPropertyStart.entry.setTime(range.getStart(), juce::NotificationType::dontSendNotification);
                        mPropertyEnd.entry.setTime(range.getEnd(), juce::NotificationType::dontSendNotification);
                    }
                    break;
                    case Zoom::AttrType::minimumLength:
                    case Zoom::AttrType::anchor:
                        break;
                }
            };

            mAccessor.addListener(mListener, NotificationType::synchronous);
            setSize(180, 74);
        }

        ~RangeEditor() override
        {
            mAccessor.removeListener(mListener);
        }

        // juce::Component
        void resized() override
        {
            auto bounds = getLocalBounds();
            mName.setBounds(bounds.removeFromTop(24));
            mPropertyStart.setBounds(bounds.removeFromTop(mPropertyStart.getHeight()));
            mPropertyEnd.setBounds(bounds.removeFromTop(mPropertyEnd.getHeight()));
            setSize(getWidth(), bounds.getY() + 2);
        }

    private:
        Zoom::Accessor& mAccessor;
        Zoom::Accessor::Listener mListener{typeid(*this).name()};
        juce::Label mName;
        PropertyHMSmsField mPropertyStart;
        PropertyHMSmsField mPropertyEnd;
    };

    return std::make_unique<RangeEditor>(accessor.getAcsr<AcsrType::timeZoom>());
}

Document::LayoutNotifier::LayoutNotifier(juce::String const name, Accessor& accessor, std::function<void(void)> fn, std::set<Group::AttrType> groupAttrs, std::set<Track::AttrType> trackAttrs)
: mAccessor(accessor)
, onLayoutUpdated(fn)
, mName(name)
, mTrackAttributes(std::move(trackAttrs))
, mGroupAttributes(std::move(groupAttrs))
{
    mListener.onAccessorInserted = [this](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::tracks:
            {
                auto listener = std::make_unique<Track::Accessor::SmartListener>(mName + "::" + typeid(*this).name(), mAccessor.getAcsr<AcsrType::tracks>(index), [this](Track::Accessor const& trackAcsr, Track::AttrType trackAttribute)
                                                                                 {
                                                                                     juce::ignoreUnused(trackAcsr);
                                                                                     if(onLayoutUpdated != nullptr && mTrackAttributes.contains(trackAttribute))
                                                                                     {
                                                                                         onLayoutUpdated();
                                                                                     }
                                                                                 });
                mTrackListeners.emplace(mTrackListeners.begin() + static_cast<long>(index), std::move(listener));
                anlWeakAssert(mTrackListeners.size() == acsr.getNumAcsrs<AcsrType::tracks>());
            }
            break;
            case AcsrType::groups:
            {
                auto listener = std::make_unique<Group::Accessor::SmartListener>(mName + "::" + typeid(*this).name(), mAccessor.getAcsr<AcsrType::groups>(index), [this](Group::Accessor const& groupAcsr, Group::AttrType groupAttribute)
                                                                                 {
                                                                                     juce::ignoreUnused(groupAcsr);
                                                                                     if(onLayoutUpdated != nullptr && mGroupAttributes.contains(groupAttribute))
                                                                                     {
                                                                                         onLayoutUpdated();
                                                                                     }
                                                                                 });
                mGroupListeners.emplace(mGroupListeners.begin() + static_cast<long>(index), std::move(listener));
                anlWeakAssert(mGroupListeners.size() == acsr.getNumAcsrs<AcsrType::groups>());
            }
            break;
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
        }
    };

    mListener.onAccessorErased = [this](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::tracks:
            {
                mTrackListeners.erase(mTrackListeners.begin() + static_cast<long>(index));
                anlWeakAssert(mTrackListeners.size() == acsr.getNumAcsrs<AcsrType::tracks>());
            }
            break;
            case AcsrType::groups:
            {
                mGroupListeners.erase(mGroupListeners.begin() + static_cast<long>(index));
                anlWeakAssert(mGroupListeners.size() == acsr.getNumAcsrs<AcsrType::groups>());
            }
            break;
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
        }
    };

    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::reader:
            case AttrType::viewport:
            case AttrType::path:
            case AttrType::grid:
            case AttrType::autoresize:
            case AttrType::samplerate:
            case AttrType::channels:
            case AttrType::editMode:
            case AttrType::drawingState:
                break;
            case AttrType::layout:
            {
                if(onLayoutUpdated != nullptr)
                {
                    onLayoutUpdated();
                }
            }
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::LayoutNotifier::~LayoutNotifier()
{
    mAccessor.removeListener(mListener);
}

ANALYSE_FILE_END
