#pragma once

#include "AnlXmlParser.h"
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
        basic = notifying | saveable | comparable,
        model = 1 << 3,
    };
    
    //! @brief The private implementation of an attribute of a model
    template<typename enum_t, enum_t index_v, typename value_t, int flags_v>
    struct AttrImpl
    {
        static_assert(std::is_enum<enum_t>::value, "enum_t must be an enum");
        static_assert(std::is_same<std::underlying_type_t<enum_t>, size_t>::value, "enum_t underlying type must be size_t");
        
        using enum_type = enum_t;
        using value_type = value_t;
        
        static enum_type const type = static_cast<enum_type>(index_v);
        static int const flags = flags_v;
        value_type value;
        AttrImpl() = default;
        AttrImpl(AttrImpl const& other)
        {
            if constexpr((flags & move_semanti_only) == 0)
            {
                value = other.value;
            }
        }
        AttrImpl(AttrImpl&& other) : value(std::move(other.value)) {}
        AttrImpl(value_type const& other)
        {
            if constexpr((flags & move_semanti_only) == 0)
            {
                value = other;
            }
        }
        AttrImpl(value_type&& other) : value(std::move(other)) {}
        
        AttrImpl& operator=(AttrImpl const& other)
        {
            if constexpr((flags & move_semanti_only) == 0)
            {
                value = other.value;
            }
            return *this;
        }
        AttrImpl& operator=(AttrImpl&& other) {value = std::move(other.value); return *this;}
        AttrImpl& operator=(value_type const& other)
        {
            if constexpr((flags & move_semanti_only) == 0)
            {
                value = other;
            }
            return *this;
        }
        AttrImpl& operator=(value_type&& other) {value = std::move(other); return *this;}
        
    private:
        static constexpr int move_semanti_only = 1 << 16;
    };

    //! @brief The template typle of an attribute of a model
    template<auto index_v, typename value_t, int flags_v>
    using Attr = AttrImpl<decltype(index_v), index_v, value_t, flags_v>;
    
    //! @brief The template typle of an attribute of a model
    template<auto index_v, typename value_t, int flags_v>
    using AttrNoCopy = AttrImpl<decltype(index_v), index_v, value_t, flags_v+(1 << 16)>;
    
    //! @brief The container type for a set of attributes
    template <class ..._Tp>
    using Container = std::tuple<_Tp...>;
    
    //! @brief The accessor a data model
    template<class parent_t, class container_t>
    class Accessor
    {
    public:
        static constexpr bool model_accessor = true;
        using container_type = container_t;
        using enum_type = typename std::tuple_element<0, container_type>::type::enum_type;
        static_assert(std::is_same<typename std::underlying_type<enum_type>::type, size_t>::value, "enum_t underlying type must be size_t");
        static_assert(is_specialization<container_type, std::tuple>::value, "econtainer_t must be a specialization of std::tuple");
        
        //! @brief The constructor with data
        Accessor(container_type const& data)
        : mData(data)
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
        template <enum_type type>
        auto const& getValue() const noexcept
        {
            return std::get<static_cast<size_t>(type)>(mData).value;
        }
        
        //! @brief Sets the value of an attribute
        //! @details If the value changed and the attribute is marked as notifying, the method notifies the listeners .
        template <enum_type type, typename T>
        void setValue(T const& value, NotificationType notification = NotificationType::synchronous)
        {
            using attr_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            using value_type = typename attr_type::value_type;
            auto& lvalue = std::get<static_cast<size_t>(type)>(mData).value;
            if constexpr((attr_type::flags & AttrFlag::model) != 0)
            {
                if constexpr(is_specialization<value_type, std::vector>::value)
                {
                    using vector_value_type = typename value_type::value_type;
                    lvalue.reserve(value.size());
                    while(lvalue.size() < value.size())
                    {
                        if constexpr(is_specialization<vector_value_type, std::unique_ptr>::value)
                        {
                            using element_value_type = typename vector_value_type::element_type;
                            lvalue.push_back(std::make_unique<element_value_type>(*value[lvalue.size()].get()));
                        }
                        else
                        {
                            lvalue.push_back(value[lvalue.size()]);
                        }
                    }
                    for(size_t index = 0; index < value.size(); ++index)
                    {
                        auto& acsr = static_cast<parent_t*>(this)->template getAccessor<type>(index);
                        if constexpr(is_specialization<vector_value_type, std::unique_ptr>::value)
                        {
                            if(value[index] != nullptr)
                            {
                                acsr.fromModel(*value[index].get(), notification);
                            }
                        }
                        else
                        {
                            acsr.fromModel(value[index], notification);
                        }
                    }
                    JUCE_COMPILER_WARNING("improve that")
                    if constexpr((attr_type::flags & AttrFlag::notifying) != 0)
                    {
                        mListeners.notify([=](Listener& listener)
                        {
                            anlWeakAssert(listener.onChanged != nullptr);
                            if(listener.onChanged != nullptr)
                            {
                                listener.onChanged(*static_cast<parent_t const*>(this), type);
                            }
                        }, notification);
                    }
                }
                else
                {
                    auto& acsr = static_cast<parent_t*>(this)->template getAccessor<type>(0);
                    acsr.fromModel(value, notification);
                }
            }
            else if constexpr(std::is_same<value_type, T>::value)
            {
                if(equal(lvalue, value) == false)
                {
                    set<value_type>(lvalue, value);
                    if constexpr((attr_type::flags & AttrFlag::notifying) != 0)
                    {
                        mListeners.notify([=](Listener& listener)
                        {
                            anlWeakAssert(listener.onChanged != nullptr);
                            if(listener.onChanged != nullptr)
                            {
                                listener.onChanged(*static_cast<parent_t const*>(this), type);
                            }
                        }, notification);
                    }
                }
            }
            else
            {
                value_type const tvalue(value);
                static_cast<parent_t*>(this)->template setValue<type>(tvalue, notification);
            }
        }
        
        //! @brief Sets the value of an attribute (initializer list specialization)
        template <enum_type type, typename T>
        void setValue(std::initializer_list<T>&& tvalue, NotificationType notification = NotificationType::synchronous)
        {
            using attr_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            typename attr_type::value_type const value {tvalue};
            static_cast<parent_t*>(this)->template setValue<type>(value, notification);
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
                using element_type = typename std::remove_reference<decltype(d)>::type;
                if constexpr((element_type::flags & AttrFlag::saveable) != 0)
                {
                    auto constexpr attr_type = element_type::type;
                    using value_type = typename element_type::value_type;
                    static auto const enumname = std::string(magic_enum::enum_name(attr_type));
                    
                    if constexpr((element_type::flags & AttrFlag::model) != 0)
                    {
                        if constexpr(is_specialization<value_type, std::vector>::value)
                        {
                            using vector_value_type = typename value_type::value_type;
                            for(size_t i = 0; i < d.value.size(); ++i)
                            {
                                if constexpr(is_specialization<vector_value_type, std::unique_ptr>::value)
                                {
                                    anlWeakAssert(d.value[i] != nullptr);
                                    if(d.value[i] != nullptr)
                                    {
                                        auto& acsr = static_cast<parent_t const*>(this)->template getAccessor<attr_type>(i);
                                        auto child = acsr.toXml(enumname.c_str());
                                        if(child != nullptr)
                                        {
                                            xml->addChildElement(child.release());
                                        }
                                    }
                                }
                                else
                                {
                                    
                                    auto& acsr = static_cast<parent_t const*>(this)->template getAccessor<attr_type>(i);
                                    auto child = acsr.toXml(enumname.c_str());
                                    if(child != nullptr)
                                    {
                                        xml->addChildElement(child.release());
                                    }
                                }
                            }
                        }
                        else
                        {
                            auto& acsr = static_cast<parent_t const*>(this)->template getAccessor<attr_type>(0);
                            auto child = acsr.toXml(enumname.c_str());
                            if(child != nullptr)
                            {
                                xml->addChildElement(child.release());
                            }
                        }
                    }
                    else
                    {
                        XmlParser::toXml(*xml.get(), enumname.c_str(), d.value);
                    }
                }
            });
            return xml;
        }
        
        //! @brief Parse the model from xml
        //! @details Only the saveable attributes are restored from the xml.
        //! If the value changed and the attribute is marked as notifying, the method notifies the listeners .
        void fromXml(juce::XmlElement const& xml, juce::StringRef const& name, NotificationType notification = NotificationType::synchronous)
        {
            anlWeakAssert(xml.hasTagName(name));
            if(!xml.hasTagName(name))
            {
                return;
            }
            
            detail::for_each(mData, [&](auto& d)
            {
                using element_type = typename std::remove_reference<decltype(d)>::type;
                if constexpr((element_type::flags & AttrFlag::saveable) != 0)
                {
                    auto constexpr attr_type = element_type::type;
                    auto const enumname = std::string(magic_enum::enum_name(attr_type));
                    
                    if constexpr((element_type::flags & AttrFlag::model) != 0)
                    {
                        using value_type = typename element_type::value_type;
                        if constexpr(is_specialization<value_type, std::vector>::value)
                        {
                            size_t index = 0;
                            auto* child = xml.getChildByName(enumname.c_str());
                            while(child != nullptr)
                            {
                                auto& acsr = static_cast<parent_t*>(this)->template getAccessor<attr_type>(index);
                                acsr.fromXml(*child, enumname.c_str(), notification);
                                child = child->getNextElementWithTagName(enumname.c_str());
                                ++index;
                            }
                        }
                        else
                        {
                            auto* child = xml.getChildByName(enumname.c_str());
                            if(child != nullptr)
                            {
                                auto& acsr = static_cast<parent_t*>(this)->template getAccessor<attr_type>(0);
                                acsr.fromXml(*child, enumname.c_str(), notification);
                            }
                        }
                    }
                    else
                    {
                        static_cast<parent_t*>(this)->template setValue<attr_type>(XmlParser::fromXml(xml, enumname.c_str(), d.value), notification);
                    }
                }
            });
        }
        
        //! @brief Copy the content from another model
        void fromModel(container_type const& model, NotificationType notification = NotificationType::synchronous)
        {
            detail::for_each(model, [&](auto const& d)
            {
                using element_type = typename std::remove_reference<decltype(d)>::type;
                if constexpr((element_type::flags & AttrFlag::saveable) != 0)
                {
                    auto constexpr attr_type = element_type::type;
                    static_cast<parent_t*>(this)->template setValue<attr_type>(d.value, notification);
                }
            });
        }
        
        //! @brief Compare the content with  another model
        bool isEquivalentTo(container_type const& model) const
        {
            bool result = true;
            detail::for_each(mData, [&](auto& d)
            {
                if(result)
                {
                    using element_type = typename std::remove_reference<decltype(d)>::type;
                    if constexpr((element_type::flags & AttrFlag::comparable) != 0)
                    {
                        using value_type = typename element_type::value_type;
                        auto constexpr attr_type = element_type::type;
                        if constexpr((element_type::flags & AttrFlag::model) != 0)
                        {
                            if constexpr(is_specialization<value_type, std::vector>::value)
                            {
                                JUCE_COMPILER_WARNING("to do")
                            }
                            else
                            {
                                auto& acsr = static_cast<parent_t const*>(this)->template getAccessor<attr_type>(0);
                                result = acsr.isEquivalentTo(std::get<static_cast<size_t>(attr_type)>(model).value);
                            }
                        }
                        else
                        {
                            result = equal(d.value, std::get<static_cast<size_t>(attr_type)>(model).value);
                        }
                    }
                }
            });
            return result;
        }
        
        class Listener
        {
        public:
            Listener() = default;
            virtual ~Listener() = default;
            
            std::function<void(parent_t const&, enum_type type)> onChanged = nullptr;
        };
        
        void addListener(Listener& listener, NotificationType const notification)
        {
            if(mListeners.add(listener))
            {
                detail::for_each(mData, [&](auto& d)
                {
                    using element_type = typename std::remove_reference<decltype(d)>::type;
                    if constexpr((element_type::flags & AttrFlag::notifying) != 0)
                    {
                        mListeners.notify([this, ptr = &listener](Listener& ltnr)
                        {
                            anlWeakAssert(ltnr.onChanged != nullptr);
                            if(&ltnr == ptr && ltnr.onChanged != nullptr)
                            {
                                auto constexpr attr_type = element_type::type;
                                ltnr.onChanged(*static_cast<parent_t const*>(this), attr_type);
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
        
    protected:
        
        //! @brief Gets the value of an attribute
        template <enum_type type>
        auto& getValueRef() noexcept
        {
            return std::get<static_cast<size_t>(type)>(mData).value;
        }
        
        template <enum_type type>
        auto& getAccessor(size_t index) noexcept;
        
        template <enum_type type>
        auto const& getAccessor(size_t index) const noexcept;
        
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
                
        // This is a specific compare method that manage containers and unique pointer
        template <typename T> static
        bool equal(T const& lhs, T const& rhs)
        {
            if constexpr(is_specialization<T, std::vector>::value ||
                         is_specialization<T, std::map>::value)
            {
                if(lhs.size() != rhs.size())
                {
                    return false;
                }
                return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin(), [](auto const& slhs, auto const& srhs)
                {
                    return equal(slhs, srhs);
                });
            }
            else if constexpr(is_specialization<T, std::unique_ptr>::value)
            {
                return lhs != nullptr && rhs != nullptr && equal(*lhs.get(), *rhs.get());
            }
            else
            {
                return lhs == rhs;
            }
        }

        
        // This is a specific assignement method that manage containers and unique pointer
        template <typename T> static
        void set(T& lhs, T const& rhs)
        {
            if constexpr(is_specialization<T, std::vector>::value)
            {
                lhs.resize(rhs.size());
                for(size_t i = 0; i < lhs.size(); ++i)
                {
                    set(lhs[i], rhs[i]);
                }
            }
            else if constexpr(is_specialization<T, std::vector>::value)
            {
                std::erase_if(lhs, [&](auto const& pair)
                {
                    return rhs.count(pair.first) == 0;
                });
                for(auto const& pair : rhs)
                {
                    set(lhs[pair.first], pair.second);
                }
            }
            else if constexpr(is_specialization<T, std::unique_ptr>::value)
            {
                if(lhs != nullptr && rhs != nullptr)
                {
                    set(*lhs.get(), *rhs.get());
                }
            }
            else
            {
                lhs = rhs;
            }
        }
        
        container_type mData;
        Notifier<Listener> mListeners;
        
        template<class parent_t_, class container_t_> friend class Accessor;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Accessor)
    };
}

ANALYSE_FILE_END
