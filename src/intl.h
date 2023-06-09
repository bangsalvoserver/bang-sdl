#ifndef __INTL_H__
#define __INTL_H__

#include <string>
#include <ranges>
#include <stdexcept>

#include "locales/locales.h"
#include "utils/format.h"

namespace intl {
    std::string translate(category cat, std::string_view str);

    std::string translate(category cat, enums::reflected_enum auto value) {
        return translate(cat, fmt::format("{}::{}", enums::enum_name_v<decltype(value)>, enums::to_string(value)));
    }

    template<typename ... Ts>
    std::string format(const std::string &format_str, const Ts & ... args) {
        try {
            return fmt::vformat(format_str, fmt::make_format_args(args ... ));
        } catch (const fmt::format_error &) {
            return format_str;
        }
    }

    template<typename T, size_t Size, size_t ... Is>
    auto make_array_format_args_impl(std::array<T, Size> &format_args, std::index_sequence<Is...>) {
        return fmt::make_format_args(format_args[Is] ...);
    }

    template<typename T, size_t Size>
    auto make_array_format_args(std::array<T, Size> &format_args) {
        return make_array_format_args_impl(format_args, std::make_index_sequence<Size>());
    }

    template<std::ranges::sized_range R> requires std::convertible_to<std::ranges::range_value_t<R>, std::string>
    std::string format(const std::string &format_str, R &&args) {
        std::array<std::string, 5> format_args;
        if (args.size() <= format_args.size()) {
            for (size_t i=0; i<args.size(); ++i) {
                format_args[i] = args[i];
            }
            try {
                return fmt::vformat(format_str, make_array_format_args(format_args));
            } catch (const fmt::format_error &) {
                // ignore
            }
        }
        return format_str;
    }
}

std::string _(intl::category cat, const auto &str) {
    return intl::translate(cat, str);
}

template<typename T, typename ... Ts>
std::string _(intl::category cat, const T &str, const Ts & ... args) {
    return intl::format(intl::translate(cat, str), args ...);
}

std::string _(const auto & ... args) {
    return _(intl::category::basic, args ...);
}

#endif