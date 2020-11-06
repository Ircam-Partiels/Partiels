#pragma once

#include "AnlNotifier.h"
#include "AnlBroadcaster.h"
#include "../../magic_enum/include/magic_enum.hpp"

ANALYSE_FILE_BEGIN

namespace Model
{
    enum AttrFlag
    {
        ignored = 0 << 0,
        notifying = 1 << 0,
        saveable = 1 << 1,
        comparable = 1 << 2,
        all = notifying | saveable | comparable
    };
    
    //! @brief The private implementation of an attribute of a model
    template<typename enum_t, enum_t index_v, typename value_t, AttrFlag flags_v> struct AttrImpl
    {
        static_assert(std::is_enum<enum_t>::value, "enum_t must be an enum");
        static_assert(std::is_same<std::underlying_type_t<enum_t>, size_t>::value, "enum_t underlying type must be size_t");
        
        using enum_type = enum_t;
        using value_type = value_t;
        
        static enum_type const type = static_cast<enum_type>(index_v);
        static AttrFlag const flags = flags_v;
        value_type value;
    };
    
    //! @brief The template typle of an attribute of a model
    template<auto index_v, typename value_t, AttrFlag flags_v> using Attr = AttrImpl<decltype(index_v), index_v, value_t, flags_v>;
    
    //! @brief The container type for a set of attributes
    template <class ..._Tp> using Container = std::tuple<_Tp...>;
    
    //! @brief The accessor a data model
    //! @todo Implement a comparaison method
    template<class container_t> class Accessor
    {
    public:
        
        using container_type = container_t;
        using enum_type = typename std::tuple_element<0, container_type>::type::enum_type;
        static_assert(std::is_same<typename std::underlying_type<enum_type>::type, size_t>::value, "enum_t underlying type must be size_t");
        static_assert(is_specialization<container_type, std::tuple>::value, "econtainer_t must be a specialization of std::tuple");

        //! @brief The constructor
        Accessor(container_type&& data = {})
        : mData(std::move(data))
        {
        }
        
        //! @brief The destructor
        ~Accessor() = default;

        //! @brief Gets a const ref to the model container
        auto const& getModel() const noexcept
        {
            return mData;
        }
        
        //! @brief Gets the value of an attribute
        template <enum_type attribute>
        auto const& getValue() const noexcept
        {
            return std::get<static_cast<size_t>(attribute)>(mData).value;
        }
        
        //! @brief Sets the value of an attribute
        //! @details If the value changed and the attribute is marked as notifying, the method notifies the listeners .
        template <enum_type attribute, typename value_v>
        auto setValue(value_v const& value, NotificationType notification)
        {
            using attr_type = typename std::tuple_element<static_cast<size_t>(attribute), container_type>::type;
            auto& lvalue = std::get<static_cast<size_t>(attribute)>(mData).value;
            if(lvalue != value)
            {
                std::get<static_cast<size_t>(attribute)>(mData).value = value;
                if constexpr((attr_type::flags & AttrFlag::notifying) != 0)
                {
                    mListeners.notify([=](Listener& listener)
                    {
                        anlWeakAssert(listener.onChanged != nullptr);
                        if(listener.onChanged != nullptr)
                        {
                            listener.onChanged(*this, attribute);
                        }
                    }, notification);
                }
            }
        }
        
        //! @brief Parse the model to xml
        //! @details Only the saveable attributes are stored into the xml
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
                    auto const enumname = std::string(magic_enum::enum_name(attr_type::type));
                    xml->setAttribute(enumname.c_str(), d.value);
                }
            });
            return xml;
        }
        
        //! @brief Parse the model from xml
        //! @details Only the saveable attributes are restored from the xml.
        //! If the value changed and the attribute is marked as notifying, the method notifies the listeners .
        auto fromXml(juce::XmlElement const& xml, juce::StringRef const& name, NotificationType notification)
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
                    using value_type = typename attr_type::value_type;
                    auto const enumname = std::string(magic_enum::enum_name(attr_type::type));
                    anlWeakAssert(xml.hasAttribute(enumname));
                    value_type newValue;
                    std::stringstream ss(xml.getStringAttribute(enumname, juce::String(d.value)).toRawUTF8());
                    ss >> newValue;
                    setValue<attr_type::type>(newValue, notification);
                }
            });
        }
        
        //! @brief Copy the content from another model
        auto fromModel(container_type const& model, NotificationType notification)
        {
            detail::for_each(mData, [&](auto& d)
            {
                using attr_type = typename std::remove_reference<decltype(d)>::type;
                if constexpr((attr_type::flags & AttrFlag::saveable) != 0)
                {
                    auto const& value = std::get<static_cast<size_t>(attr_type::type)>(model).value;
                    setValue<attr_type::type>(value, notification);
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
        
        void addListener(Listener& listener, NotificationType const notification)
        {
            if(mListeners.add(listener))
            {
                detail::for_each(mData, [&](auto& d)
                {
                    using attr_type = typename std::remove_reference<decltype(d)>::type;
                    auto const enumname = std::string(magic_enum::enum_name(attr_type::type));
                    if constexpr((attr_type::flags & AttrFlag::notifying) != 0)
                    {
                        mListeners.notify([this, ptr = &listener](Listener& ltnr)
                        {
                            anlWeakAssert(ltnr.onChanged != nullptr);
                            if(&ltnr == ptr && ltnr.onChanged != nullptr)
                            {
                                ltnr.onChanged(*this, attr_type::type);
                            }
                        }, notification);
                    }
                });
            }
        }
        
        void removeListener(Listener& listener)
        {
            mListeners.remove(listener);
        }
        
    private:
        
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
        Notifier<Listener> mListeners;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Accessor)
    };
}

ANALYSE_FILE_END
