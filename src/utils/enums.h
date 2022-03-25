#ifndef __ENUMS_H__
#define __ENUMS_H__

#include <boost/preprocessor.hpp>

#include <algorithm>
#include <string>
#include <ranges>
#include <limits>
#include <bit>

#include "type_list.h"

namespace enums {
    namespace detail {
        template<size_t S, typename ... Ts> struct sized_int {};
        template<size_t S, typename ... Ts> using sized_int_t = typename sized_int<S, Ts ...>::type;

        template<size_t S, typename T> constexpr bool fits_in_v = S <= std::numeric_limits<T>::max();

        template<size_t S, typename T> struct sized_int<S, T>
            : std::enable_if<fits_in_v<S, T>, T> {};
        template<size_t S, typename First, typename ... Ts> struct sized_int<S, First, Ts...>
            : std::conditional<fits_in_v<S, First>, First, sized_int_t<S, Ts...>> {};
    }

    template<size_t S> using sized_int_t = detail::sized_int_t<S, uint8_t, uint16_t, uint32_t, uint64_t>;

    template<typename T> concept enumeral = std::is_enum_v<T>;

    template<enumeral auto E> struct enum_tag_t { static constexpr auto value = E; };
    template<enumeral auto E> constexpr enum_tag_t<E> enum_tag;

    template<typename T> concept reflected_enum = requires (T value) {
        requires enumeral<T>;
        get_enum_reflector_type(value);
    };

    template<reflected_enum T> using reflector = decltype(get_enum_reflector_type(T{}));

    template<reflected_enum T> constexpr std::string_view enum_name_v = reflector<T>::enum_name;

    template<reflected_enum T> constexpr auto enum_values_v = reflector<T>::values;

    template<reflected_enum T> constexpr auto size_v = enum_values_v<T>.size();

    template<reflected_enum auto ... Values> using enum_sequence = util::type_list<enum_tag_t<Values>...>;
    namespace detail {
        template<reflected_enum T, typename ISeq> struct make_enum_sequence{};
        template<reflected_enum T, size_t ... Is> struct make_enum_sequence<T, std::index_sequence<Is...>> {
            using type = enum_sequence<enum_values_v<T>[Is]...>;
        };

        template<template<reflected_enum auto> typename Filter>
        struct enum_filter_wrapper {
            template<typename EnumConst> struct type{};
            template<reflected_enum auto Value> struct type<enum_tag_t<Value>> : Filter<Value> {};
        };
    }

    template<reflected_enum T> using make_enum_sequence = typename detail::make_enum_sequence<T, std::make_index_sequence<size_v<T>>>::type;

    template<template<reflected_enum auto> typename Filter, typename ESeq>
    using filter_enum_sequence = util::type_list_filter_t<
        detail::enum_filter_wrapper<Filter>::template type, ESeq>;

    template<reflected_enum T> constexpr bool is_flags_enum() {
        size_t i = 1;
        for (auto value : enum_values_v<T>) {
            if (value != static_cast<T>(i)) return false;
            i <<= 1;
        }
        return true;
    }
    template<typename T> concept flags_enum = is_flags_enum<T>();

    template<reflected_enum T> constexpr bool is_linear_enum() {
        size_t i = 0;
        for (auto value : enum_values_v<T>) {
            if (value != static_cast<T>(i)) return false;
            ++i;
        }
        return true;
    }
    template<typename T> concept linear_enum = is_linear_enum<T>();

    template<reflected_enum T> constexpr size_t indexof(T value) {
        if constexpr (linear_enum<T>) {
            return static_cast<size_t>(value);
        } else if constexpr (flags_enum<T>) {
            return std::countr_zero(static_cast<size_t>(value));
        } else if constexpr (std::ranges::is_sorted(enum_values_v<T>)) {
            return std::ranges::lower_bound(enum_values_v<T>, value) - enum_values_v<T>.begin();
        } else {
            return std::ranges::find(enum_values_v<T>, value) - enum_values_v<T>.begin();
        }
    }

    template<reflected_enum auto E> concept value_with_data = requires {
        requires reflected_enum<decltype(E)>;
        reflector<decltype(E)>::get_data(enum_tag<E>);
    };

    template<reflected_enum auto E> requires value_with_data<E>
    constexpr auto enum_data_v = reflector<decltype(E)>::get_data(enum_tag<E>);

    template<reflected_enum auto E> requires value_with_data<E>
    using enum_data_t = decltype(enum_data_v<E>);

