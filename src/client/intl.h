#ifndef __INTL_H__
#define __INTL_H__

#include <string>
#include <ranges>
#include <fmt/format.h>
#include <fmt/args.h>

#include "utils/enums.h"

namespace intl {
    std::string translate(std::string_view str);

    std::string translate(enums::reflected_enum auto value) {
        return translate(enums::full_name(value));
    }

    template<typename ... Ts>
    std::string format(const std::string &format_str, const Ts & ... args) {
        try {
            return fmt::vformat(format_str, fmt::make_format_args(args ... ));
        } catch (const fmt::format_error &err) {
            return format_str;
        }
    }

    template<std::ranges::input_range R> requires std::convertible_to<std::ranges::range_value_t<R>, std::string>
    std::string format(const std::string &format_str, R &&args) {
        fmt::dynamic_format_arg_store<fmt::format_context> fmt_args;
        for (const std::string &arg : args) {
            fmt_args.push_back(arg);
        }

        try {
            return fmt::vformat(format_str, fmt_args);
        } catch (const fmt::format_error &err) {
            return format_str;
        }
    }
}

std::string _(const auto &str) {
    return intl::translate(str);
}

template<typename T, typename ... Ts>
std::string _(const T &str, const Ts & ... args) {
    return intl::format(intl::translate(str), args ...);
}

#endif