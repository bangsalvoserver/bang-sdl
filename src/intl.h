#ifndef __INTL_H__
#define __INTL_H__

#include <string>
#include <ranges>
#include <stdexcept>
#include <fmt/format.h>
#include <fmt/args.h>

#include "locales/locales.h"

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