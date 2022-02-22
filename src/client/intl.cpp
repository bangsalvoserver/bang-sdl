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
            static constexpr auto strings = enums::enum_data_v<decltype(enum_const)::value>;
            auto it = strings.find(str);
            if (it == strings.end()) {
                return str;
            } else {
                return it->second;
            }
        }, current_language));
    }
}