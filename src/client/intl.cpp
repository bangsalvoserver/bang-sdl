#include "intl.h"

#include "locales/locales.h"

namespace intl {
    DEFINE_ENUM_DATA_IN_NS(intl, language,
        (english, locale_en_txt)
        (italian, locale_it_txt)
    )
}

#ifdef WIN32
#include <winnls.h>

namespace intl {
    language get_system_language() {
        LANGID lang = GetUserDefaultUILanguage();
        switch(PRIMARYLANGID(lang)) {
        case LANG_ITALIAN:  return language::italian;
        case LANG_ENGLISH:
        default:            return language::english;
        }
    }
}

#else
#include <locale>

namespace intl {
    language get_system_language() {
        std::string lang = std::locale("").name();
        if (lang.starts_with("it_IT")) {
            return language::italian;
        } else {
            return language::english;
        }
    }
}

#endif

namespace intl {
    static const language current_language = get_system_language();

    std::string translate(std::string_view str) {
        return std::string(enums::visit_enum([&](auto enum_const) {
            constexpr auto strings = enums::enum_data_v<decltype(enum_const)::value>;
            auto it = strings.find(str);
            if (it == strings.end()) {
                return str;
            } else {
                return it->second;
            }
        }, current_language));
    }

    std::string format(const std::string &format_str, const std::vector<std::string> &args) {
        using ctx = fmt::format_context;
        std::vector<fmt::basic_format_arg<ctx>> fmt_args;
        fmt_args.reserve(args.size());
        for (const auto &a : args) {
            fmt_args.push_back(fmt::detail::make_arg<ctx>(a));
        }

        try {
            return fmt::vformat(format_str, fmt::basic_format_args<ctx>(fmt_args.data(), fmt_args.size()));
        } catch (const fmt::format_error &err) {
            return format_str;
        }
    }
}