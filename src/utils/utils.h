#ifndef __UTILS_H__
#define __UTILS_H__

#include "enum_variant.h"
#include "sdlnet.h"
#include "svstream.h"

namespace util {

    template<typename T>
    struct id_counter {
        static inline int counter = 0;
        const int id;
        id_counter() : id(++counter) {}
    };

    template<typename ... Ts> struct overloaded : Ts ... { using Ts::operator() ...; };
    template<typename ... Ts> overloaded(Ts ...) -> overloaded<Ts ...>;

}

#endif