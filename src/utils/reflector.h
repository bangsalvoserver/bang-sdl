#ifndef __REFLECTOR_H__
#define __REFLECTOR_H__

#include <boost/preprocessor.hpp>
#include <utility>
#include <cstddef>

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
static constexpr size_t num_fields = BOOST_PP_VARIADIC_SIZE(__VA_ARGS__); \
template<size_t N, class Self> \
struct field_data {}; \
BOOST_PP_SEQ_FOR_EACH_I(REFLECT_EACH, data, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define REFLECT_EACH(r, data, i, x) \
PAIR(x); \
template<class Self> struct field_data<i, Self> { \
    Self &self; \
    constexpr field_data(Self &self) : self(self) {} \
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
            typename T::template field_data<I, T>;
            { typename T::template field_data<I, T>(t) } -> is_field_data;
        };

        template<typename T, typename ISeq>
        struct has_all_field_datas {};

        template<typename T, size_t ... Is>
        struct has_all_field_datas<T, std::index_sequence<Is...>> {
            static constexpr bool value = (has_field_data<T, Is> && ...);
        };
    }

    template<typename T>
    concept reflectable = requires {
        T::num_fields;
        requires detail::has_all_field_datas<T, std::make_index_sequence<T::num_fields>>::value;
    };

    template<reflectable T> constexpr size_t num_fields = T::num_fields;

    template<size_t I, reflectable T>
    constexpr auto get_field_data(T &x) {
        return typename T::template field_data<I, T>(x);
    }
}


#endif