    template<reflected_enum Enum> auto get_data(Enum value) {
        static constexpr auto data_array = []<Enum ... Es>(enum_sequence<Es...>) {
            return std::array{ enum_data_v<Es> ... };
        }(make_enum_sequence<Enum>());
        return data_array[indexof(value)];
    }

    template<reflected_enum auto E> concept value_with_type = requires {
        requires reflected_enum<decltype(E)>;
        reflector<decltype(E)>::get_type(enum_tag<E>);
    };

    template<reflected_enum auto E> requires value_with_type<E>
    using enum_type_t = decltype(reflector<decltype(E)>::get_type(enum_tag<E>));

    template<reflected_enum T> struct invalid_enum {
        static constexpr T value = static_cast<T>(std::numeric_limits<std::underlying_type_t<T>>::max());
    };

    template<reflected_enum T> requires flags_enum<T> struct invalid_enum<T> {
        static constexpr T value = static_cast<T>(0);
    };

    template<reflected_enum T> constexpr T invalid_enum_v = invalid_enum<T>::value;

    template<typename T> concept enum_with_names = requires {
        requires reflected_enum<T>;
        reflector<T>::names;
        requires reflector<T>::names.size() == size_v<T>;
        requires std::convertible_to<typename decltype(reflector<T>::names)::value_type, std::string_view>;
    };

    template<enum_with_names T> constexpr auto enum_names_v = reflector<T>::names;

    template<enum_with_names T>
    constexpr std::string_view to_string(T value) {
        return enum_names_v<T>[indexof(value)];
    }

    template<enum_with_names T> constexpr T from_string(std::string_view str) {
        if (auto it = std::ranges::find(enum_names_v<T>, str); it != enum_names_v<T>.end()) {
            return enum_values_v<T>[it - enum_names_v<T>.begin()];
        } else {
            return invalid_enum_v<T>;
        }
    }

    template<typename T> concept enum_with_full_names = requires {
        requires reflected_enum<T>;
        reflector<T>::full_names;
        requires reflector<T>::full_names.size() == size_v<T>;
        requires std::convertible_to<typename decltype(reflector<T>::full_names)::value_type, std::string_view>;
    };

    template<enum_with_full_names T> constexpr auto enum_full_names_v = reflector<T>::full_names;
    
    template<enum_with_full_names T>
    constexpr std::string_view full_name(T value) {
        return enum_full_names_v<T>[indexof(value)];
    }

    constexpr auto to_underlying(reflected_enum auto value) {
        return static_cast<std::underlying_type_t<decltype(value)>>(value);
    }

    inline namespace flag_operators {
        template<flags_enum T> constexpr T operator & (T lhs, T rhs) {
            return static_cast<T>(to_underlying(lhs) & to_underlying(rhs));
        }

        template<flags_enum T> constexpr T &operator &= (T &lhs, T rhs) {
            return lhs = lhs & rhs;
        }

        template<flags_enum T> constexpr T operator | (T lhs, T rhs) {
            return static_cast<T>(to_underlying(lhs) | to_underlying(rhs));
        }

        template<flags_enum T> constexpr T &operator |= (T &lhs, T rhs) {
            return lhs = lhs | rhs;
        }

        template<flags_enum T> constexpr T operator ~ (T value) {
            return static_cast<T>(~to_underlying(value));
        }

        template<flags_enum T> constexpr T operator ^ (T lhs, T rhs) {
            return static_cast<T>(to_underlying(lhs) ^ to_underlying(rhs));
        }

        template<flags_enum T> constexpr T &operator ^= (T &lhs, T rhs) {
            return lhs = lhs ^ rhs;
        }
    }

    template<flags_enum T> constexpr T flags_none = static_cast<T>(0);

    template<flags_enum T> constexpr T flags_all = []<T ... values>(enum_sequence<values...>) {
        using namespace flag_operators;
        return (values | ...);
    }(make_enum_sequence<T>());

    template<enum_with_names T> requires flags_enum<T>
    constexpr T flags_from_string(std::string_view str) {
        constexpr std::string_view whitespace = " \t";
        T ret = flags_none<T>;
        while (true) {
            size_t pos = str.find_first_not_of(whitespace);
            if (pos == std::string_view::npos) break;
            str = str.substr(pos);
            pos = str.find_first_of(whitespace);
            ret |= from_string<T>(str.substr(0, pos));
            if (pos == std::string_view::npos) break;
            str = str.substr(pos);
        }
        return ret;
    }

