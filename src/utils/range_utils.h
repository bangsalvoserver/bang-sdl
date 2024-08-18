#ifndef __RANGE_UTILS_H__
#define __RANGE_UTILS_H__

#include "misc.h"

#include <stdexcept>

template<rn::input_range R> requires std::is_pointer_v<rn::range_value_t<R>>
rn::range_value_t<R> get_single_element(R &&range) {
    auto begin = rn::begin(range);
    auto end = rn::end(range);

    if (begin != end) {
        auto first = *begin;
        if (++begin == end) {
            return first;
        }
    }
    return nullptr;
}

template<rn::input_range R>
bool contains_at_least(R &&range, int size) {
    if (size == 0) return true;
    for (const auto &value : range) {
        if (--size == 0) return true;
    }
    return false;
}

struct random_element_error : std::runtime_error {
    random_element_error(): std::runtime_error{"Empty range in random_element"} {}
};

template<rn::forward_range R, typename Rng>
decltype(auto) random_element(R &&range, Rng &rng) {
    rn::range_value_t<R> ret;
    if (rn::sample(std::forward<R>(range), &ret, 1, rng).out == &ret) {
        throw random_element_error();
    }
    return ret;
}

#endif