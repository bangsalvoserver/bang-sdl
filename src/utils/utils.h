#ifndef __UTILS_H__
#define __UTILS_H__

#include <utility>

namespace util {

    template<typename T>
    struct id_counter {
        static inline int counter = 0;
        const int id;
        id_counter() : id(++counter) {}
    };

    template<typename ... Ts> struct overloaded : Ts ... { using Ts::operator() ...; };
    template<typename ... Ts> overloaded(Ts ...) -> overloaded<Ts ...>;

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