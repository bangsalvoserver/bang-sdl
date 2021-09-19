#ifndef __ENUMS_H__
#define __ENUMS_H__

#include <boost/preprocessor.hpp>
#include <algorithm>
#include <string>

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

    template<typename T> struct enum_values {};
    template<typename T> constexpr auto enum_values_v = enum_values<T>::value;

    template<typename T> concept reflected_enum = std::is_enum_v<T> && requires {
        enum_values<T>::value;
    };

    template<reflected_enum T> constexpr size_t size_v = enum_values_v<T>.size();

    template<reflected_enum auto Value> struct enum_constant {};
    template<reflected_enum auto Value> constexpr enum_constant<Value> enum_tag;

    template<reflected_enum auto ... Values> using enum_sequence = util::type_list<enum_constant<Values>...>;
    namespace detail {
        template<reflected_enum T, typename ISeq> struct make_enum_sequence{};
        template<reflected_enum T, size_t ... Is> struct make_enum_sequence<T, std::index_sequence<Is...>> {
            using type = enum_sequence<enum_values_v<T>[Is]...>;
        };

        template<template<reflected_enum auto> typename Filter>
        struct enum_filter_wrapper {
            template<typename EnumConst> struct type{};
            template<reflected_enum auto Value> struct type<enum_constant<Value>> : Filter<Value> {};
        };
    }

    template<reflected_enum T> using make_enum_sequence = typename detail::make_enum_sequence<T, std::make_index_sequence<size_v<T>>>::type;

    template<template<reflected_enum auto> typename Filter, typename ESeq>
    using filter_enum_sequence = util::type_list_filter_t<
        detail::enum_filter_wrapper<Filter>::template type, ESeq>;

    template<reflected_enum T> struct enum_names {};
    template<typename T> concept has_names = reflected_enum<T> && requires { enum_names<T>::value; };

    template<reflected_enum T> struct enum_full_names {};
    template<typename T> concept has_full_names = reflected_enum<T> && requires { enum_full_names<T>::value; };

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
        size_t i = 1;
        for (auto value : enum_values_v<T>) {
            if (value != static_cast<T>(i)) return false;
            ++i;
        }
        return true;
    }
    template<typename T> concept linear_enum = is_linear_enum<T>();

    template<reflected_enum T> constexpr size_t indexof(T value) {
        if constexpr (flags_enum<T>) {
            size_t i = 0;
            for (size_t n = static_cast<size_t>(value); n != 1; n >>= 1, ++i);
            return i;
        } else if constexpr (linear_enum<T>) {
            return static_cast<size_t>(value);
        } else if constexpr (std::ranges::is_sorted(enum_values_v<T>)) {
            return std::ranges::lower_bound(enum_values_v<T>, value) - enum_values_v<T>.begin();
        } else {
            return std::ranges::find(enum_values_v<T>, value) - enum_values_v<T>.begin();
        }
    }

    template<reflected_enum auto Enum> struct enum_data{};
    template<reflected_enum auto Enum> constexpr bool has_data = requires { enum_data<Enum>::value; };
    template<reflected_enum auto Enum> constexpr auto enum_data_v = enum_data<Enum>::value;
    template<reflected_enum auto Enum> using enum_data_t = decltype(enum_data<Enum>::value);

    template<reflected_enum Enum> auto get_data(Enum value) {
        constexpr auto data_array = []<Enum ... Es>(enum_sequence<Es...>) {
            return std::array{ enum_data_v<Es> ... };
        }(make_enum_sequence<Enum>());
        return data_array[indexof(value)];
    }
    
    template<reflected_enum auto Enum> struct enum_type{};
    template<reflected_enum auto Enum> constexpr bool has_type = requires { typename enum_type<Enum>::type; };
    template<reflected_enum auto Enum> using enum_type_t = typename enum_type<Enum>::type;

    template<reflected_enum T> constexpr T invalid_enum_value = static_cast<T>(std::numeric_limits<std::underlying_type_t<T>>::max());

    template<reflected_enum T> constexpr T from_string(std::string_view str) {
        if (auto it = std::ranges::find(enum_names<T>::value, str); it != enum_names<T>::value.end()) {
            return enum_values_v<T>[it - enum_names<T>::value.begin()];
        } else {
            return invalid_enum_value<T>;
        }
    }

    template<has_names T>
    constexpr std::string_view to_string(T value) {
        return enum_names<T>::value[indexof(value)];
    }
    
    template<has_full_names T>
    constexpr std::string_view full_name(T value) {
        return enum_full_names<T>::value[indexof(value)];
    }

    constexpr auto to_underlying(reflected_enum auto value) noexcept {
        return static_cast<std::underlying_type_t<decltype(value)>>(value);
    }

    inline namespace flag_operators {
        template<flags_enum T> constexpr T operator & (T lhs, T rhs) noexcept {
            return static_cast<T>(to_underlying(lhs) & to_underlying(rhs));
        }

        template<flags_enum T> constexpr T &operator &= (T &lhs, T rhs) noexcept {
            return lhs = lhs & rhs;
        }

        template<flags_enum T> constexpr T operator | (T lhs, T rhs) noexcept {
            return static_cast<T>(to_underlying(lhs) | to_underlying(rhs));
        }

        template<flags_enum T> constexpr T &operator |= (T &lhs, T rhs) noexcept {
            return lhs = lhs | rhs;
        }

        template<flags_enum T> constexpr T operator ~ (T value) noexcept {
            return static_cast<T>(~to_underlying(value));
        }

        template<flags_enum T> constexpr T operator ^ (T lhs, T rhs) noexcept {
            return static_cast<T>(to_underlying(lhs) ^ to_underlying(rhs));
        }

        template<flags_enum T> constexpr T &operator ^= (T &lhs, T rhs) noexcept {
            return lhs = lhs ^ rhs;
        }
    }

    inline namespace stream_operators {
        template<typename Stream, has_names T>
        Stream &operator << (Stream &out, const T &value) {
            return out << to_string(value);
        }

        template<typename Stream, has_names T> requires flags_enum<T>
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

#define ENUM_DATA_STRUCT(enumName, elementTuple) \
    template<> struct enum_data<enumName::ENUM_ELEMENT_NAME(elementTuple)> { \
        static constexpr auto value = ENUM_TUPLE_TAIL(elementTuple); \
    };

#define ENUM_TYPE_STRUCT(enumName, elementTuple) \
    template<> struct enum_type<enumName::ENUM_ELEMENT_NAME(elementTuple)> { \
        using type = ENUM_TUPLE_TAIL(elementTuple); \
    };

#define GENERATE_ENUM_CASE(r, enumNameFunTuple, elementTuple) \
    BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(elementTuple), 1), \
        DO_NOTHING, BOOST_PP_TUPLE_ELEM(1, enumNameFunTuple)) \
        (BOOST_PP_TUPLE_ELEM(0, enumNameFunTuple), elementTuple)

