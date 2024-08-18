#ifndef __LOCALES_H__
#define __LOCALES_H__

#include "utils/static_map.h"
#include "utils/enums.h"

namespace intl {
    enum class language {
        english,
        italian,
    };

    template<language E> struct language_tag {};

    enum class category {
        basic,
        cards,
    };

    template<category E> struct category_tag {};
}

#define BEGIN_LOCALE(CAT, LANG) \
namespace intl { \
    constexpr auto get_language_translations(category_tag<category::CAT>, language_tag<language::LANG>) { \
        return utils::static_map<std::string_view, std::string_view>({

#define LOCALE_VALUE(name, value) {#name, value},

#define END_LOCALE() \
        }); \
    } \
}

#endif