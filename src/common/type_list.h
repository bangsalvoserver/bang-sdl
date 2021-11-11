#ifndef __TYPE_LIST_H__
#define __TYPE_LIST_H__

#include <type_traits>

namespace util {

    template<typename ... Ts> struct type_list {
        static constexpr size_t size = sizeof...(Ts);
    };

    template<size_t N, typename TList> struct get_nth {};
    template<size_t I, typename TList> using get_nth_t = typename get_nth<I, TList>::type;

    template<size_t N, typename First, typename ... Ts> struct get_nth<N, type_list<First, Ts...>> {
        using type = typename get_nth<N-1, type_list<Ts ...>>::type;
    };

    template<typename First, typename ... Ts> struct get_nth<0, type_list<First, Ts ...>> {
        using type = First;
    };

    template<typename T, typename TList> struct type_list_contains{};
    template<typename T, typename TList> constexpr bool type_list_contains_v = type_list_contains<T, TList>::value;

    template<typename T> struct type_list_contains<T, type_list<>> : std::false_type {};

    template<typename T, typename ... Ts> struct type_list_contains<T, type_list<T, Ts...>> : std::true_type {};
    template<typename T, typename First, typename ... Ts>
    struct type_list_contains<T, type_list<First, Ts...>> : type_list_contains<T, type_list<Ts...>> {};

    template<typename T, typename TList> struct type_list_indexof{};
    template<typename T, typename TList> constexpr size_t type_list_indexof_v = type_list_indexof<T, TList>::value;

    template<typename T, typename First, typename ... Ts> struct type_list_indexof<T, type_list<First, Ts...>> {
        static constexpr size_t value = 1 + type_list_indexof_v<T, type_list<Ts ...>>;
    };
    template<typename T, typename ... Ts> struct type_list_indexof<T, type_list<T, Ts...>> {
        static constexpr size_t value = 0;
    };

    template<typename T, typename TList> struct type_list_append{};
    template<typename T, typename TList> using type_list_append_t = typename type_list_append<T, TList>::type;

    template<typename T, typename ... Ts> struct type_list_append<T, type_list<Ts...>> {
        using type = type_list<Ts..., T>;
    };

    template<bool Cond, typename T, typename TList> using type_list_append_if_t =
        std::conditional_t<Cond, type_list_append_t<T, TList>, TList>;

    namespace detail {
        
        template<template<typename> typename Filter, typename TList, typename TFrom> struct type_list_filter{};

        template<template<typename> typename Filter, typename TList>
        struct type_list_filter<Filter, TList, util::type_list<>> {
            using type = TList;
        };

        template<template<typename> typename Filter, typename TList, typename First, typename ... Ts>
        struct type_list_filter<Filter, TList, util::type_list<First, Ts...>> {
            using type = typename type_list_filter<
                Filter,
                util::type_list_append_if_t<
                    Filter<First>::value,
                    First,
                    TList
                >,
                util::type_list<Ts...>
            >::type;
        };
    }

    template<template<typename> typename Filter, typename TList>
    using type_list_filter_t = typename detail::type_list_filter<Filter, type_list<>, TList>::type;

    template<typename TList1, typename TList2> struct type_list_concat{};
    template<typename TList1, typename TList2> using type_list_concat_t = typename type_list_concat<TList1, TList2>::type;
    template<typename ... Ts, typename ... Us> struct type_list_concat<type_list<Ts...>, type_list<Us...>> {
        using type = type_list<Ts..., Us...>;
    };

    template<typename ... Ts> struct type_list_join{};
    template<typename ... Ts> using type_list_join_t = typename type_list_join<Ts ...>::type;
    template<> struct type_list_join<> {
        using type = type_list<>;
    };
    template<typename First, typename ... Ts> struct type_list_join<First, Ts...> {
        using type = type_list_concat_t<First, type_list_join_t<Ts ...>>;
    };
}

#endif