#define GENERATE_ENUM_STRUCTS(enumName, elementTupleSeq, value_fun_name) \
template<> struct enum_values<enumName> { \
    static constexpr std::array value { \
        BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(CREATE_ENUM_VALUES_ELEMENT, enumName, elementTupleSeq)) \
    }; \
}; \
template<> struct enum_full_names<enumName> { \
    static constexpr std::array<std::string_view, BOOST_PP_SEQ_SIZE(elementTupleSeq)> value { \
        BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(CREATE_ENUM_FULL_NAMES_ELEMENT, enumName, elementTupleSeq)) \
    }; \
}; \
template<> struct enum_names<enumName> { \
    static constexpr std::array<std::string_view, BOOST_PP_SEQ_SIZE(elementTupleSeq)> value { \
        BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(CREATE_ENUM_NAMES_ELEMENT, enumName, elementTupleSeq)) \
    }; \
}; \
BOOST_PP_SEQ_FOR_EACH(GENERATE_ENUM_CASE, (enumName, value_fun_name), elementTupleSeq)

#define IMPL_DEFINE_ENUM(enumName, elementTupleSeq, enum_value_fun, value_fun_name) \
enum class enumName : ENUM_INT(enum_value_fun, elementTupleSeq) { \
    BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH_I(enum_value_fun, enumName, elementTupleSeq)) \
}; namespace enums { \
GENERATE_ENUM_STRUCTS(enumName, elementTupleSeq, value_fun_name) \
}

