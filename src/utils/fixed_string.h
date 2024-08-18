#ifndef __FIXED_STRING_H__
#define __FIXED_STRING_H__

#include <string_view>

namespace utils {

    template<typename T, std::size_t Size>
    struct fixed_string {
        constexpr explicit(false) fixed_string(const T* str) {
            for (decltype(Size) i{}; i < Size; ++i) { data[i] = str[i]; }
            data[Size] = T();
        }
        [[nodiscard]] constexpr auto operator<=>(const fixed_string&) const = default;
        [[nodiscard]] constexpr explicit(false) operator std::basic_string_view<T>() const { return {std::data(data), Size}; }
        [[nodiscard]] constexpr auto size() const { return Size; }
        T data[Size+1];
    };

    template<class T, std::size_t Capacity, std::size_t Size = Capacity-1> fixed_string(const T (&str)[Capacity]) -> fixed_string<T, Size>;

}

#endif