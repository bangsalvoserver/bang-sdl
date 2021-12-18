#include "intl.h"
#include "common/resource.h"
#include "common/svstream.h"

#include <map>

DECLARE_RESOURCE(locale_en_txt)
DECLARE_RESOURCE(locale_it_txt)

namespace intl {
    DEFINE_ENUM_IN_NS(intl, language,
        (english)
        (italian)
    )

    template<language Lang> static const resource_view get_res_locale() = delete;

    template<> const resource_view get_res_locale<language::english>() { return GET_RESOURCE(locale_en_txt); }
    template<> const resource_view get_res_locale<language::italian>() { return GET_RESOURCE(locale_it_txt); }
}

#ifdef WIN32
#include <winnls.h>

namespace intl {
    language get_system_language() {
        LANGID lang = GetUserDefaultUILanguage();
        if (lang == 1040) {
            return language::italian;
        } else {
            return language::english;
        }
    }
}

#else

namespace intl {
    language get_system_language() {
        return language::english;
    }
}

#endif

static const auto resource_stream(const resource_view &view) {
    return util::isviewstream({view.data, view.length});
}

namespace intl {
    static const auto strings = []{
        std::map<std::string, std::string, std::less<>> strings;

        constexpr auto lut = []<language ... Es>(enums::enum_sequence<Es...>) {
            return std::array {
                get_res_locale<Es> ...
            };
        }(enums::make_enum_sequence<language>());

        auto stream = resource_stream(lut[enums::indexof(get_system_language())]());

        std::string line;
        while (std::getline(stream, line)) {
            if (line.empty()) continue;
            size_t space_pos = line.find_first_of(" \t");
            size_t not_space_pos = line.find_first_not_of(" \t", space_pos);
            strings.emplace(line.substr(0, space_pos), line.substr(not_space_pos, line.size() - 1 - not_space_pos));
        }

        return strings;
    }();

    std::string translate(std::string_view str) {
        auto it = strings.find(str);
        if (it == strings.end()) {
            return std::string(str);
        } else {
            return it->second;
        }
    }
}