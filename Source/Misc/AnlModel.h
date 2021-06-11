#pragma once

#include "AnlNotifier.h"
#include "AnlXmlParser.h"
#include <magic_enum/magic_enum.hpp>

ANALYSE_FILE_BEGIN

namespace Model
{
    // clang-format off
    enum Flag
    {
          ignored = 0 << 0
        , notifying = 1 << 0
        , saveable = 1 << 1
        , comparable = 1 << 2
        , basic = notifying | saveable | comparable
    };
    // clang-format on

    static constexpr size_t resizable = 0;

    //! @brief The private implementation of an attribute
    template <typename enum_t, enum_t index_v, typename value_t, int flags_v>
    struct AttrImpl
    {
        static_assert(std::is_enum<enum_t>::value, "enum_t must be an enum");
        static_assert(std::is_same<std::underlying_type_t<enum_t>, size_t>::value, "enum_t underlying type must be size_t");
        static_assert(std::is_default_constructible<value_t>::value, "value_t must be default constructible");
        static_assert(std::is_copy_constructible<value_t>::value, "value_t must be copy constructible");

        using enum_type = enum_t;
        using value_type = value_t;

        static enum_type constexpr type = static_cast<enum_type>(index_v);
        static int constexpr flags = flags_v;
        value_type value;
    };

    //! @brief The private implementation of an accessor
    template <typename enum_t, enum_t index_v, typename accessor_t, int flags_v, size_t size_flags_v>
    struct AcsrImp
    {
        static_assert(std::is_enum<enum_t>::value, "enum_t must be an enum");
        static_assert(std::is_same<std::underlying_type_t<enum_t>, size_t>::value, "enum_t underlying type must be size_t");

        using enum_type = enum_t;
        using accessor_type = accessor_t;
        using attr_container_type = typename accessor_type::attr_container_type;

        static enum_type constexpr type = static_cast<enum_type>(index_v);
        static int constexpr flags = flags_v;
        static size_t constexpr size_flags = size_flags_v;
        std::vector<std::unique_ptr<accessor_type>> accessors;
    };

    //! @brief The template implementation of an attribute
    template <auto index_v, typename value_t, int flags_v>
    using Attr = AttrImpl<decltype(index_v), index_v, value_t, flags_v>;

    //! @brief The template implementation of an accessor
    template <auto index_v, typename accessor_t, int flags_v, size_t size_v>
    using Acsr = AcsrImp<decltype(index_v), index_v, accessor_t, flags_v, size_v>;

    //! @brief The container of a data model of attributes and accessors
    template <class... _Tp>
    using Container = std::tuple<_Tp...>;

    // Private
    struct default_empty_accessor
    {
        enum enum_type : size_t
        {
        };
        static int constexpr flags = Flag::ignored;
    };
    using default_empty_container = Container<default_empty_accessor>;

    //! @brief The accessor a container
    //! @todo Check insertion of non resizable accessors
    template <class parent_t, class attr_container_t, class acsr_container_t = default_empty_container>
    class Accessor
    {
    public:
        using attr_container_type = attr_container_t;
        using attr_enum_type = typename std::tuple_element<0, attr_container_type>::type::enum_type;

        using acsr_container_type = acsr_container_t;
        using acsr_enum_type = typename std::tuple_element<0, acsr_container_type>::type::enum_type;

        static_assert(is_specialization<attr_container_type, std::tuple>::value, "attr_container_type must be a specialization of std::tuple");
        static_assert(std::is_same<typename std::underlying_type<attr_enum_type>::type, size_t>::value, "attr_enum_type underlying type must be size_t");

        static_assert(is_specialization<acsr_container_type, std::tuple>::value, "acsr_container_type must be a specialization of std::tuple");
        static_assert(std::is_same<typename std::underlying_type<acsr_enum_type>::type, size_t>::value, "acsr_enum_type underlying type must be size_t");

