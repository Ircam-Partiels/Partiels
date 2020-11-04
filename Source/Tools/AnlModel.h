#pragma once

#include "AnlListenerList.h"
#include "AnlStringParser.h"

ANALYSE_FILE_BEGIN

template <char... chars> using tstring = std::integer_sequence<char, chars...>;
template <typename T, T... chars> constexpr tstring<chars...> operator "" _tstr() { return { }; }

// https://stackoverflow.com/questions/13065166/c11-tagged-tuple
template<typename... Ts> struct typelist {template<typename T> using prepend = typelist<T, Ts...>;};

template<typename T, typename... Ts> struct index;
template<typename T, typename... Ts> struct index<T, T, Ts...>: std::integral_constant<int, 0> {};
template<typename T, typename U, typename... Ts> struct index<T, U, Ts...>: std::integral_constant<int, index<T, Ts...>::value+1> {};

template<int n, typename... Ts> struct nth_impl;
template<typename T, typename... Ts> struct nth_impl<0, T, Ts...> { using type = T; };
template<int n, typename T, typename... Ts> struct nth_impl<n, T, Ts...> { using type = typename nth_impl<n - 1, Ts...>::type; };
template<int n, typename... Ts> using nth = typename nth_impl<n, Ts...>::type;

template<int n, int m, typename... Ts> struct extract_impl;
template<int n, int m, typename T, typename... Ts> struct extract_impl<n, m, T, Ts...>: extract_impl<n, m - 1, Ts...> {};
template<int n, typename T, typename... Ts> struct extract_impl<n, 0, T, Ts...> { using types = typename extract_impl<n, n - 1, Ts...>::types::template prepend<T>; };
template<int n, int m> struct extract_impl<n, m> { using types = typelist<>; };
template<int n, int m, typename... Ts> using extract = typename
extract_impl<n, m, Ts...>::types;

template<typename S, typename T> struct tt_impl;
template<typename... Ss, typename... Ts>
struct tt_impl<typelist<Ss...>, typelist<Ts...>>:
public std::tuple<Ts...> {
    template<typename... Args> tt_impl(Args &&...args):
    std::tuple<Ts...>(std::forward<Args>(args)...) {}
    template<typename S> nth<index<S, Ss...>::value, Ts...> get() {
        return std::get<index<S, Ss...>::value>(*this); }
};
template<typename... Ts> struct tagged_tuple:
tt_impl<extract<2, 0, Ts...>, extract<2, 1, Ts...>> {
    template<typename... Args> tagged_tuple(Args &&...args):
    tt_impl<extract<2, 0, Ts...>, extract<2, 1, Ts...>>(std::forward<Args>(args)...) {}
};

namespace Model
{
    namespace detail
    {
        // https://stackoverflow.com/questions/16387354/template-tuple-calling-a-function-on-each-element
        template<int... Is> struct seq {};
        template<int N, int... Is> struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};
        template<int... Is> struct gen_seq<0, Is...> : seq<Is...> {};
        template<typename T, typename F, int... Is> void for_each(T&& t, F f, seq<Is...>) {auto l = { (f(std::get<Is>(t)), 0)... }; juce::ignoreUnused(l);}
        template<typename... Ts, typename F> void for_each(std::tuple<Ts...> const& t, F f) {for_each(t, f, gen_seq<sizeof...(Ts)>());}
        template<typename... Ts, typename F> void for_each(std::tuple<Ts...>& t, F f) {for_each(t, f, gen_seq<sizeof...(Ts)>());}
    }

    template<typename , bool, typename> struct TypedData;
    template<typename T, bool _saveable, char... _name> struct TypedData<T, _saveable, tstring<_name...>>
    {
        static bool const saveable = _saveable;
        static constexpr char name[sizeof...(_name) + 1] = { _name..., '\0' };
        T value;
    };
    
    template<typename T, bool _saveable, size_t _index> struct Data
    {
        static bool const saveable = _saveable;
        static bool const index = _index;
        T value;
    };
    
    template<class data_t> class Accessor
    {
    private:
        // https://stackoverflow.com/questions/13101061/detect-if-a-type-is-a-stdtuple
        template <typename> struct is_tuple: std::false_type {};
        template <typename ...T> struct is_tuple<std::tuple<T...>>: std::true_type {};
        
    public:
        
        static_assert(is_tuple<data_t>{}, "data_t must be a specialization of std::tuple");
        
        Accessor(data_t&& data = {})
        : mData(std::move(data))
        {
        }
        
        ~Accessor() = default;
    
        inline operator data_t() const { return mData; }
        
        std::unique_ptr<juce::XmlElement> toXml(juce::StringRef const& name) const
        {
            auto xml = std::make_unique<juce::XmlElement>(name);
            anlWeakAssert(xml != nullptr);
            if(xml == nullptr)
            {
                return nullptr;
            }
            
            detail::for_each(mData, [&](auto const& d)
            {
                using attr_type_t = typename std::remove_reference<decltype(d)>::type;
                if constexpr(attr_type_t::saveable)
                {
                    xml->setAttribute(attr_type_t::name, d.value);
                }
            });
            return xml;
        }
        
        static data_t fromXml(juce::XmlElement const& xml, juce::StringRef const& name, data_t model = {})
        {
            anlWeakAssert(xml.hasTagName(name));
            if(!xml.hasTagName(name))
            {
                return {};
            }
        
            detail::for_each(model, [&](auto& d)
            {
                using attr_type_t = typename std::remove_reference<decltype(d)>::type;
                if constexpr(attr_type_t::saveable)
                {
                    anlWeakAssert(xml.hasAttribute(attr_type_t::name));
                    std::stringstream ss(xml.getStringAttribute(attr_type_t::name, juce::String(d.value)).toRawUTF8());
                    ss >> d.value;
                }
            });
        
            return std::move(model);
        }
        
        void fromXml(juce::XmlElement const& xml, Accessor model, juce::NotificationType const notification)
        {
            fromModel(fromXml(xml, std::forward(model)), notification);
        }
        
        class Listener
        {
        public:
            Listener() = default;
            virtual ~Listener() = default;
            
            std::function<void(Accessor const&, size_t)> onChanged = nullptr;
        };
        
    private:
        data_t mData;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Accessor)
    };
}

ANALYSE_FILE_END
