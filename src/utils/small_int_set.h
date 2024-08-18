#ifndef __SMALL_INT_SET_H__
#define __SMALL_INT_SET_H__

#include <array>
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <initializer_list>
#include <bit>

#include "json_serial.h"

class small_int_set_iterator {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using value_type = int;
    using reference = int;

private:
    uint8_t value;
    uint8_t index;

public:
    constexpr small_int_set_iterator(uint8_t value, uint8_t index)
        : value{value}, index{index} {}
    
    constexpr int operator *() const {
        return index;
    }

    constexpr small_int_set_iterator &operator++() {
        uint8_t mask = (1 << (index + 1)) - 1;
        index = std::countr_zero<uint8_t>(value & ~mask);
        return *this;
    }

    constexpr small_int_set_iterator operator++(int) {
        auto copy = *this;
        ++*this;
        return copy;
    }

    constexpr small_int_set_iterator &operator--() {
        uint8_t mask = (1 << index) - 1;
        index = 7 - std::countl_zero<uint8_t>(value & mask);
        return *this;
    }

    constexpr small_int_set_iterator operator--(int) {
        auto copy = *this;
        --*this;
        return copy;
    }

    constexpr bool operator == (const small_int_set_iterator &other) const = default;
};

struct small_int_set_sized_tag_t {};

static constexpr small_int_set_sized_tag_t small_int_set_sized_tag;

class small_int_set {
private:
    uint8_t m_value{};

    constexpr small_int_set(uint8_t value)
        : m_value{value} {}

public:
    constexpr small_int_set(std::initializer_list<int> values) {
        int prev = -1;
        for (int value : values) {
            if (value < 0 || value >= 8) {
                throw std::runtime_error("invalid small_int_set, ints must be in range 0-7");
            }
            if (value <= prev) {
                throw std::runtime_error("invalid small_int_set, values must be in ascending order");
            }
            m_value |= 1 << value;
            prev = value;
        }
    }

    constexpr small_int_set(small_int_set_sized_tag_t, size_t size)
        : m_value{static_cast<uint8_t>(size == 0 ? 0 : (1 << size) - 1)} {}

    constexpr auto begin() const {
        return small_int_set_iterator{m_value, static_cast<uint8_t>(std::countr_zero(m_value))};
    }

    constexpr auto end() const {
        return small_int_set_iterator{m_value, 8};
    }

    constexpr size_t size() const {
        return std::popcount(m_value);
    }

    constexpr int operator[](size_t index) const {
        return *(std::next(begin(), index));
    }
};

namespace json {
    template<typename Context>
    struct serializer<small_int_set, Context> {
        json operator()(small_int_set value) const {
            auto ret = json::array();
            for (int n : value) {
                ret.push_back(n);
            }
            return ret;
        }
    };
}

#endif