        //! @brief The constructor with data
        Accessor(attr_container_type const& container = {})
        : mAttributes(container)
        {
            if constexpr(std::is_same<acsr_container_type, default_empty_container>::value == false)
            {
                detail::for_each(mAccessors, [&](auto const& d)
                                 {
                                     using element_type = typename std::remove_reference<decltype(d)>::type;
                                     auto constexpr acsr_type = element_type::type;
                                     auto constexpr size_flags = element_type::size_flags;
                                     for(size_t index = 0; index < size_flags; ++index)
                                     {
                                         if(!static_cast<parent_t*>(this)->template insertAcsr<acsr_type>(index, NotificationType::synchronous))
                                         {
                                             anlStrongAssert(false && "allocation failed");
                                         }
                                     }
                                 });
            }
        }

        //! @brief The destructor
        virtual ~Accessor()
        {
            if constexpr(std::is_same<acsr_container_type, default_empty_container>::value == false)
            {
                detail::for_each(mAccessors, [&](auto const& d)
                                 {
                                     using element_type = typename std::remove_reference<decltype(d)>::type;
                                     auto constexpr acsr_type = element_type::type;
                                     while(getNumAcsrs<acsr_type>() > 0)
                                     {
                                         eraseAcsr<acsr_type>(getNumAcsrs<acsr_type>() - 1, NotificationType::synchronous);
                                     }
                                 });
            }
        }

        //! @brief Gets an accessor of a container
        template <acsr_enum_type type>
        auto getNumAcsrs() const noexcept
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            return std::get<static_cast<size_t>(type)>(mAccessors).accessors.size();
        }

        //! @brief Gets all the accessors of a container
        template <acsr_enum_type type>
        auto getAcsrs() noexcept
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            using element_type = typename std::tuple_element<static_cast<size_t>(type), acsr_container_type>::type;

            using accessor_type = typename element_type::accessor_type;
            auto& accessors = std::get<static_cast<size_t>(type)>(mAccessors).accessors;
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
        template <acsr_enum_type type>
        auto getAcsrs() const noexcept
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            using element_type = typename std::tuple_element<static_cast<size_t>(type), acsr_container_type>::type;

