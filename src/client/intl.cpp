#include "intl.h"

#include "locales/locale_en.h"
#include "locales/locale_it.h"

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
        return std::string(enums::visit_enum([&]<language E>(enums::enum_tag_t<E>) {
            static constexpr auto strings = get_language_translations(enums::enum_tag<E>);
            auto it = strings.find(str);
            if (it == strings.end()) {
                return str;
            } else {
                return it->second;
            }
        }, current_language));
    }
}