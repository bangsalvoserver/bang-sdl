#ifndef __INTL_H__
#define __INTL_H__

#include <string>
#include <ranges>
#include <stdexcept>
#include <format>

#include "locales/locales.h"

namespace intl {
    std::string translate(category cat, std::string_view str);

    template<enums::enumeral T> 
    std::string translate(category cat, T value) {
        return translate(cat, std::format("{}::{}", reflect::type_name<T>(), enums::to_string(value)));
    }

    template<typename ... Ts>
    std::string format(const std::string &format_str, const Ts & ... args) {
        try {
            return std::vformat(format_str, std::make_format_args(args ... ));
        } catch (const std::format_error &) {
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