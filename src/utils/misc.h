#ifndef __UTILS_H__
#define __UTILS_H__

#ifdef USE_STD_RANGES

#include <ranges>

namespace rn = std::ranges;
namespace rv = std::views;

#else

#include <range/v3/all.hpp>

namespace rn = ranges;
namespace rv = ranges::views;

#endif

#define FWD(x) std::forward<decltype(x)>(x)

template<typename ... Ts> struct overloaded : Ts ... { using Ts::operator() ...; };
template<typename ... Ts> overloaded(Ts ...) -> overloaded<Ts ...>;

template<typename T>
class nullable_ref {
private:
    T *m_ptr = nullptr;

public:
    nullable_ref() = default;
    nullable_ref(T &value): m_ptr{&value} {}

    operator T &() const { return *m_ptr; }
    operator const T &() const { return *m_ptr; }
};

#endif