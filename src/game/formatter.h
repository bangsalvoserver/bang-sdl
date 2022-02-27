#ifndef __FORMATTER_H__
#define __FORMATTER_H__

#include "player.h"

#include "utils/utils.h"

namespace banggame{

    struct nonlocalized_string : std::string {
        using std::string::string;
    };

    inline nonlocalized_string operator "" _nonloc(const char *str, size_t len) {
        return nonlocalized_string(str, len);
    }

    struct with_owner {
        card *value;
    };

    template<std::convertible_to<std::string> T, typename ... Ts>
    game_formatted_string::game_formatted_string(T &&message, Ts && ... args)
        : localized(!std::is_same_v<std::remove_cvref_t<T>, nonlocalized_string>)
        , format_str(std::forward<T>(message))
        , format_args{util::overloaded{
            [](int value) { return value; },
            [](std::string value) { return value; },
            [](card *value) {
                return card_format_id{0, value ? value->id : 0};
            },
            [](with_owner value) {
                return card_format_id{
                    value.value && value.value->owner ? value.value->owner->id : 0,
                    value.value ? value.value->id : 0};
            },
            [](player *value) {
                return player_format_id{value ? value->id : 0};
            }
        }(std::forward<Ts>(args)) ...} {}

}

#endif