            using accessor_type = typename element_type::accessor_type;
            auto const& accessors = std::get<static_cast<size_t>(type)>(mAccessors).accessors;
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
        template <acsr_enum_type type>
        auto& getAcsr() noexcept
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            using element_type = typename std::tuple_element<static_cast<size_t>(type), acsr_container_type>::type;
            anlWeakAssert(element_type::size_flags == 1);
            return *std::get<static_cast<size_t>(type)>(mAccessors).accessors[0].get();
        }

        //! @brief Gets an accessor of a container
        template <acsr_enum_type type>
        auto const& getAcsr() const noexcept
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            using element_type = typename std::tuple_element<static_cast<size_t>(type), acsr_container_type>::type;
            anlWeakAssert(element_type::size_flags == 1);
            return *std::get<static_cast<size_t>(type)>(mAccessors).accessors[0].get();
        }

        //! @brief Gets an accessor of a container
        template <acsr_enum_type type>
        auto& getAcsr(size_t index) noexcept
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            using element_type = typename std::tuple_element<static_cast<size_t>(type), acsr_container_type>::type;
            anlWeakAssert(element_type::size_flags == 0);
            return *std::get<static_cast<size_t>(type)>(mAccessors).accessors[index].get();
        }

        //! @brief Gets an accessor of a container
        template <acsr_enum_type type>
        auto const& getAcsr(size_t index) const noexcept
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            using element_type = typename std::tuple_element<static_cast<size_t>(type), acsr_container_type>::type;
            anlWeakAssert(element_type::size_flags == 0);
            return *std::get<static_cast<size_t>(type)>(mAccessors).accessors[index].get();
        }

        //! @brief Inserts a new accessor in the container
        template <acsr_enum_type type>
        bool insertAcsr(size_t index, NotificationType const notification)
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            using element_type = typename std::tuple_element<static_cast<size_t>(type), acsr_container_type>::type;
            using sub_accessor_type = typename element_type::accessor_type;
            return insertAcsr<type>(index, std::make_unique<sub_accessor_type>(), notification);
        }

        //! @brief Erase an accessor from the container
        template <acsr_enum_type type>
        void eraseAcsr(size_t index, NotificationType const notification)
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            auto& lock = getLock();
            auto const canAccess = lock.exchange(false);
            anlStrongAssert(canAccess == true);
            if(!canAccess)
            {
                return;
            }

            using element_type = typename std::tuple_element<static_cast<size_t>(type), acsr_container_type>::type;

            using sub_accessor_type = typename element_type::accessor_type;

            auto& accessors = std::get<static_cast<size_t>(type)>(mAccessors).accessors;
            auto backup = std::shared_ptr<sub_accessor_type>(accessors[index].release());
            anlWeakAssert(backup != nullptr);

            accessors.erase(accessors.begin() + static_cast<long>(index));

            if constexpr((element_type::flags & Flag::notifying) != 0)
            {
                mListeners.notify([=, this](Listener& listener) mutable
                                  {
                                      if(listener.onAccessorErased != nullptr)
                                      {
                                          listener.onAccessorErased(*static_cast<parent_t const*>(this), type, index);
                                      }
                                  },
                                  notification);
            }
            if(onAccessorErased != nullptr)
            {
                lock.store(true);
                onAccessorErased(type, index, notification);
                lock.store(false);
            }

            // Detaches the mutex that prevent recurvise changes
            backup->setLock(nullptr);
            lock = true;
        }

        //! @brief Gets an attribute from the container
        template <attr_enum_type type>
        auto const& getAttr() const noexcept
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            return std::get<static_cast<size_t>(type)>(mAttributes).value;
        }

        //! @brief Sets the value of an attribute
        //! @details If the value changed and the attribute is marked as notifying, the method notifies the listeners .
        template <attr_enum_type type, typename T>
        void setAttr(T const& value, NotificationType const notification)
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            using element_type = typename std::tuple_element<static_cast<size_t>(type), attr_container_type>::type;

            using value_type = typename element_type::value_type;
            if constexpr(std::is_same<value_type, T>::value)
            {
                auto& lock = getLock();
                auto const canAccess = lock.exchange(false);
                anlWeakAssert(canAccess == true);
                if(!canAccess)
                {
                    return;
                }

                auto& lvalue = std::get<static_cast<size_t>(type)>(mAttributes).value;
                if(isEquivalentTo(lvalue, value) == false)
                {
                    lvalue = value;
                    if(onAttrUpdated != nullptr)
                    {
                        lock.store(true);
                        onAttrUpdated(type, notification);
                        lock.store(false);
                    }

                    if constexpr((element_type::flags & Flag::notifying) != 0)
                    {
                        mListeners.notify([=, this](Listener& listener)
                                          {
                                              if(listener.onAttrChanged != nullptr)
                                              {
                                                  listener.onAttrChanged(*static_cast<parent_t const*>(this), type);
                                              }
                                          },
                                          notification);
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
        template <attr_enum_type type, typename T>
        void setAttr(std::initializer_list<T>&& tvalue, NotificationType const notification)
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            using element_type = typename std::tuple_element<static_cast<size_t>(type), attr_container_type>::type;
            typename element_type::value_type const value{tvalue};
            static_cast<parent_t*>(this)->template setAttr<type>(value, notification);
        }

        //! @brief Parse the container to xml
        //! @details Only the saveable attributes are stored into the xml
        auto toXml(juce::StringRef const& name) const
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            auto xml = std::make_unique<juce::XmlElement>(name);
            anlWeakAssert(xml != nullptr);
            if(xml == nullptr)
            {
                return std::unique_ptr<juce::XmlElement>();
            }
            xml->setAttribute("AnlModelVersion", ProjectInfo::versionNumber);

            detail::for_each(mAttributes, [&](auto const& d)
                             {
                                 using element_type = typename std::remove_reference<decltype(d)>::type;
                                 if constexpr((element_type::flags & Flag::saveable) != 0)
                                 {
                                     auto constexpr attr_type = element_type::type;
                                     static auto const enumname = std::string(magic_enum::enum_name(attr_type));
                                     XmlParser::toXml(*xml.get(), enumname.c_str(), d.value);
                                 }
                             });

            detail::for_each(mAccessors, [&](auto const& d)
                             {
                                 using element_type = typename std::remove_reference<decltype(d)>::type;
                                 if constexpr((element_type::flags & Flag::saveable) != 0)
                                 {
                                     auto constexpr acsr_type = element_type::type;
                                     static auto const enumname = std::string(magic_enum::enum_name(acsr_type));
                                     auto const acsrs = getAcsrs<acsr_type>();
                                     for(auto const& acsr : acsrs)
                                     {
                                         auto child = acsr.get().toXml(enumname.c_str());
                                         if(child != nullptr)
                                         {
                                             xml->addChildElement(child.release());
                                         }
                                     }
                                 }
                             });
            return xml;
        }

        auto toJson()
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            nlohmann::json json;
            detail::for_each(mAttributes, [&](auto const& d)
                             {
                                 using element_type = typename std::remove_reference<decltype(d)>::type;
                                 if constexpr((element_type::flags & Flag::saveable) != 0)
                                 {
                                     json[std::string(magic_enum::enum_name(element_type::type))] = d.value;
                                 }
                             });

            detail::for_each(mAccessors, [&](auto const& d)
                             {
                                 using element_type = typename std::remove_reference<decltype(d)>::type;
                                 if constexpr((element_type::flags & Flag::saveable) != 0)
                                 {
                                     auto& j = json[std::string(magic_enum::enum_name(element_type::type))];
                                     for(auto const& acsr : getAcsrs<element_type::type>())
                                     {
                                         j.push_back(acsr.get().toJson());
                                     }
                                 }
                             });
            return json;
        }

        void fromJson(nlohmann::json const& json, NotificationType const notification)
        {
            Accessor temporary;
            fromJson(temporary, json);
            copyFrom(temporary, notification);
        }

        //! @brief Parse the container from xml
        //! @details Only the saveable attributes are restored from the xml.
        //! If the value changed and the attribute is marked as notifying, the method notifies the listeners .
        void fromXml(juce::XmlElement const& xml, juce::StringRef const& name, NotificationType const notification)
        {
            Accessor temporary;
            auto copy = parseXml(xml, xml.getIntAttribute("AnlModelVersion", 0));
            if(copy != nullptr)
            {
                fromXml(temporary, *copy.get(), name);
            }
            copyFrom(temporary, notification);
        }

        //! @brief Copy the content from another container
        void copyFrom(Accessor const& accessor, NotificationType const notification)
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            detail::for_each(accessor.mAttributes, [&](auto const& d)
                             {
                                 using element_type = typename std::remove_reference<decltype(d)>::type;
                                 if constexpr((element_type::flags & Flag::saveable) != 0)
                                 {
                                     auto constexpr attr_type = element_type::type;
                                     static_cast<parent_t*>(this)->template setAttr<attr_type>(d.value, notification);
                                 }
                             });

            // Remove accessors
            detail::for_each_inv(accessor.mAccessors, [&](auto const& d)
                                 {
                                     using element_type = typename std::remove_reference<decltype(d)>::type;
                                     if constexpr((element_type::flags & Flag::saveable) != 0)
                                     {
                                         auto constexpr acsr_type = element_type::type;
                                         auto& accessors = std::get<static_cast<size_t>(acsr_type)>(mAccessors).accessors;
                                         anlStrongAssert(element_type::size_flags == 0 || d.accessors.size() == accessors.size());

                                         if constexpr(element_type::size_flags == 0)
                                         {
                                             for(auto index = accessors.size(); index > 0_z; --index)
                                             {
                                                 auto const& acsrPtr = accessors[index - 1].get();
                                                 anlStrongAssert(acsrPtr != nullptr);
                                                 if(acsrPtr == nullptr || static_cast<parent_t const*>(&accessor)->template getAcsrPosition<acsr_type>(*acsrPtr) == npos)
                                                 {
                                                     static_cast<parent_t*>(this)->template eraseAcsr<acsr_type>(index - 1, notification);
                                                 }
                                             }
                                         }
                                     }
                                 });

            // Insert accessors
            detail::for_each(accessor.mAccessors, [&](auto const& d)
                             {
                                 using element_type = typename std::remove_reference<decltype(d)>::type;
                                 if constexpr((element_type::flags & Flag::saveable) != 0)
                                 {
                                     auto constexpr acsr_type = element_type::type;
                                     auto& accessors = std::get<static_cast<size_t>(acsr_type)>(mAccessors).accessors;
                                     anlStrongAssert(element_type::size_flags == 0 || d.accessors.size() == accessors.size());
                                     if constexpr(element_type::size_flags == 0)
                                     {
                                         for(auto index = 0_z; index < d.accessors.size(); ++index)
                                         {
                                             auto const& acsrPtr = d.accessors[index].get();
                                             anlStrongAssert(acsrPtr != nullptr);
                                             if(acsrPtr != nullptr && static_cast<parent_t*>(this)->template getAcsrPosition<acsr_type>(*acsrPtr) == npos)
                                             {
                                                 if(!static_cast<parent_t*>(this)->template insertAcsr<acsr_type>(index, notification))
                                                 {
                                                     anlWeakAssert(false);
                                                 }
                                             }
                                         }
                                     }
                                 }
                             });

            // Updates accessors
            detail::for_each(accessor.mAccessors, [&](auto const& d)
                             {
                                 using element_type = typename std::remove_reference<decltype(d)>::type;
                                 if constexpr((element_type::flags & Flag::saveable) != 0)
                                 {
                                     auto constexpr acsr_type = element_type::type;
                                     auto& accessors = std::get<static_cast<size_t>(acsr_type)>(mAccessors).accessors;
                                     anlStrongAssert(d.accessors.size() == accessors.size());
                                     for(size_t index = 0; index < std::min(accessors.size(), d.accessors.size()); ++index)
                                     {
                                         if constexpr(element_type::size_flags == 0)
                                         {
                                             anlStrongAssert(accessors[index] != nullptr);
                                             if(accessors[index] != nullptr)
                                             {
                                                 auto const position = static_cast<parent_t*>(this)->template getAcsrPosition<acsr_type>(*accessors[index]);
                                                 anlStrongAssert(position != npos);
                                                 if(position != npos && d.accessors[position] != nullptr)
                                                 {
                                                     accessors[index]->copyFrom(*(d.accessors[position].get()), notification);
                                                 }
                                             }
                                         }
                                         else
                                         {
                                             anlStrongAssert(accessors[index] != nullptr && d.accessors[index] != nullptr);
                                             if(accessors[index] != nullptr && d.accessors[index] != nullptr)
                                             {
                                                 accessors[index]->copyFrom(*(d.accessors[index].get()), notification);
                                             }
                                         }
                                     }
                                 }
                             });
        }

        //! @brief Compare the content with accessor
        bool isEquivalentTo(Accessor const& accessor) const
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            bool result = true;
            detail::for_each(accessor.mAttributes, [&](auto& d)
                             {
                                 if(result)
                                 {
                                     using element_type = typename std::remove_reference<decltype(d)>::type;
                                     if constexpr((element_type::flags & Flag::comparable) != 0)
                                     {
                                         auto constexpr attr_type = element_type::type;
                                         result = isEquivalentTo(getAttr<attr_type>(), d.value);
                                     }
                                 }
                             });

            detail::for_each(accessor.mAccessors, [&](auto& d)
                             {
                                 using element_type = typename std::remove_reference<decltype(d)>::type;
                                 if constexpr((element_type::flags & Flag::comparable) != 0)
                                 {
                                     if(!result)
                                     {
                                         return;
                                     }
                                     auto constexpr acsr_type = element_type::type;
                                     auto const acsrs = getAcsrs<acsr_type>();
                                     result = acsrs.size() == d.accessors.size();
                                     if(!result)
                                     {
                                         return;
                                     }
                                     result = std::equal(acsrs.cbegin(), acsrs.cend(), d.accessors.cbegin(), [](auto const& acsr, auto const& ctnr)
                                                         {
                                                             return ctnr != nullptr && acsr.get().isEquivalentTo(*(ctnr.get()));
                                                         });
                                 }
                             });
            return result;
        }

        class Listener
        {
        public:
            Listener() = default;
            virtual ~Listener() = default;

            std::function<void(parent_t const&, attr_enum_type)> onAttrChanged = nullptr;
            std::function<void(parent_t const&, acsr_enum_type, size_t)> onAccessorInserted = nullptr;
            std::function<void(parent_t const&, acsr_enum_type, size_t)> onAccessorErased = nullptr;
        };

        void addListener(Listener& listener, NotificationType const notification)
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            if(mListeners.add(listener))
            {
                detail::for_each(mAttributes, [&](auto& d)
                                 {
                                     using element_type = typename std::remove_reference<decltype(d)>::type;
                                     if constexpr((element_type::flags & Flag::notifying) != 0)
                                     {
                                         mListeners.notify([this, ptr = &listener](Listener& ltnr)
                                                           {
                                                               if(&ltnr == ptr && ltnr.onAttrChanged != nullptr)
                                                               {
                                                                   ltnr.onAttrChanged(*static_cast<parent_t const*>(this), element_type::type);
                                                               }
                                                           },
                                                           notification);
                                     }
                                 });

                detail::for_each(mAccessors, [&](auto& d)
                                 {
                                     using element_type = typename std::remove_reference<decltype(d)>::type;
                                     if constexpr((element_type::flags & Flag::notifying) != 0)
                                     {
                                         auto const acsrs = getAcsrs<element_type::type>();
                                         for(size_t index = 0; index < acsrs.size(); ++index)
                                         {
                                             mListeners.notify([this, index, ptr = &listener](Listener& ltnr)
                                                               {
                                                                   if(&ltnr == ptr && ltnr.onAccessorInserted != nullptr)
                                                                   {
                                                                       ltnr.onAccessorInserted(*static_cast<parent_t const*>(this), element_type::type, index);
                                                                   }
                                                               },
                                                               notification);
                                         }
                                     }
                                 });
            }
        }

        void removeListener(Listener& listener)
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            mListeners.remove(listener);
        }

        std::function<void(attr_enum_type type, NotificationType notification)> onAttrUpdated = nullptr;
        std::function<void(acsr_enum_type type, size_t index, NotificationType notification)> onAccessorInserted = nullptr;
        std::function<void(acsr_enum_type type, size_t index, NotificationType notification)> onAccessorErased = nullptr;

    protected:
        //! @brief Inserts a new accessor in the container
        template <acsr_enum_type type>
        bool insertAcsr(size_t index, std::unique_ptr<typename std::tuple_element<static_cast<size_t>(type), acsr_container_type>::type::accessor_type> accessor, NotificationType const notification)
        {
            anlStrongAssert(accessor != nullptr);
            if(accessor == nullptr)
            {
                return false;
            }

            auto& lock = getLock();
            auto const canAccess = lock.exchange(false);
            anlStrongAssert(canAccess == true);
            if(!canAccess)
            {
                return false;
            }

            auto& accessors = std::get<static_cast<size_t>(type)>(mAccessors).accessors;
            auto it = accessors.insert(accessors.begin() + static_cast<long>(index), std::move(accessor));
            if(it != accessors.end())
            {
                // Attaches the mutex to prevent recursive changes
                (*it)->setLock(&getLock());
                lock = true;
                notifyAccessorInsertion<type>(index, notification);
                return true;
            }
            lock = true;
            return false;
        }

        static constexpr size_t npos = static_cast<size_t>(-1);
        template <acsr_enum_type type>
        size_t getAcsrPosition(typename std::tuple_element<static_cast<size_t>(type), acsr_container_type>::type::accessor_type const& other) const
        {
            juce::ignoreUnused(other);
            return npos;
        }

        virtual std::unique_ptr<juce::XmlElement> parseXml(juce::XmlElement const& xml, int version)
        {
            juce::ignoreUnused(version);
            return std::make_unique<juce::XmlElement>(xml);
        }

    private:
        static auto fromJson(Accessor& accessor, nlohmann::json const& json)
        {
            detail::for_each(accessor.mAttributes, [&](auto& d)
                             {
                                 using element_type = typename std::remove_reference<decltype(d)>::type;
                                 if constexpr((element_type::flags & Flag::saveable) != 0)
                                 {
                                     auto it = json.find(std::string(magic_enum::enum_name(element_type::type)));
                                     anlWeakAssert(it != json.cend());
                                     if(it != json.cend())
                                     {
                                         it->get_to(d.value);
                                     }
                                 }
                             });

            detail::for_each(accessor.mAccessors, [&](auto& d)
                             {
                                 using element_type = typename std::remove_reference<decltype(d)>::type;
                                 if constexpr((element_type::flags & Flag::saveable) != 0)
                                 {
                                     auto it = json.find(std::string(magic_enum::enum_name(element_type::type)));
                                     anlWeakAssert(it != json.cend());
                                     if(it == json.cend())
                                     {
                                         return;
                                     }
                                     auto& accessors = std::get<static_cast<size_t>(element_type::type)>(accessor.mAccessors).accessors;
                                     anlWeakAssert((element_type::size_flags == 0 && accessors.empty()) || (element_type::size_flags != 0 && it->size() == accessors.size()));
                                     if constexpr(element_type::size_flags == 0)
                                     {
                                         for(auto index = 0_z; index < it->size(); ++index)
                                         {
                                             if(static_cast<parent_t*>(&accessor)->template insertAcsr<element_type::type>(index, NotificationType::synchronous))
                                             {
                                                 anlWeakAssert(accessors[index] != nullptr);
                                                 if(accessors[index] != nullptr)
                                                 {
                                                     fromJson(*accessors[index].get(), it->at(index));
                                                 }
                                             }
                                         }
                                     }
                                     else
                                     {
                                         for(auto index = 0_z; index < std::min(accessors.size(), it->size()); ++index)
                                         {
                                             anlWeakAssert(accessors[index] != nullptr);
                                             if(accessors[index] != nullptr)
                                             {
                                                 fromJson(*accessors[index].get(), it->at(index));
                                             }
                                         }
                                     }
                                 }
                             });
        }

        static void fromXml(Accessor& accessor, juce::XmlElement const& xml, juce::StringRef const& name)
        {
            anlWeakAssert(juce::MessageManager::existsAndIsLockedByCurrentThread());
            anlWeakAssert(xml.hasTagName(name));
            if(!xml.hasTagName(name))
            {
                return;
            }

            detail::for_each(accessor.mAttributes, [&](auto& d)
                             {
                                 using element_type = typename std::remove_reference<decltype(d)>::type;
                                 if constexpr((element_type::flags & Flag::saveable) != 0)
                                 {
                                     auto constexpr attr_type = element_type::type;
                                     auto const enumname = std::string(magic_enum::enum_name(attr_type));
                                     static_cast<parent_t*>(&accessor)->template setAttr<attr_type>(XmlParser::fromXml(xml, enumname.c_str(), d.value), NotificationType::synchronous);
                                 }
                             });

            detail::for_each(accessor.mAccessors, [&](auto& d)
                             {
                                 using element_type = typename std::remove_reference<decltype(d)>::type;
                                 if constexpr((element_type::flags & Flag::saveable) != 0)
                                 {
                                     auto constexpr acsr_type = element_type::type;
                                     auto const enumname = std::string(magic_enum::enum_name(acsr_type));

                                     std::vector<juce::XmlElement const*> childs;
                                     for(auto* child = xml.getChildByName(enumname.c_str()); child != nullptr; child = child->getNextElementWithTagName(enumname.c_str()))
                                     {
                                         childs.push_back(child);
                                     }

                                     auto& accessors = std::get<static_cast<size_t>(acsr_type)>(accessor.mAccessors).accessors;
                                     anlWeakAssert(element_type::size_flags == 0 || childs.size() == accessors.size());

                                     if constexpr(element_type::size_flags == 0)
                                     {
                                         anlWeakAssert(accessors.empty());
                                         for(auto index = 0_z; index < childs.size(); ++index)
                                         {
                                             if(static_cast<parent_t*>(&accessor)->template insertAcsr<acsr_type>(index, NotificationType::synchronous))
                                             {
                                                 anlWeakAssert(accessors[index] != nullptr && childs[index] != nullptr);
                                                 if(accessors[index] != nullptr && childs[index] != nullptr)
                                                 {
                                                     accessors[index]->fromXml(*childs[index], enumname.c_str(), NotificationType::synchronous);
                                                 }
                                             }
                                             else
                                             {
                                                 accessors.push_back(nullptr);
                                                 anlWeakAssert(false && "allocation failed");
                                             }
                                         }
                                     }
                                     else
                                     {
                                         anlWeakAssert(childs.size() == accessors.size());
                                         for(size_t index = 0; index < std::min(accessors.size(), childs.size()); ++index)
                                         {
                                             anlWeakAssert(accessors[index] != nullptr && childs[index] != nullptr);
                                             if(accessors[index] != nullptr && childs[index] != nullptr)
                                             {
                                                 accessors[index]->fromXml(*childs[index], enumname.c_str(), NotificationType::synchronous);
                                             }
                                         }
                                     }
                                 }
                             });
        }

        template <acsr_enum_type type>
        void notifyAccessorInsertion(size_t index, NotificationType const notification)
        {
            auto& lock = getLock();
            auto const canAccess = lock.exchange(false);
            anlStrongAssert(canAccess == true);
            if(!canAccess)
            {
                return;
            }

            if(onAccessorInserted != nullptr)
            {
                lock.store(true);
                onAccessorInserted(type, index, notification);
                lock.store(false);
            }

            using element_type = typename std::tuple_element<static_cast<size_t>(type), acsr_container_type>::type;
            if constexpr((element_type::flags & Flag::notifying) != 0)
            {
                mListeners.notify([index, this](Listener& listener)
                                  {
                                      if(listener.onAccessorInserted != nullptr)
                                      {
                                          listener.onAccessorInserted(*static_cast<parent_t const*>(this), type, index);
                                      }
                                  },
                                  notification);
            }

            lock = true;
        }

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
            template <typename T, typename F, size_t... Is>
            static void for_each(T&& t, F f, std::integer_sequence<size_t, Is...>)
            {
                auto l = {(f(std::get<Is>(t)), 0)...};
                juce::ignoreUnused(l);
            }

            template <typename... Ts, typename F>
            static void for_each(std::tuple<Ts...> const& t, F f)
            {
                detail::for_each(t, f, std::make_integer_sequence<size_t, sizeof...(Ts)>());
            }

            template <typename... Ts, typename F>
            static void for_each_inv(std::tuple<Ts...> const& t, F f)
            {
                detail::for_each(t, f, make_index_sequence_reverse<sizeof...(Ts)>());
            }

            template <typename... Ts, typename F>
            static void for_each(std::tuple<Ts...>& t, F f)
            {
                detail::for_each(t, f, std::make_integer_sequence<size_t, sizeof...(Ts)>());
            }

            template <typename... Ts, typename F>
            static void for_each_inv(std::tuple<Ts...>& t, F f)
            {
                detail::for_each(t, f, make_index_sequence_reverse<sizeof...(Ts)>());
            }
        };

        // This is a specific compare method that manage containers and unique pointer
        template <typename T>
        static bool isEquivalentTo(T const& lhs, T const& rhs)
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
            else if constexpr(is_specialization<T, std::reference_wrapper>::value)
            {
                return std::addressof(lhs) == std::addressof(rhs);
            }
            else if constexpr(is_specialization<T, std::optional>::value)
            {
                return lhs.has_value() == rhs.has_value() && (!lhs.has_value() || isEquivalentTo(*lhs, *rhs));
            }
            else
            {
                return lhs == rhs;
            }
        }

        attr_container_type mAttributes;
        acsr_container_type mAccessors;

        Notifier<Listener> mListeners;

        std::atomic<bool> mLock{true};
        std::atomic<bool>* mSharedLock = nullptr;

        template <class, class, class>
        friend class Accessor;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Accessor)
    };
} // namespace Model

ANALYSE_FILE_END
