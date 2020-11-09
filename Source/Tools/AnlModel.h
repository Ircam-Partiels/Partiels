#pragma once

#include "AnlNotifier.h"
#include "AnlBroadcaster.h"
#include "../../magic_enum/include/magic_enum.hpp"

ANALYSE_FILE_BEGIN

namespace Model
{
    class StringParser
    {
    public:
        template <typename T> static
        T fromString(juce::String const& string)
        {
            if constexpr (std::is_constructible<T, juce::String>::value)
            {
                return T(string);
            }
            else if constexpr(is_container<T>::value)
            {
                T value;
                juce::StringArray stringArray;
                stringArray.addLines(string);
                value.resize(static_cast<size_t>(stringArray.size()));
                std::transform(stringArray.begin(), stringArray.end(), value.begin(), [](auto const line)
                {
                    return fromString<typename T::value_type>(line);
                });
                return value;
            }
            else
            {
                T value;
                std::stringstream stream(string.toRawUTF8());
                stream >> value;
                return value;
            }
        }
        
        template <typename T> static
        juce::String toString(T const& value)
        {
            if constexpr (is_explicitly_convertible<T, juce::String>::value)
            {
                return static_cast<juce::String>(value);
            }
            else if constexpr(is_container<T>::value)
            {
                juce::StringArray stringArray;
                stringArray.ensureStorageAllocated(static_cast<int>(value.size()));
                for(auto const& subvalue : value)
                {
                    stringArray.add(toString(subvalue));
                }
                return stringArray.joinIntoString("\n");
            }
            else
            {
                std::ostringstream stream;
                stream << value;
                return juce::String(stream.str());
            }
        }
        
        template <typename T>
        static T fromXml(juce::XmlElement const& xml, juce::StringRef const& name, T const& defaultValue)
        {
            return fromString<T>(xml.getStringAttribute(name, toString<T>(defaultValue)));
        }
        
        template <> unsigned long fromString(juce::String const& string);
        template <> juce::String toString(unsigned long const& value);
        
        template <> juce::File fromString(juce::String const& string);
        template <> juce::String toString(juce::File const& value);
        
        template <> juce::Range<double> fromString(juce::String const& string);
        template <> juce::String toString(juce::Range<double> const& value);
    };
    
    enum AttrFlag
    {
        ignored = 0 << 0,
        notifying = 1 << 0,
        saveable = 1 << 1,
        comparable = 1 << 2,
        all = notifying | saveable | comparable
    };
    
    //! @brief The private implementation of an attribute of a model
    template<typename enum_t, enum_t index_v, typename value_t, int flags_v> struct AttrImpl
    {
        static_assert(std::is_enum<enum_t>::value, "enum_t must be an enum");
        static_assert(std::is_same<std::underlying_type_t<enum_t>, size_t>::value, "enum_t underlying type must be size_t");
        
        using enum_type = enum_t;
        using value_type = value_t;
        
        static enum_type const type = static_cast<enum_type>(index_v);
        static int const flags = flags_v;
        value_type value;
    };
    
    //! @brief The template typle of an attribute of a model
    template<auto index_v, typename value_t, int flags_v> using Attr = AttrImpl<decltype(index_v), index_v, value_t, flags_v>;
    
    //! @brief The container type for a set of attributes
    template <class ..._Tp> using Container = std::tuple<_Tp...>;
    
    //! @brief The accessor a data model
    template<class parent_t, class container_t> class Accessor
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
            auto& lvalue = std::get<static_cast<size_t>(type)>(mData).value;
            if(lvalue != value)
            {
                std::get<static_cast<size_t>(type)>(mData).value = value;
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
        
        //! @brief Sets the value of an attribute (initializer list specialization)
        template <enum_type type, typename T>
        void setValue(std::initializer_list<T>&& tvalue, NotificationType notification = NotificationType::synchronous)
        {
            using attr_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            typename attr_type::value_type const value {tvalue};
            setValue<type>(value, notification);
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
                using namespace magic_enum::bitwise_operators;
                if constexpr((element_type::flags & AttrFlag::saveable) != 0)
                {
                    auto constexpr attr_type = element_type::type;
                    auto const enumname = std::string(magic_enum::enum_name(attr_type));
                    xml->setAttribute(enumname.c_str(), StringParser::toString(d.value));
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
                    anlWeakAssert(xml.hasAttribute(enumname));
                    setValue<attr_type>(StringParser::fromXml(xml, enumname, d.value), notification);
                }
            });
        }
        
        //! @brief Copy the content from another model
        void fromModel(container_type const& model, NotificationType notification = NotificationType::synchronous)
        {
            detail::for_each(mData, [&](auto& d)
            {
                using element_type = typename std::remove_reference<decltype(d)>::type;
                if constexpr((element_type::flags & AttrFlag::saveable) != 0)
                {
                    auto constexpr attr_type = element_type::type;
                    auto const& value = std::get<static_cast<size_t>(attr_type)>(model).value;
                    setValue<attr_type>(value, notification);
                }
            });
        }
        
        //! @brief Compare the content with  another model
        bool isEquivalentTo(container_type const& model)
        {
            bool result = true;
            detail::for_each(mData, [&](auto& d)
            {
                if(result)
                {
                    using element_type = typename std::remove_reference<decltype(d)>::type;
                    if constexpr((element_type::flags & AttrFlag::comparable) != 0)
                    {
                        auto constexpr attr_type = element_type::type;
                        result = areEquivalent(d.value, std::get<static_cast<size_t>(attr_type)>(model).value);
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
                
        template <typename T> static
        bool areEquivalent(T const& lhs, T const& rhs)
        {
            if constexpr(is_container<T>::value)
            {
                if(lhs.size() != rhs.size())
                {
                    return false;
                }
                return equal(lhs.cbegin(), lhs.cend(), rhs.cbegin(), [](auto const& slhs, auto const& srhs)
                {
                    return areEquivalent(slhs, srhs);
                });
            }
            else if constexpr(is_specialization<T, std::unique_ptr>::value)
            {
                return lhs != nullptr && rhs != nullptr && areEquivalent(*lhs.get(), *rhs.get());
            }
            else
            {
                return lhs == rhs;
            }
        }
        
        container_type mData;
        Notifier<Listener> mListeners;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Accessor)
    };
}

ANALYSE_FILE_END
