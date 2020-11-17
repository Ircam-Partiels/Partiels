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
        model = 1 << 3 // private
    };
    
    //! @brief The private implementation of an attribute of a model
    template<typename enum_t, enum_t index_v, typename value_t, int flags_v>
    struct AttrImpl
    {
        static_assert(std::is_enum<enum_t>::value, "enum_t must be an enum");
        static_assert(std::is_same<std::underlying_type_t<enum_t>, size_t>::value, "enum_t underlying type must be size_t");
        static_assert(std::is_default_constructible<value_t>::value, "value_t must be default constructible");
        static_assert(std::is_copy_constructible<value_t>::value, "value_t must be copy constructible");
        
        using enum_type = enum_t;
        using value_type = value_t;
        
        static enum_type const type = static_cast<enum_type>(index_v);
        static int const flags = flags_v;
        value_type value;
    };
    
    //! @brief The private implementation of sub model
    template<typename enum_t, enum_t index_v, typename model_t, typename accessor_t, int flags_v, size_t size_flags_v>
    struct SubModelImp
    {
        static_assert(std::is_enum<enum_t>::value, "enum_t must be an enum");
        static_assert(std::is_same<std::underlying_type_t<enum_t>, size_t>::value, "enum_t underlying type must be size_t");
        
        using enum_type = enum_t;
        using model_type = model_t;
        using accessor_type = accessor_t;
        
        struct container_type
        {
        private:
            model_type model;
            
        public:
            accessor_type accessor {model};
            container_type() = default;
            container_type(model_type const& m)
            : model(m)
            {
            }
            
            container_type(model_type&& m)
            : model(std::move(m))
            {
            }
        };
        
        static enum_type const type = static_cast<enum_type>(index_v);
        static int const flags = flags_v | model;
        static size_t const size_flags = size_flags_v;
        std::vector<std::unique_ptr<container_type>> containers;
        
        SubModelImp()
        {
            containers.reserve(size_flags);
            for(size_t i = 0; i < size_flags; ++i)
            {
                containers.push_back(std::make_unique<container_type>());
            }
        }
        
        SubModelImp(SubModelImp const& other)
        {
            auto const& ctnrs = other.containers;
            anlStrongAssert(size_flags == 0 || ctnrs.size() == size_flags);
            auto const numContainers = size_flags > 0 ? size_flags : ctnrs.size();
            for(size_t i = 0; i < numContainers; ++i)
            {
                anlStrongAssert(i < ctnrs.size() && ctnrs[i] != nullptr);
                if(i < ctnrs.size() && ctnrs[i] != nullptr)
                {
                    containers.push_back(std::make_unique<container_type>(ctnrs[i]->accessor.getModel()));
                }
                else
                {
                    containers.push_back(std::make_unique<container_type>());
                }
            }
        }
        
        SubModelImp(SubModelImp&& other) : containers(std::move(other.containers))
        {
            anlStrongAssert(size_flags == 0 || containers.size() == size_flags);
        }
        
        SubModelImp(std::initializer_list<model_type> models)
        {
            containers.reserve(models.size());
            for(auto model : models)
            {
                containers.push_back(std::make_unique<container_type>(model));
            }
            anlStrongAssert(size_flags == 0 || containers.size() == size_flags);
        }
    };

    //! @brief The template implementation of an attribute of a model
    template<auto index_v, typename value_t, int flags_v>
    using Attr = AttrImpl<decltype(index_v), index_v, value_t, flags_v>;
    
    //! @brief The template implementation of an attribute of a model
    template<auto index_v, typename model_t, typename accessor_t, int flags_v, size_t size_v>
    using Model = SubModelImp<decltype(index_v), index_v, model_t, accessor_t, flags_v, size_v>;
    
    //! @brief The container type for a set of attributes
    template <class ..._Tp>
    using Container = std::tuple<_Tp...>;
    
    //! @brief The accessor a data model
    template<class parent_t, class container_t>
    class Accessor
    {
    public:
        using container_type = container_t;
        using enum_type = typename std::tuple_element<0, container_type>::type::enum_type;
        
        static_assert(std::is_same<typename std::underlying_type<enum_type>::type, size_t>::value, "enum_t underlying type must be size_t");
        static_assert(is_specialization<container_type, std::tuple>::value, "container_t must be a specialization of std::tuple");
        
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
        
        //! @brief Gets all the accessors of an attriute
        template <enum_type type>
        auto getAccessors() noexcept
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::model)!= 0, "element is not a model");
            
            using accessor_type = typename element_type::accessor_type;
            auto& containers = std::get<static_cast<size_t>(type)>(mData).containers;
            std::vector<std::reference_wrapper<accessor_type>> acrs;
            acrs.reserve(containers.size());
            for(auto& container : containers)
            {
                anlStrongAssert(container != nullptr);
                if(container != nullptr)
                {
                    acrs.push_back(container->accessor);
                }
            }
            return acrs;
        }
        
        template <enum_type type>
        auto getAccessors() const noexcept
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::model)!= 0, "element is not a model");
            
            using accessor_type = typename element_type::accessor_type;
            auto const& containers = std::get<static_cast<size_t>(type)>(mData).containers;
            std::vector<std::reference_wrapper<accessor_type const>> acrs;
            acrs.reserve(containers.size());
            for(auto const& container : containers)
            {
                anlStrongAssert(container != nullptr);
                if(container != nullptr)
                {
                    acrs.push_back(container->accessor);
                }
            }
            return acrs;
        }
        
        template <enum_type type>
        auto& getAccessor(size_t index) noexcept
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::model)!= 0, "element is not a model");
            return std::get<static_cast<size_t>(type)>(mData).containers[index]->accessor;
        }
        
        template <enum_type type>
        auto const& getAccessor(size_t index) const noexcept
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::model)!= 0, "element is not a model");
            return std::get<static_cast<size_t>(type)>(mData).containers[index]->accessor;
        }
        
        template <enum_type type>
        void insertModel(long index, typename std::tuple_element<static_cast<size_t>(type), container_type>::type::model_type model, NotificationType notification = NotificationType::synchronous)
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::model)!= 0, "element is not a model");
            
            using submodel_container_type = typename element_type::container_type;
            static_assert(element_type::size_flags == 0, "model is not resizable");
            
            auto container = std::make_unique<submodel_container_type>(std::move(model));
            anlStrongAssert(container != nullptr);
            if(container == nullptr)
            {
                return;
            }
            
            auto& containers = std::get<static_cast<size_t>(type)>(mData).containers;
            auto it = containers.insert(containers.begin() + std::max(index, static_cast<long>(containers.size())), std::move(container));
            if(it != containers.end())
            {
                if constexpr((element_type::flags & AttrFlag::notifying) != 0)
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
        
        template <enum_type type>
        void eraseModel(size_t index, NotificationType notification = NotificationType::synchronous)
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::model)!= 0, "element is not a model");
            
            using submodel_container_type = typename element_type::container_type;
            static_assert(element_type::size_flags == 0, "model is not resizable");
            
            auto& containers = std::get<static_cast<size_t>(type)>(mData).containers;
            auto backup = std::shared_ptr<submodel_container_type>(containers[index].release());
            containers.erase(containers.begin() + static_cast<long>(index));
            if constexpr((element_type::flags & AttrFlag::notifying) != 0)
            {
                mListeners.notify([=](Listener& listener) mutable
                {
                    anlWeakAssert(listener.onChanged != nullptr);
                    anlWeakAssert(backup != nullptr);
                    if(listener.onChanged != nullptr)
                    {
                        listener.onChanged(*static_cast<parent_t const*>(this), type);
                    }
                    backup.reset();
                }, notification);
            }
        }
        
        //! @brief Gets the value of an attribute
        template <enum_type type>
        auto const& getValue() const noexcept
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::model) == 0, "element is a not an attribute");
            return std::get<static_cast<size_t>(type)>(mData).value;
        }
        
        //! @brief Sets the value of an attribute
        //! @details If the value changed and the attribute is marked as notifying, the method notifies the listeners .
        template <enum_type type, typename T>
        void setValue(T const& value, NotificationType notification = NotificationType::synchronous)
        {
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::model) == 0, "element is a not an attribute");
            
            using value_type = typename element_type::value_type;
            if constexpr(std::is_same<value_type, T>::value)
            {
                auto& lvalue = std::get<static_cast<size_t>(type)>(mData).value;
                if(equal(lvalue, value) == false)
                {
                    set<value_type>(lvalue, value);
                    if constexpr((element_type::flags & AttrFlag::notifying) != 0)
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
            using element_type = typename std::tuple_element<static_cast<size_t>(type), container_type>::type;
            static_assert((element_type::flags & AttrFlag::model) == 0, "element is a not an attribute");
            
            typename element_type::value_type const value {tvalue};
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
                    static auto const enumname = std::string(magic_enum::enum_name(attr_type));
                    
                    if constexpr((element_type::flags & AttrFlag::model) != 0)
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
                        std::vector<juce::XmlElement const*> childs;
                        for(auto* child = xml.getChildByName(enumname.c_str()); child != nullptr; child = child->getNextElementWithTagName(enumname.c_str()))
                        {
                            childs.push_back(child);
                        }
                        
                        auto& containers = std::get<static_cast<size_t>(attr_type)>(mData).containers;
                        anlStrongAssert(element_type::size_flags == 0 || childs.size() == containers.size());
                        if constexpr(element_type::size_flags == 0)
                        {
                            while(containers.size() > childs.size())
                            {
                                auto const index = containers.size() - 1;
                                eraseModel<attr_type>(index);
                            }
                        }
                        for(size_t index = 0; index < std::min(containers.size(), childs.size()); ++index)
                        {
                            anlStrongAssert(containers[index] != nullptr);
                            if(containers[index] != nullptr)
                            {
                                containers[index]->accessor.fromXml(*childs[index], enumname.c_str(), notification);
                            }
                        }
                        if constexpr(element_type::size_flags == 0)
                        {
                            while(childs.size() > containers.size())
                            {
                                auto const index = containers.size();
                                insertModel<attr_type>(static_cast<long>(index), {});
                                if(containers[index] != nullptr)
                                {
                                    containers[index]->accessor.fromXml(*childs[index], enumname.c_str(), notification);
                                }
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
                    if constexpr((element_type::flags & AttrFlag::model) != 0)
                    {
                        auto& containers = std::get<static_cast<size_t>(attr_type)>(mData).containers;
                        anlStrongAssert(element_type::size_flags == 0 || d.containers.size() == containers.size());
                        if constexpr(element_type::size_flags == 0)
                        {
                            while(containers.size() > d.containers.size())
                            {
                                auto const index = containers.size() - 1;
                                eraseModel<attr_type>(index);
                            }
                        }
                        for(size_t index = 0; index < std::min(containers.size(), d.containers.size()); ++index)
                        {
                            anlStrongAssert(containers[index] != nullptr && d.containers[index] != nullptr);
                            if(containers[index] != nullptr && d.containers[index] != nullptr)
                            {
                                containers[index]->accessor.fromModel(d.containers[index]->accessor.getModel(), notification);
                            }
                        }
                        if constexpr(element_type::size_flags == 0)
                        {
                            while(d.containers.size() > containers.size())
                            {
                                auto const index = containers.size();
                                anlStrongAssert(d.containers[index] != nullptr);
                                if(d.containers[index] != nullptr)
                                {
                                    insertModel<attr_type>(static_cast<long>(index), d.containers[index]->accessor.getModel());
                                }
                            }
                        }
                    }
                    else
                    {
                        static_cast<parent_t*>(this)->template setValue<attr_type>(d.value, notification);
                    }
                    
                }
            });
        }
        
        //! @brief Compare the content with  another model
        bool isEquivalentTo(container_type const& model) const
        {
            bool result = true;
            detail::for_each(model, [&](auto& d)
            {
                if(result)
                {
                    using element_type = typename std::remove_reference<decltype(d)>::type;
                    auto constexpr attr_type = element_type::type;
                    if constexpr((element_type::flags & AttrFlag::comparable) != 0)
                    {
                        if constexpr((element_type::flags & AttrFlag::model) != 0)
                        {
                            auto const acsrs = getAccessors<attr_type>();
                            result = acsrs.size() != d.containers.size() || std::equal(acsrs.cbegin(), acsrs.cend(), d.containers.cbegin(), [](auto const& acsr, auto const& ctnr)
                            {
                                return ctnr != nullptr && acsr.get().isEquivalentTo(ctnr->accessor.getModel());
                            });
                        }
                        else
                        {
                            result = equal(getValue<attr_type>(), d.value);
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
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Accessor)
    };
}

ANALYSE_FILE_END