    template<enum_with_names T> requires flags_enum<T>
    constexpr std::string flags_to_string(T value) {
        std::string ret;
        for (T v : enum_values_v<T>) {
            if (bool(value & v)) {
                if (!ret.empty()) {
                    ret += ' ';
                }
                ret.append(to_string(v));
            }
        }
        return ret;
    }
    
    template<typename RetType, typename Function, reflected_enum T> RetType visit_enum(Function &&fun, T value) {
        static constexpr auto lut = []<T ... Values>(enum_sequence<Values...>) {
            return std::array{ +[](Function &&fun) -> RetType {
                return fun(enum_tag<Values>);
            } ... };
        }(make_enum_sequence<T>());
        return lut[indexof(value)](std::forward<Function>(fun));
    }

    template<typename Function, reflected_enum T> decltype(auto) visit_enum(Function &&fun, T value) {
        using result_type = std::invoke_result_t<Function, enums::enum_tag_t<enums::enum_values_v<T>[0]>>;
        return visit_enum<result_type>(fun, value);
    }

    inline namespace stream_operators {
        template<typename Stream, enum_with_names T>
        Stream &operator << (Stream &out, const T &value) {
            return out << to_string(value);
        }

        template<typename Stream, enum_with_names T> requires flags_enum<T>
        Stream &operator << (Stream &out, const T &flags) {
            bool first = true;
            for (auto value : enum_values_v<T>) {
                if (bool(flags & value)) {
                    if (first) {
                        first = false;
                    } else {
                        out << ' ';
                    }
                    out << to_string(value);
                }
            }
            return out;
        }
    }

    template<enums::flags_enum E>
    auto enum_flag_values(E value) {
        return enums::enum_values_v<E> | std::views::filter([=](E item) {
            using namespace flag_operators;
            return bool(item & value);
        });
    }
    
    template<flags_enum T>
    class bitset {
    private:
        T m_value;

    public:
        constexpr bitset(T value = static_cast<T>(0)) : m_value(value) {}

        constexpr bool empty() const { return to_underlying(m_value) == 0; }
        constexpr bool check(T value) const { return bool(m_value & value); }
        constexpr void set(T value) { m_value |= value; }
        constexpr void unset(T value) { m_value &= ~value; }
        constexpr void toggle(T value) { m_value ^= value; }
        constexpr void clear() { m_value = static_cast<T>(0); }

        constexpr T data() const { return m_value; }

        template<typename Stream> friend
        Stream &operator << (Stream &out, const bitset &value) {
            return out << value.data();
        }
    };
}

#define DO_NOTHING(...)

#define HELPER1(...) ((__VA_ARGS__)) HELPER2
#define HELPER2(...) ((__VA_ARGS__)) HELPER1
#define HELPER1_END
#define HELPER2_END
#define ADD_PARENTHESES(sequence) BOOST_PP_CAT(HELPER1 sequence,_END)

#define GET_FIRST_OF(name, ...) name
#define GET_TAIL_OF(name, ...) __VA_ARGS__

#define ENUM_ELEMENT_NAME(tuple) GET_FIRST_OF tuple
#define ENUM_TUPLE_TAIL(tuple) GET_TAIL_OF tuple

#define CREATE_ENUM_ELEMENT(r, enumName, i, elementTuple) (ENUM_ELEMENT_NAME(elementTuple) = i)
#define CREATE_FLAG_ELEMENT(r, enumName, i, elementTuple) (ENUM_ELEMENT_NAME(elementTuple) = 1 << i)

#define CREATE_ENUM_VALUES_ELEMENT(r, enumName, elementTuple) (enumName::ENUM_ELEMENT_NAME(elementTuple))
#define CREATE_ENUM_NAMES_ELEMENT(r, enumName, elementTuple) (BOOST_PP_STRINGIZE(ENUM_ELEMENT_NAME(elementTuple)))
#define CREATE_ENUM_FULL_NAMES_ELEMENT(r, enumName, elementTuple) (BOOST_PP_STRINGIZE(enumName) "::" BOOST_PP_STRINGIZE(ENUM_ELEMENT_NAME(elementTuple)))

#define CREATE_ENUM_ELEMENT_MAX_VALUE(n) n - 1
#define CREATE_FLAG_ELEMENT_MAX_VALUE(n) 1 << (n - 1)

