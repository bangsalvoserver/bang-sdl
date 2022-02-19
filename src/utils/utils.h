#ifndef __UTILS_H__
#define __UTILS_H__

#include "enum_variant.h"
#include "svstream.h"

#include <iostream>
#include <ranges>
#include <map>

namespace util {

    template<typename T>
    struct id_counter {
        static inline int counter = 0;
        const int id;
        id_counter() : id(++counter) {}
    };

    template<typename ... Ts> struct overloaded : Ts ... { using Ts::operator() ...; };
    template<typename ... Ts> overloaded(Ts ...) -> overloaded<Ts ...>;

    template<enums::flags_enum E>
    auto enum_flag_values(E value) {
        return enums::enum_values_v<E> | std::views::filter([=](E item) {
            using namespace enums::flag_operators;
            return bool(item & value);
        });
    }

    template<typename T> struct nocopy_wrapper : T {
        nocopy_wrapper(T &&value) : T(std::move(value)) {}

        nocopy_wrapper(nocopy_wrapper &&) = default;
        nocopy_wrapper &operator = (nocopy_wrapper &&) = default;

        nocopy_wrapper(const nocopy_wrapper &value) : T(const_cast<T &&>(static_cast<const T &>(value))) {
            throw -1;
        }

        nocopy_wrapper &operator = (const nocopy_wrapper &) {
            throw -1;
        }
    };

    template<typename T> nocopy_wrapper(T &&) -> nocopy_wrapper<T>;

}

#endif