#define IMPL_DEFINE_ENUM_IN_NS(namespaceName, enumName, elementTupleSeq, enum_value_fun, value_fun_name) \
    enum class enumName : ENUM_INT(enum_value_fun, elementTupleSeq) { \
        BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH_I(enum_value_fun, enumName, elementTupleSeq)) \
    }; \
} namespace enums { using namespace namespaceName; \
GENERATE_ENUM_STRUCTS(namespaceName::enumName, elementTupleSeq, value_fun_name) \
} namespace namespaceName {

#define DEFINE_ENUM(enumName, enumElements) \
    IMPL_DEFINE_ENUM(enumName, ADD_PARENTHESES(enumElements), CREATE_ENUM_ELEMENT, DO_NOTHING)
#define DEFINE_ENUM_FLAGS(enumName, enumElements) \
    IMPL_DEFINE_ENUM(enumName, ADD_PARENTHESES(enumElements), CREATE_FLAG_ELEMENT, DO_NOTHING)

#define DEFINE_ENUM_DATA(enumName, enumElements) \
    IMPL_DEFINE_ENUM(enumName, ADD_PARENTHESES(enumElements), CREATE_ENUM_ELEMENT, ENUM_DATA_STRUCT)
#define DEFINE_ENUM_FLAGS_DATA(enumName, enumElements) \
    IMPL_DEFINE_ENUM(enumName, ADD_PARENTHESES(enumElements), CREATE_FLAG_ELEMENT, ENUM_DATA_STRUCT)

#define DEFINE_ENUM_TYPES(enumName, enumElements) \
    IMPL_DEFINE_ENUM(enumName, ADD_PARENTHESES(enumElements), CREATE_ENUM_ELEMENT, ENUM_TYPE_STRUCT)

#define DEFINE_ENUM_IN_NS(namespaceName, enumName, enumElements) \
    IMPL_DEFINE_ENUM_IN_NS(namespaceName, enumName, ADD_PARENTHESES(enumElements), CREATE_ENUM_ELEMENT, DO_NOTHING)
#define DEFINE_ENUM_FLAGS_IN_NS(namespaceName, enumName, enumElements) \
    IMPL_DEFINE_ENUM_IN_NS(namespaceName, enumName, ADD_PARENTHESES(enumElements), CREATE_FLAG_ELEMENT, DO_NOTHING)

#define DEFINE_ENUM_DATA_IN_NS(namespaceName, enumName, enumElements) \
    IMPL_DEFINE_ENUM_IN_NS(namespaceName, enumName, ADD_PARENTHESES(enumElements), CREATE_ENUM_ELEMENT, ENUM_DATA_STRUCT)
#define DEFINE_ENUM_FLAGS_DATA_IN_NS(namespaceName, enumName, enumElements) \
    IMPL_DEFINE_ENUM_IN_NS(namespaceName, enumName, ADD_PARENTHESES(enumElements), CREATE_FLAG_ELEMENT, ENUM_DATA_STRUCT)

#define DEFINE_ENUM_TYPES_IN_NS(namespaceName, enumName, enumElements) \
    IMPL_DEFINE_ENUM_IN_NS(namespaceName, enumName, ADD_PARENTHESES(enumElements), CREATE_ENUM_ELEMENT, ENUM_TYPE_STRUCT)

#endif