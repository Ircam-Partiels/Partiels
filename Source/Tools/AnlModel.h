#pragma once

#include "AnlListenerList.h"
#include "AnlStringParser.h"
#include "../../magic_enum/include/magic_enum.hpp"

ANALYSE_FILE_BEGIN

namespace Model
{
    enum AttrFlag
    {
        ignored = 0,
        notifying = 1,
        saveable = 2,
        copyable = 4,
        all = 7
    };
    
    //! @brief The private implementation of an attribute of a model
    template<typename enum_t, enum_t index_v, typename value_t, AttrFlag flags_v> struct AttrTypeImpl
    {
        static_assert(std::is_enum<enum_t>::value, "enum_t must be an enum");
        static_assert(std::is_same<std::underlying_type_t<enum_t>, size_t>::value, "enum_t underlying type must be size_t");
        
        using enum_type = enum_t;
        using value_type = value_t;
        
        static AttrFlag const flags = flags_v;
        static enum_t const entry = static_cast<enum_t>(index_v);
        
        value_type value;
    };
    
    //! @brief The template typle of an attribute of a model
    template<auto index_v, typename value_t, AttrFlag flags_v> using AttrType = AttrTypeImpl<decltype(index_v), index_v, value_t, flags_v>;
    
    //! @brief The container type for a set of attributes
    template <class ..._Tp> using Container = std::tuple<_Tp...>;
    
    //! @brief The accessor a data model
    template<class container_t> class Accessor
    {
    private:
        // https://stackoverflow.com/questions/13101061/detect-if-a-type-is-a-stdtuple
        template <typename> struct is_tuple: std::false_type {};
        template <typename ...T> struct is_tuple<std::tuple<T...>>: std::true_type {};
        
    public:
        
        using container_type = container_t;
        using enum_type = typename std::tuple_element<0, container_type>::type::enum_type;
        static_assert(is_tuple<container_type>{}, "container_t must be a specialization of std::tuple");

        Accessor(container_type&& data = {})
        : mData(std::move(data))
        {
        }
        
        ~Accessor() = default;

        //! @brief Gets a const ref to the model container
        auto getModel() const noexcept
        {
            return mData;
        }
        
        //! @brief Gets the value of an attribute
        template <enum_type entry>
        auto getValue() const noexcept
        {
            return std::get<static_cast<size_t>(entry)>(mData).value;
        }
        
        //! @brief Sets the value of an attribute
        template <enum_type entry, typename value_v>
        auto setValue(value_v value)
        {
            using value_type = typename std::tuple_element<static_cast<size_t>(entry), container_type>::type::value_type;
            static_assert(std::is_same<value_type, value_v>::value, "enum_t underlying type must be size_t");
            auto& lvalue = std::get<static_cast<size_t>(entry)>(mData).value;
            if(lvalue != value)
            {
                std::get<static_cast<size_t>(entry)>(mData).value = value;
            }
        }
        
        auto toXml(juce::StringRef const& name) const
        {
            auto xml = std::make_unique<juce::XmlElement>(name);
            anlWeakAssert(xml != nullptr);
            if(xml == nullptr)
            {
                return std::unique_ptr<juce::XmlElement>();
            }
            
            detail::for_each(mData, [&](auto const& d)
            {
                using attr_type = typename std::remove_reference<decltype(d)>::type;
                using namespace magic_enum::bitwise_operators;
                if constexpr((attr_type::flags & AttrFlag::saveable) != 0)
                {
                    auto const enumname = std::string(magic_enum::enum_name(attr_type::entry));
                    xml->setAttribute(enumname.c_str(), d.value);
                }
            });
            return xml;
        }
        
        auto fromXml(juce::XmlElement const& xml, juce::StringRef const& name)
        {
            anlWeakAssert(xml.hasTagName(name));
            if(!xml.hasTagName(name))
            {
                return;
            }
            
            detail::for_each(mData, [&](auto& d)
            {
                using attr_type = typename std::remove_reference<decltype(d)>::type;
                if constexpr((attr_type::flags & AttrFlag::saveable) != 0)
                {
                    auto const enumname = std::string(magic_enum::enum_name(attr_type::entry));
                    anlWeakAssert(xml.hasAttribute(enumname));
                    std::stringstream ss(xml.getStringAttribute(enumname, juce::String(d.value)).toRawUTF8());
                    ss >> d.value;
                }
            });
        }
        
        void fromModel(container_type const& model)
        {
            Accessor<container_type> acsr{model};
            detail::for_each(mData, [&](auto& d)
                             {
                using attr_type = typename std::remove_reference<decltype(d)>::type;
                if constexpr((attr_type::flags & AttrFlag::copyable) != 0)
                {
                    setValue<attr_type>(acsr.getValue<attr_type>());
                }
            });
        }
        
        class Listener
        {
        public:
            Listener() = default;
            virtual ~Listener() = default;
            
            std::function<void(Accessor const&, enum_type attribute)> onChanged = nullptr;
        };
        
        void addListener(Listener& listener, juce::NotificationType const notification)
        {
            if(mListeners.add(listener))
            {
                detail::for_each(mData, [&](auto& d)
                {
                    using attr_type = typename std::remove_reference<decltype(d)>::type;
                    auto const attribute = attr_type::entry;
                    mListeners.notify([this, attribute, ptr = &listener](Listener& ltnr)
                    {
                        anlWeakAssert(ltnr.onChanged != nullptr);
                        if(&ltnr == ptr && ltnr.onChanged != nullptr)
                        {
                            ltnr.onChanged(this, attribute);
                        }
                    }, notification);
                });
            }
        }
        
        void removeListener(Listener& listener)
        {
            mListeners.remove(listener);
        }
        
    private:
        
        void notifyListener(std::set<enum_type> const& attrs, juce::NotificationType const notification)
        {
            for(auto const attr : attrs)
            {
                mListeners.notify([=](Listener& listener)
                {
                    anlWeakAssert(listener.onChanged != nullptr);
                    if(listener.onChanged != nullptr)
                    {
                        listener.onChanged(*this, attr);
                    }
                }, notification);
            }
        }
        
        // This is a for_each mechanism for std::tuple
        struct detail
        {
            // https://stackoverflow.com/questions/16387354/template-tuple-calling-a-function-on-each-element
            template<int... Is> struct seq {};
            template<int N, int... Is> struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};
            template<int... Is> struct gen_seq<0, Is...> : seq<Is...> {};
            template<typename T, typename F, int... Is> static void for_each(T&& t, F f, seq<Is...>) {auto l = { (f(std::get<Is>(t)), 0)... }; juce::ignoreUnused(l);}
            template<typename... Ts, typename F> static void for_each(std::tuple<Ts...> const& t, F f) {for_each(t, f, gen_seq<sizeof...(Ts)>());}
            template<typename... Ts, typename F> static void for_each(std::tuple<Ts...>& t, F f) {for_each(t, f, gen_seq<sizeof...(Ts)>());}
        };
        
        container_type mData;
        Tools::ListenerList<Listener> mListeners;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Accessor)
    };
}

ANALYSE_FILE_END
