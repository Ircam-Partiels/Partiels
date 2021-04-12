#pragma once

#include "AnlGroupModel.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    template<typename T>
    class TrackManager
    {
    public:
        TrackManager(Accessor& Accessor)
        : mAccessor(Accessor)
        {
            mListener.onAttrChanged = [this](class Accessor const& acsr, AttrType attribute)
            {
                juce::ignoreUnused(acsr);
                switch(attribute)
                {
                    case AttrType::identifier:
                    case AttrType::name:
                    case AttrType::height:
                    case AttrType::colour:
                    case AttrType::expanded:
                        break;
                    case AttrType::layout:
                    case AttrType::tracks:
                    {
                        updateLayout();
                    }
                        break;
                }
            };
            
            mAccessor.addListener(mListener, NotificationType::synchronous);
        }
        
        virtual ~TrackManager()
        {
            mAccessor.removeListener(mListener);
        }
        
        bool hasTrack(juce::String const& identifier) const
        {
            auto const trackAcsrs = mAccessor.getAttr<AttrType::tracks>();
            return std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
            {
                return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
            });
        }
        
        std::optional<std::reference_wrapper<Track::Accessor>> getTrack(juce::String const& identifier)
        {
            auto const trackAcsrs = mAccessor.getAttr<AttrType::tracks>();
            auto it = std::find_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
            {
                return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
            });
            return it != trackAcsrs.cend() ? *it : std::optional<std::reference_wrapper<Track::Accessor>>{};
        }
        
        std::map<juce::String, T> const& getGroupContents() const
        {
            return mContents;
        }
        
        virtual void updateContentsStarted() {}
        virtual void updateContentsEnded() {}
        virtual void removeFromContents(T& content) = 0;
        virtual T createForContents(Track::Accessor& trackAccessor) = 0;
        
    private:
        void updateLayout()
        {
            updateContentsStarted();
            auto const& layout = mAccessor.getAttr<AttrType::layout>();
            auto it = mContents.begin();
            while(it != mContents.end())
            {
                if(std::none_of(layout.cbegin(), layout.cend(), [&](auto const& identifer)
                {
                    return identifer == it->first;
                }) || !hasTrack(it->first))
                {
                    removeFromContents(it->second);
                    it = mContents.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            
            for(auto const& identifier : layout)
            {
                auto acsrIt = mContents.find(identifier);
                if(acsrIt == mContents.cend())
                {
                    auto trackAcsr = getTrack(identifier);
                    if(trackAcsr)
                    {
                        mContents.emplace(identifier, createForContents(trackAcsr->get()));
                    }
                }
            }
            updateContentsEnded();
        }
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        std::map<juce::String, T> mContents;
    };
}

ANALYSE_FILE_END
