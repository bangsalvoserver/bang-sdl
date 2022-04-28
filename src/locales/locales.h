#ifndef __LOCALES_H__
#define __LOCALES_H__

#include "utils/static_map.h"
#include "utils/enums.h"

namespace intl {
    DEFINE_ENUM(language,
        (english)
        (italian)
    )

    DEFINE_ENUM(category,
        (basic)
        (cards)
    )
}

#define BEGIN_LOCALE(CAT, LANG) \
namespace intl { \
    constexpr auto get_language_translations(enums::enum_tag_t<category::CAT>, enums::enum_tag_t<language::LANG>) { \
        return util::static_map<std::string_view, std::string_view>({

#define LOCALE_VALUE(name, value) {#name, value},

#define END_LOCALE() \
        }); \
    } \
}

#endif