#ifndef __INTL_H__
#define __INTL_H__

#include <string>
#include <fmt/format.h>
#include "common/enums.h"

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

    std::string format(const std::string &format_str, const std::vector<std::string> &args);
}

std::string _(const auto &str) {
    return intl::translate(str);
}

template<typename T, typename ... Ts>
std::string _(const T &str, const Ts & ... args) {
    return intl::format(intl::translate(str), args ...);
}

#endif