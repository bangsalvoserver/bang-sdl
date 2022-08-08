#include "intl.h"

#include "locales/locale_en.h"
#include "locales/locale_it.h"
#include "locales/cards_en.h"
#include "locales/cards_it.h"

#ifdef WIN32
#include <windows.h>

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

    std::string translate(category cat, std::string_view str) {
        return std::string(enums::visit_enum([&](auto category_tag, auto language_tag) {
            if constexpr (requires { get_language_translations(category_tag, language_tag); }) {
                static constexpr auto strings = get_language_translations(category_tag, language_tag);
                auto it = strings.find(str);
                if (it == strings.end()) {
                    return str;
                } else {
                    return it->second;
                }
            } else {
                return str;
            }
        }, cat, current_language));
    }
}