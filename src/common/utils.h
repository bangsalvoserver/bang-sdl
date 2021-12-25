#ifndef __UTILS_H__
#define __UTILS_H__

#include "enum_variant.h"
#include "sdlnet.h"
#include "svstream.h"
#include "json_serial.h"

#include <iostream>
#include <ranges>

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

}

#endif