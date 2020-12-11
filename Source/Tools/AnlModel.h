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
        container = 1 << 3 // private
    };
    
    static constexpr int resizable = 0;
    
    //! @brief The private implementation of an attribute
    template<typename enum_t, enum_t index_v, typename value_t, int flags_v>
    struct AttrImpl
    {
        static_assert(std::is_enum<enum_t>::value, "enum_t must be an enum");
        static_assert(std::is_same<std::underlying_type_t<enum_t>, size_t>::value, "enum_t underlying type must be size_t");
        static_assert(std::is_default_constructible<value_t>::value, "value_t must be default constructible");
        static_assert(std::is_copy_constructible<value_t>::value, "value_t must be copy constructible");
        static_assert((flags_v & AttrFlag::container) == 0, "attribute cannot be a container");
        
        using enum_type = enum_t;
        using value_type = value_t;
        
        static enum_type constexpr type = static_cast<enum_type>(index_v);
        static int constexpr flags = flags_v;
        value_type value;
    };
    
    //! @brief The private implementation of an accessor
    template<typename enum_t, enum_t index_v, typename accessor_t, int flags_v, size_t size_flags_v>
    struct AcsrImp
    {
        static_assert(std::is_enum<enum_t>::value, "enum_t must be an enum");
        static_assert(std::is_same<std::underlying_type_t<enum_t>, size_t>::value, "enum_t underlying type must be size_t");
        static_assert((flags_v & AttrFlag::container) == 0, "container flag is implicit");
        
        using enum_type = enum_t;
        using accessor_type = accessor_t;
        using container_type = typename accessor_type::container_type;
        
        static enum_type constexpr type = static_cast<enum_type>(index_v);
        static int constexpr flags = flags_v | AttrFlag::container;
        static size_t constexpr size_flags = size_flags_v;
        std::vector<std::unique_ptr<accessor_type>> accessors;
        
        AcsrImp()
        {
            accessors.reserve(size_flags);
            for(size_t i = 0; i < size_flags; ++i)
            {
                accessors.push_back(std::make_unique<accessor_type>(container_type{}));
            }
        }
        
        AcsrImp(AcsrImp const& other)
        {
            auto const& acsrs = other.accessors;
            anlStrongAssert(size_flags == resizable || acsrs.size() == size_flags);
            auto const numContainers = size_flags > resizable ? size_flags : acsrs.size();
            for(size_t i = 0; i < numContainers; ++i)
            {
                anlStrongAssert(i < acsrs.size() && acsrs[i] != nullptr);
                if(i < acsrs.size() && acsrs[i] != nullptr)
                {
                    accessors.push_back(std::make_unique<accessor_type>(acsrs[i]->getContainer()));
                }
                else
                {
                    accessors.push_back(std::make_unique<accessor_type>(container_type{}));
                }
            }
        }
        
        AcsrImp(AcsrImp&& other) : accessors(std::move(other.accessors))
        {
            anlStrongAssert(size_flags == resizable || accessors.size() == size_flags);
        }
        
        AcsrImp(std::initializer_list<container_type> containers)
        {
            accessors.reserve(containers.size());
            for(auto container : containers)
            {
                accessors.push_back(std::make_unique<accessor_type>(container));
            }
            anlStrongAssert(size_flags == resizable || accessors.size() == size_flags);
        }
    };

    //! @brief The template implementation of an attribute
    template<auto index_v, typename value_t, int flags_v>
    using Attr = AttrImpl<decltype(index_v), index_v, value_t, flags_v>;
    
    //! @brief The template implementation of an accessor
    template<auto index_v, typename accessor_t, int flags_v, size_t size_v>
    using Acsr = AcsrImp<decltype(index_v), index_v, accessor_t, flags_v, size_v>;
    
    //! @brief The container of a data model of attributes and accessors
    template <class ..._Tp>
    using Container = std::tuple<_Tp...>;
    
    //! @brief The accessor a container
    template<class parent_t, class container_t>
    class Accessor
    {
    public:
        using container_type = container_t;
        using enum_type = typename std::tuple_element<0, container_type>::type::enum_type;
        
        static_assert(std::is_same<typename std::underlying_type<enum_type>::type, size_t>::value, "enum_t underlying type must be size_t");
        static_assert(is_specialization<container_type, std::tuple>::value, "container_t must be a specialization of std::tuple");
        
        //! @brief The constructor with data
        Accessor(container_type const& container)
        : mData(container)
        {
        }
        
        //! @brief The destructor
        ~Accessor() = default;

        //! @brief Gets a const ref to the container
        auto const& getContainer() const noexcept
        {
            return mData;
        }
        
        //! @brief Gets all the accessors of a container
        template <enum_type type>
        auto getAccessors() noexcept
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::container) != 0, "element is not a container");
            
            using accessor_type = typename element_type::accessor_type;
            auto& accessors = std::get<static_cast<size_t>(type)>(mData).accessors;
            std::vector<std::reference_wrapper<accessor_type>> acrs;
            acrs.reserve(accessors.size());
            for(auto& accessor : accessors)
            {
                if(accessor != nullptr)
                {
                    acrs.push_back(*accessor.get());
                }
            }
            return acrs;
        }
        
        //! @brief Gets all the accessors of a container
        template <enum_type type>
        auto getAccessors() const noexcept
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::container) != 0, "element is not a container");
            
            using accessor_type = typename element_type::accessor_type;
            auto const& accessors = std::get<static_cast<size_t>(type)>(mData).accessors;
            std::vector<std::reference_wrapper<accessor_type const>> acrs;
            acrs.reserve(accessors.size());
            for(auto const& accessor : accessors)
            {
                anlStrongAssert(accessor != nullptr);
                if(accessor != nullptr)
                {
                    acrs.push_back(*accessor.get());
                }
            }
            return acrs;
        }
        
        //! @brief Gets an accessor of a container
        template <enum_type type>
        auto& getAccessor(size_t index) noexcept
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::container) != 0, "element is not a container");
            return *std::get<static_cast<size_t>(type)>(mData).accessors[index].get();
        }
        
        //! @brief Gets an accessor of a container
        template <enum_type type>
        auto const& getAccessor(size_t index) const noexcept
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::container) != 0, "element is not a container");
            return *std::get<static_cast<size_t>(type)>(mData).accessors[index].get();
        }
        
        //! @brief Inserts a new accessor in the container
        template <enum_type type>
        bool insertAccessor(long index, NotificationType const notification)
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::container) != 0, "element is not a container");
            
            using sub_container_type = typename element_type::container_type;
            using sub_accessor_type = typename element_type::accessor_type;
            return insertAccessor(index, std::make_unique<sub_accessor_type>(sub_container_type{}), notification);
        }
        
        //! @brief Erase an accessor from the container
        template <enum_type type>
        void eraseAccessor(size_t index, NotificationType const notification)
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::container) != 0, "element is not a container");
            
            using sub_accessor_type = typename element_type::accessor_type;
            static_assert(element_type::size_flags == 0, "container is not resizable");
            
            auto& accessors = std::get<static_cast<size_t>(type)>(mData).accessors;
            auto backup = std::shared_ptr<sub_accessor_type>(accessors[index].release());
            anlWeakAssert(backup != nullptr);
            accessors.erase(accessors.begin() + static_cast<long>(index));
            
            // Detaches the mutex that prevent recurvise changes
            backup->setLock(nullptr);
            
            if constexpr((element_type::flags & AttrFlag::notifying) != 0)
            {
                mListeners.notify([this](Listener& listener) mutable
                {
                    anlWeakAssert(listener.onChanged != nullptr);
                    if(listener.onChanged != nullptr)
                    {
                        listener.onChanged(*static_cast<parent_t const*>(this), type);
                    }
                }, notification);
            }
        }
        
        //! @brief Gets an attribute from the container
        template <enum_type type>
        auto const& getAttr() const noexcept
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::container) == 0, "element is a not an attribute");
            return std::get<static_cast<size_t>(type)>(mData).value;
        }
        
        //! @brief Sets the value of an attribute
        //! @details If the value changed and the attribute is marked as notifying, the method notifies the listeners .
        template <enum_type type, typename T>
        void setAttr(T const& value, NotificationType const notification)
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::container) == 0, "element is a not an attribute");
            
            using value_type = typename element_type::value_type;
            if constexpr(std::is_same<value_type, T>::value)
            {
                auto& lock = getLock();
                auto const canAccess = lock.exchange(false);
                anlStrongAssert(canAccess == true);
                if(!canAccess)
                {
                    return;
                }
                
                auto& lvalue = std::get<static_cast<size_t>(type)>(mData).value;
                if(isEquivalentTo(lvalue, value) == false)
                {
                    setValue<value_type>(lvalue, value);
                    if(onUpdated != nullptr)
                    {
                        onUpdated(type, notification);
                    }
                    if constexpr((element_type::flags & AttrFlag::notifying) != 0)
                    {
                        mListeners.notify([=, this](Listener& listener)
                        {
                            anlWeakAssert(listener.onChanged != nullptr);
                            if(listener.onChanged != nullptr)
                            {
                                listener.onChanged(*static_cast<parent_t const*>(this), type);
                            }
                        }, notification);
                    }
                }
                lock.store(true);
            }
            else
            {
                value_type const tvalue(value);
                static_cast<parent_t*>(this)->template setAttr<type>(tvalue, notification);
            }
        }
        
        //! @brief Sets the value of an attribute (initializer list specialization)
        template <enum_type type, typename T>
        void setAttr(std::initializer_list<T>&& tvalue, NotificationType const notification)
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::container) == 0, "element is a not an attribute");
            
            typename element_type::value_type const value {tvalue};
            static_cast<parent_t*>(this)->template setAttr<type>(value, notification);
        }
        
        //! @brief Parse the container to xml
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
                    static auto const enumname = std::string(magic_enum::enum_name(attr_type));
                    
                    if constexpr((element_type::flags & AttrFlag::container) != 0)
                    {
                        auto const acsrs = getAccessors<attr_type>();
                        for(auto const& acsr : acsrs)
                        {
                            auto child = acsr.get().toXml(enumname.c_str());
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
        
        //! @brief Parse the container from xml
        //! @details Only the saveable attributes are restored from the xml.
        //! If the value changed and the attribute is marked as notifying, the method notifies the listeners .
        void fromXml(juce::XmlElement const& xml, juce::StringRef const& name, NotificationType const notification)
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
                    
                    if constexpr((element_type::flags & AttrFlag::container) != 0)
                    {
                        std::vector<juce::XmlElement const*> childs;
                        for(auto* child = xml.getChildByName(enumname.c_str()); child != nullptr; child = child->getNextElementWithTagName(enumname.c_str()))
                        {
                            childs.push_back(child);
                        }
                        
                        auto& accessors = std::get<static_cast<size_t>(attr_type)>(mData).accessors;
                        anlWeakAssert(element_type::size_flags == 0 || childs.size() == accessors.size());
                        if constexpr(element_type::size_flags == 0)
                        {
                            while(accessors.size() > childs.size())
                            {
                                auto const index = accessors.size() - 1;
                                static_cast<parent_t*>(this)->template eraseAccessor<attr_type>(index, notification);
                            }
                        }
                        for(size_t index = 0; index < std::min(accessors.size(), childs.size()); ++index)
                        {
                            anlStrongAssert(accessors[index] != nullptr);
                            if(accessors[index] != nullptr)
                            {
                                accessors[index]->fromXml(*childs[index], enumname.c_str(), notification);
                            }
                        }
                        if constexpr(element_type::size_flags == 0)
                        {
                            while(childs.size() > accessors.size())
                            {
                                auto const index = accessors.size();
                                if(static_cast<parent_t*>(this)->template insertAccessor<attr_type>(static_cast<long>(index), notification))
                                {
                                    if(accessors[index] != nullptr)
                                    {
                                        accessors[index]->fromXml(*childs[index], enumname.c_str(), notification);
                                    }
                                }
                                else
                                {
                                    anlStrongAssert(false && "allocation failed");
                                }
                            }
                        }
                    }
                    else
                    {
                        static_cast<parent_t*>(this)->template setAttr<attr_type>(XmlParser::fromXml(xml, enumname.c_str(), d.value), notification);
                    }
                }
            });
        }
        
        //! @brief Copy the content from another container
        void fromContainer(container_type const& container, NotificationType const notification)
        {
            detail::for_each(container, [&](auto const& d)
            {
                using element_type = typename std::remove_reference<decltype(d)>::type;
                if constexpr((element_type::flags & AttrFlag::saveable) != 0)
                {
                    auto constexpr attr_type = element_type::type;
                    if constexpr((element_type::flags & AttrFlag::container) != 0)
                    {
                        auto& accessors = std::get<static_cast<size_t>(attr_type)>(mData).accessors;
                        anlStrongAssert(element_type::size_flags == 0 || d.accessors.size() == accessors.size());
                        if constexpr(element_type::size_flags == 0)
                        {
                            while(accessors.size() > d.accessors.size())
                            {
                                auto const index = accessors.size() - 1;
                                static_cast<parent_t*>(this)->template eraseAccessor<attr_type>(index, notification);
                            }
                        }
                        for(size_t index = 0; index < std::min(accessors.size(), d.accessors.size()); ++index)
                        {
                            anlStrongAssert(accessors[index] != nullptr && d.accessors[index] != nullptr);
                            if(accessors[index] != nullptr && d.accessors[index] != nullptr)
                            {
                                accessors[index]->fromContainer(d.accessors[index]->getContainer(), notification);
                            }
                        }
                        if constexpr(element_type::size_flags == 0)
                        {
                            while(d.accessors.size() > accessors.size())
                            {
                                auto const index = accessors.size();
                                anlStrongAssert(d.accessors[index] != nullptr);
                                if(d.accessors[index] != nullptr)
                                {
                                    if(static_cast<parent_t*>(this)->template insertAccessor<attr_type>(static_cast<long>(index), notification))
                                    {
                                        if(accessors[index] != nullptr)
                                        {
                                            accessors[index]->fromContainer(d.accessors[index]->getContainer(), notification);
                                        }
                                    }
                                    else
                                    {
                                        anlStrongAssert(false && "allocation failed");
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        static_cast<parent_t*>(this)->template setAttr<attr_type>(d.value, notification);
                    }
                    
                }
            });
        }
        
        //! @brief Compare the content with another container
        bool isEquivalentTo(container_type const& container) const
        {
            bool result = true;
            detail::for_each(container, [&](auto& d)
            {
                if(result)
                {
                    using element_type = typename std::remove_reference<decltype(d)>::type;
                    auto constexpr attr_type = element_type::type;
                    if constexpr((element_type::flags & AttrFlag::comparable) != 0)
                    {
                        if constexpr((element_type::flags & AttrFlag::container) != 0)
                        {
                            auto const acsrs = getAccessors<attr_type>();
                            result = acsrs.size() != d.accessors.size() || std::equal(acsrs.cbegin(), acsrs.cend(), d.accessors.cbegin(), [](auto const& acsr, auto const& ctnr)
                            {
                                return ctnr != nullptr && acsr.get().isEquivalentTo(ctnr->getContainer());
                            });
                        }
                        else
                        {
                            result = isEquivalentTo(getAttr<attr_type>(), d.value);
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
                auto& lock = getLock();
                auto const isLocked = lock.exchange(false);
                
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
                
                lock.store(isLocked);
            }
        }
        
        void removeListener(Listener& listener)
        {
            mListeners.remove(listener);
        }
        
        std::function<void(enum_type type, NotificationType notification)> onUpdated = nullptr;
        
    protected:
        
        //! @brief Inserts a new accessor in the container
        template <enum_type type>
        bool insertAccessor(long index, std::unique_ptr<typename std::tuple_element<static_cast<size_t>(type), container_type>::type::accessor_type> accessor, NotificationType const notification)
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::container) != 0, "element is not a container");
            
            anlStrongAssert(accessor != nullptr);
            if(accessor == nullptr)
            {
                return false;
            }
            
            auto& accessors = std::get<static_cast<size_t>(type)>(mData).accessors;
            index = index < 0 ? static_cast<long>(accessors.size()) : std::max(index, static_cast<long>(accessors.size()));
            auto it = accessors.insert(accessors.begin() + index, std::move(accessor));
            if(it != accessors.end())
            {
                // Attaches the mutex to prevent recursive changes
                (*it)->setLock(&getLock());
                
                if(onUpdated != nullptr)
                {
                    onUpdated(type, notification);
                }
                if constexpr((element_type::flags & AttrFlag::notifying) != 0)
                {
                    mListeners.notify([=, this](Listener& listener)
                    {
                        anlWeakAssert(listener.onChanged != nullptr);
                        if(listener.onChanged != nullptr)
                        {
                            listener.onChanged(*static_cast<parent_t const*>(this), type);
                        }
                    }, notification);
                }
            }
            return true;
        }
        
    private:
        
        void setLock(std::atomic<bool>* lock)
        {
            mSharedLock = lock;
        }
        
        std::atomic<bool>& getLock()
        {
            return mSharedLock == nullptr ? mLock : *mSharedLock;
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
                
        // This is a specific compare method that manage containers and unique pointer
        template <typename T> static
        bool isEquivalentTo(T const& lhs, T const& rhs)
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
                    return isEquivalentTo(slhs, srhs);
                });
            }
            else if constexpr(is_specialization<T, std::unique_ptr>::value)
            {
                return lhs != nullptr && rhs != nullptr && isEquivalentTo(*lhs.get(), *rhs.get());
            }
            else
            {
                return lhs == rhs;
            }
        }

        
        // This is a specific assignement method that manage containers and unique pointer
        template <typename T> static
        void setValue(T& lhs, T const& rhs)
        {
            if constexpr(is_specialization<T, std::vector>::value)
            {
                lhs.resize(rhs.size());
                for(size_t i = 0; i < lhs.size(); ++i)
                {
                    setValue(lhs[i], rhs[i]);
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
                    setValue(lhs[pair.first], pair.second);
                }
            }
            else if constexpr(is_specialization<T, std::unique_ptr>::value)
            {
                if(lhs != nullptr && rhs != nullptr)
                {
                    setValue(*lhs.get(), *rhs.get());
                }
            }
            else
            {
                lhs = rhs;
            }
        }
        
        container_type mData;
        Notifier<Listener> mListeners;
        
        std::atomic<bool> mLock {true};
        std::atomic<bool>* mSharedLock = nullptr;
        
        template<class, class> friend class Accessor;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Accessor)
    };
}

ANALYSE_FILE_END
