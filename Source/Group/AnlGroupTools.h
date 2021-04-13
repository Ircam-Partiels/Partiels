#pragma once

#include "AnlGroupModel.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    namespace Tools
    {
        std::optional<std::reference_wrapper<Track::Accessor>> getTrackAcsr(Accessor const& accessor, juce::String const& identifier);
    }
    
    template<typename T>
    class TrackMap
    {
    public:
        TrackMap() = default;
        virtual ~TrackMap() = default;
        
        std::map<juce::String, T> const& getContents() const
        {
            return mContents;
        }
        
        void updateContents(Accessor const& accessor, std::function<T(Track::Accessor&)> createContent, std::function<void(T&)> removeContent)
        {
            auto hasTrack = [&](juce::String const& identifier)
            {
                auto const& layout = accessor.getAttr<AttrType::layout>();
                if(std::none_of(layout.cbegin(), layout.cend(), [&](auto const& layoutId)
                {
                    return layoutId == identifier;
                }))
                {
                    return false;
                }
                auto const trackAcsrs = accessor.getAttr<AttrType::tracks>();
                return std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                {
                    return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
                });
            };
            
            auto const& layout = accessor.getAttr<AttrType::layout>();
            auto it = mContents.begin();
            while(it != mContents.end())
            {
                if(!hasTrack(it->first))
                {
                    if(removeContent != nullptr)
                    {
                        removeContent(it->second);
                    }
                    it = mContents.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            
            for(auto const& identifier : layout)
            {
                auto contentIt = mContents.find(identifier);
                if(contentIt == mContents.cend())
                {
                    auto trackAcsr = Tools::getTrackAcsr(accessor, identifier);
                    if(trackAcsr.has_value() && createContent != nullptr)
                    {
                        mContents.emplace(identifier, createContent(trackAcsr->get()));
                    }
                }
            }
        }
        
    private:
        std::map<juce::String, T> mContents;
    };
}

ANALYSE_FILE_END
