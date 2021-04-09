#pragma once

#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    template<typename T>
    class GroupContainer
    {
    public:
        GroupContainer(Accessor& accessor)
        : mAccessor(accessor)
        {
            auto updateLayout = [&]()
            {
                auto const& layout = mAccessor.getAttr<AttrType::layout>();
                auto it = mTrackContents.begin();
                while(it != mTrackContents.end())
                {
                    if(!std::binary_search(layout.cbegin(), layout.cend(), it->first) || !Tools::hasTrack(mAccessor, it->first))
                    {
                        removeFromGroup(it->second);
                        it = mTrackContents.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
                
                for(auto const& identifier : layout)
                {
                    auto acsrIt = mTrackContents.find(identifier);
                    if(acsrIt == mTrackContents.cend())
                    {
                        auto trackAcsr = Tools::getTrack(mAccessor, identifier);
                        if(trackAcsr)
                        {
                            mTrackContents.emplace(identifier, createForGroup(trackAcsr->get()));
                        }
                    }
                }
            };
            
            mListener.onAttrChanged = [=](Accessor const& acsr, AttrType attribute)
            {
                juce::ignoreUnused(acsr);
                switch(attribute)
                {
                    case AttrType::file:
                    case AttrType::layoutVertical:
                    case AttrType::expanded:
                        break;
                    case AttrType::layout:
                    {
                        updateLayout();
                    }
                        break;
                }
            };
            
            mListener.onAccessorErased = mListener.onAccessorInserted = [=](Accessor const& acsr, AcsrType type, size_t index)
            {
                juce::ignoreUnused(acsr, index);
                switch(type)
                {
                    case AcsrType::timeZoom:
                    case AcsrType::transport:
                        break;
                    case AcsrType::tracks:
                    {
                        updateLayout();
                    }
                        break;
                }
            };
            
            mAccessor.addListener(mListener, NotificationType::synchronous);
        }
        
        virtual ~GroupContainer()
        {
            mAccessor.removeListener(mListener);
        }
        
        std::map<juce::String, T> const& getGroupContent() const
        {
            return mTrackContents;
        }
        
        virtual void updateStarted() {}
        virtual void updateEnded() {}
        virtual void removeFromGroup(T& value) = 0;
        virtual T createForGroup(Track::Accessor& trackAccessor) = 0;
        
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        std::map<juce::String, T> mTrackContents;
    };
}

ANALYSE_FILE_END
