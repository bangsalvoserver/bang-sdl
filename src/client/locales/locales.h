#ifndef __LOCALES_H__
#define __LOCALES_H__

#include "utils/static_map.h"
#include "utils/enums.h"

namespace intl {
    DEFINE_ENUM_IN_NS(intl, language,
        (english)
        (italian)
    )
}

#define BEGIN_LOCALE(LANG) \
namespace enums { \
    template<> struct enum_data<language::LANG> { \
        static constexpr auto value = util::static_map<std::string_view, std::string_view>({

#define LOCALE_VALUE(name, value) {#name, value},

#define END_LOCALE() \
        }); \
    }; \
}

#endif