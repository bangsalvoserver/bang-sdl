#ifndef __SMALL_STRING_H__
#define __SMALL_STRING_H__

#include <array>
#include <string_view>
#include <algorithm>

template<size_t MaxSize>
class basic_small_string {
private:
    std::array<char, MaxSize> m_str;
    size_t m_size = 0;

public:
    constexpr basic_small_string() = default;

    template<size_t N>
    constexpr basic_small_string(const char (&str)[N])
        : m_size{N - 1}
    {
        static_assert(N - 1 <= MaxSize, "String is too large");
        std::copy(str, str + m_size, m_str.begin());
    }

    constexpr bool empty() const {
        return m_size == 0;
    }

    constexpr size_t size() const {
        return m_size;
    }

    constexpr operator std::string_view() const {
        return std::string_view{m_str.data(), m_str.data() + size()};
    }
};

using small_string = basic_small_string<32>;

#endif