#ifndef __FORMATTER_H__
#define __FORMATTER_H__

#include "player.h"

namespace banggame{

    struct nonlocalized_string : std::string {
        using std::string::string;
    };

    inline nonlocalized_string operator "" _nonloc(const char *str, size_t len) {
        return nonlocalized_string(str, len);
    }

    template<std::convertible_to<std::string> T, typename ... Ts>
    game_formatted_string::game_formatted_string(T &&message, Ts && ... args)
        : localized(!std::is_same_v<std::remove_cvref_t<T>, nonlocalized_string>)
        , format_str(std::forward<T>(message))
        , format_args{util::overloaded{
            [](int value) { return value; },
            [](std::string value) { return value; },
            [](card *value) {
                return card_format_id{
                    value && value->owner ? value->owner->id : 0,
                    value ? value->id : 0};
            },
            [](player *value) {
                return player_format_id{value ? value->id : 0};
            }
        }(std::forward<Ts>(args)) ...} {}

}

#endif