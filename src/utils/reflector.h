#ifndef __REFLECTOR_H__
#define __REFLECTOR_H__

#include <boost/preprocessor.hpp>
#include <utility>
#include <cstddef>

#include "type_list.h"

#define REM(...) __VA_ARGS__
#define EAT(...)

// Retrieve the type
#define TYPEOF(x) DETAIL_TYPEOF(DETAIL_TYPEOF_PROBE x,)
#define DETAIL_TYPEOF(...) DETAIL_TYPEOF_HEAD(__VA_ARGS__)
#define DETAIL_TYPEOF_HEAD(x, ...) REM x
#define DETAIL_TYPEOF_PROBE(...) (__VA_ARGS__),
// Strip off the type
#define STRIP(x) EAT x
// Show the type without parenthesis
#define PAIR(x) REM x

#define REFLECTABLE(...) \
struct reflector_num_fields { \
    static constexpr size_t value = BOOST_PP_VARIADIC_SIZE(__VA_ARGS__); \
}; \
template<size_t N, class Self> \
struct reflector_field_data {}; \
BOOST_PP_SEQ_FOR_EACH_I(REFLECT_EACH, data, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define REFLECT_EACH(r, data, i, x) \
PAIR(x){}; \
template<class Self> struct reflector_field_data<i, Self> { \
    static constexpr size_t index = i; \
    using self_type = Self; \
    Self &self; \
    constexpr reflector_field_data(Self &self) : self(self) {} \
    \
    TYPEOF(x) &get() { \
        return self.STRIP(x); \
    } \
    std::add_const_t<TYPEOF(x)> &get() const { \
        return self.STRIP(x); \
    } \
    const char *name() const { \
        return BOOST_PP_STRINGIZE(STRIP(x)); \
    } \
};

namespace reflector {
    namespace detail {
        template<typename T>
        concept is_field_data = requires(T &t) {
            t.get();
            t.name();
        };

        template<typename T, size_t I>
        concept has_field_data = requires(T &t) {
            typename T::template reflector_field_data<I, T>;
            { typename T::template reflector_field_data<I, T>(t) } -> is_field_data;
        };
    }

    template<typename T>
    concept reflectable = requires {
        typename T::reflector_num_fields;
        requires []<size_t ... Is>(std::index_sequence<Is...>) {
            return (detail::has_field_data<T, Is> && ...);
        }(std::make_index_sequence<T::reflector_num_fields::value>());
    };

    template<reflectable ... Ts> struct reflectable_base : Ts ... {
        using reflector_base_types = util::type_list<Ts ...>;
    };

    namespace detail {
        template<typename T, typename TList> struct is_derived_from_all {};

        template<typename T, typename ... Ts>
        struct is_derived_from_all<T, util::type_list<Ts ...>> {
            static constexpr bool value = (std::is_base_of_v<Ts, T> && ...);
        };

        template<typename T, typename TList> concept derived_from_all = is_derived_from_all<T, TList>::value;

        template<typename T> concept has_reflectable_base = requires {
            typename T::reflector_base_types;
            requires derived_from_all<T, typename T::reflector_base_types>;
        };

        template<reflectable T> struct num_fields {
            static constexpr size_t value = T::reflector_num_fields::value;
        };

        template<typename TList> struct sum_num_fields{};
        template<typename ... Ts> struct sum_num_fields<util::type_list<Ts ...>> {
            static constexpr size_t value = (num_fields<Ts>::value + ...);
        };

        template<reflectable T> requires has_reflectable_base<T>
        struct num_fields<T> {
            static constexpr size_t value = T::reflector_num_fields::value +
                sum_num_fields<typename T::reflector_base_types>::value;
        };

        template<size_t I, typename Derived, typename TList> struct impl_field_data {};
        template<size_t I, typename Derived> struct impl_field_data<I, Derived, util::type_list<>> {
            using type = typename Derived::template reflector_field_data<I, Derived>;
        };

        template<size_t I, typename Derived, typename First, typename ... Ts>
        requires (I < num_fields<First>::value)
        struct impl_field_data<I, Derived, util::type_list<First, Ts...>> {
            using type = typename First::template reflector_field_data<I, First>;
        };

        template<size_t I, typename Derived, typename First, typename ... Ts>
        requires (I >= num_fields<First>::value)
        struct impl_field_data<I, Derived, util::type_list<First, Ts...>>
            : impl_field_data<I - num_fields<First>::value, Derived, util::type_list<Ts ...>> {};

        template<size_t I, reflectable T> struct field_data : impl_field_data<I, T, util::type_list<>> {};
        template<size_t I, reflectable T> requires has_reflectable_base<T>
        struct field_data<I, T> : impl_field_data<I, T, typename T::reflector_base_types> {};
    }

    template<reflectable T> constexpr size_t num_fields = detail::num_fields<T>::value;

    template<size_t I, reflectable T>
    constexpr auto get_field_data(T &x) {
        return typename detail::field_data<I, T>::type(x);
    }

    template<size_t I, reflectable T>
    constexpr auto get_field_data(const T &x) {
        using field_data = typename detail::field_data<I, T>::type;
        using self_type = typename field_data::self_type;
        return typename self_type::reflector_field_data<field_data::index, const self_type>(x);
    }
}


#endif