#define ENUM_INT(enum_value_fun, elementTupleSeq) enums::sized_int_t<enum_value_fun##_MAX_VALUE(BOOST_PP_SEQ_SIZE(elementTupleSeq))>

#define ENUM_DATA_FUNCTIONS(enumName, elementTuple) \
    static constexpr auto get_data(enums::enum_tag_t<enumName::ENUM_ELEMENT_NAME(elementTuple)>) { \
        return ENUM_TUPLE_TAIL(elementTuple); \
    }

#define ENUM_TYPE_FUNCTIONS(enumName, elementTuple) \
    static constexpr ENUM_TUPLE_TAIL(elementTuple) get_type(enums::enum_tag_t<enumName::ENUM_ELEMENT_NAME(elementTuple)>);

#define GENERATE_ENUM_CASE(r, enumNameFunTuple, elementTuple) \
    BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(elementTuple), 1), \
        DO_NOTHING, BOOST_PP_TUPLE_ELEM(1, enumNameFunTuple)) \
        (BOOST_PP_TUPLE_ELEM(0, enumNameFunTuple), elementTuple)

#define REFLECTOR_NAME(enumName) __##enumName##_reflector

#define IMPL_DEFINE_ENUM(enumName, elementTupleSeq, enum_value_fun, value_fun_name) \
enum class enumName : ENUM_INT(enum_value_fun, elementTupleSeq) { \
    BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH_I(enum_value_fun, enumName, elementTupleSeq)) \
}; \
struct REFLECTOR_NAME(enumName) { \
    static constexpr std::string_view enum_name = BOOST_PP_STRINGIZE(enumName); \
    static constexpr std::array values { \
        BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(CREATE_ENUM_VALUES_ELEMENT, enumName, elementTupleSeq)) \
    }; \
    static constexpr std::array<std::string_view, values.size()> full_names { \
        BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(CREATE_ENUM_FULL_NAMES_ELEMENT, enumName, elementTupleSeq)) \
    }; \
    static constexpr std::array<std::string_view, values.size()> names { \
        BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(CREATE_ENUM_NAMES_ELEMENT, enumName, elementTupleSeq)) \
    }; \
    BOOST_PP_SEQ_FOR_EACH(GENERATE_ENUM_CASE, (enumName, value_fun_name), elementTupleSeq) \
}; \
REFLECTOR_NAME(enumName) get_enum_reflector_type(enumName);

#define DO_FWD_DECLARE_HELPER(elementTuple) struct ENUM_TUPLE_TAIL(elementTuple);
#define DO_FWD_DECLARE(r, _, elementTuple) \
    BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(elementTuple), 1), \
        DO_NOTHING, DO_FWD_DECLARE_HELPER)(elementTuple)

#define IMPL_FWD_DECLARE(elementTupleSeq) BOOST_PP_SEQ_FOR_EACH(DO_FWD_DECLARE, _, elementTupleSeq)

#define DEFINE_ENUM(enumName, enumElements) \
    IMPL_DEFINE_ENUM(enumName, ADD_PARENTHESES(enumElements), CREATE_ENUM_ELEMENT, DO_NOTHING)
#define DEFINE_ENUM_FLAGS(enumName, enumElements) \
    IMPL_DEFINE_ENUM(enumName, ADD_PARENTHESES(enumElements), CREATE_FLAG_ELEMENT, DO_NOTHING)

#define DEFINE_ENUM_DATA(enumName, enumElements) \
    IMPL_DEFINE_ENUM(enumName, ADD_PARENTHESES(enumElements), CREATE_ENUM_ELEMENT, ENUM_DATA_FUNCTIONS)
#define DEFINE_ENUM_FLAGS_DATA(enumName, enumElements) \
    IMPL_DEFINE_ENUM(enumName, ADD_PARENTHESES(enumElements), CREATE_FLAG_ELEMENT, ENUM_DATA_FUNCTIONS)

#define DEFINE_ENUM_TYPES(enumName, enumElements) \
    IMPL_DEFINE_ENUM(enumName, ADD_PARENTHESES(enumElements), CREATE_ENUM_ELEMENT, ENUM_TYPE_FUNCTIONS)

#define DEFINE_ENUM_FWD_TYPES(enumName, enumElements) \
    IMPL_FWD_DECLARE(ADD_PARENTHESES(enumElements)) DEFINE_ENUM_TYPES(enumName, enumElements)

#endif