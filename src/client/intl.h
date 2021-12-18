#ifndef __INTL_H__
#define __INTL_H__

#include <string>
#include <cstdio>
#include "common/enums.h"

namespace intl {
    std::string translate(std::string_view str);

    std::string translate(enums::reflected_enum auto value) {
        return translate(enums::full_name(value));
    }

    inline const auto &conv_string(const auto &val) {
        return val;
    }

    inline const char *conv_string(const std::string &val) {
        return val.c_str();
    }

    template<typename T, typename ... Ts>
    std::string translate_format(const T &str, const Ts & ... args) {
        std::string format_str = translate(str);
        std::string buffer(format_str.size() + 100, '\0');
        int len = std::snprintf(buffer.data(), buffer.size(), format_str.c_str(), conv_string(args) ...);
        if (len > 0) {
            buffer.resize(len);
        } else {
            buffer = format_str;
        }
        return buffer;
    }
}

std::string _(const auto &str) {
    return intl::translate(str);
}

template<typename T, typename ... Ts>
std::string _(const T &str, const Ts & ... args) {
    return intl::translate_format(str, args ...);
}

#endif