#pragma once

#include "AnlGroupModel.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    namespace Tools
    {
        bool hasTrackAcsr(Accessor const& accessor, juce::String const& identifier);
        std::optional<std::reference_wrapper<Track::Accessor>> getTrackAcsr(Accessor const& accessor, juce::String const& identifier);
    } // namespace Tools

    class LayoutNotifier
    {
    public:
        LayoutNotifier(Accessor& accessor, std::function<void(void)> fn = nullptr, std::set<Track::AttrType> attributes = {Track::AttrType::identifier});
        ~LayoutNotifier();

    private:
        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        std::set<Track::AttrType> mAttributes;

    public:
        std::function<void(void)> onLayoutUpdated = nullptr;

    private:
        struct cmp
        {
            bool operator()(std::unique_ptr<Track::Accessor::SmartListener> const& lhs, std::unique_ptr<Track::Accessor::SmartListener> const& rhs) const
            {
                return rhs == nullptr || (lhs != nullptr && std::addressof(lhs->accessor.get()) > std::addressof(rhs->accessor.get()));
            }
        };
        std::set<std::unique_ptr<Track::Accessor::SmartListener>, cmp> mTrackListeners;
    };

    template <typename T>
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
            auto const& layout = accessor.getAttr<AttrType::layout>();
            auto it = mContents.begin();
            while(it != mContents.end())
            {
                if(!Tools::hasTrackAcsr(accessor, it->first))
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
} // namespace Group

ANALYSE_FILE_END
