#ifndef __UTILS_H__
#define __UTILS_H__

namespace util {

    template<typename ... Ts> struct overloaded : Ts ... { using Ts::operator() ...; };
    template<typename ... Ts> overloaded(Ts ...) -> overloaded<Ts ...>;

}

#endif