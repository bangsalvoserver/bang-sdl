#ifndef __FORMATTER_H__
#define __FORMATTER_H__

#include "common/format_str.h"
#include "player.h"

namespace banggame{

    struct unlocalized_string : std::string {
        using std::string::string;
    };

    inline unlocalized_string operator "" _unloc(const char *str, size_t len) {
        return unlocalized_string(str, len);
    }

    template<std::convertible_to<std::string> T, typename ... Ts>
    game_formatted_string make_formatted_string(T &&message, Ts && ... args) {
        return game_formatted_string{
            !std::is_same_v<std::decay_t<T>, unlocalized_string>,
            std::forward<T>(message),
            std::vector<game_format_arg>{util::overloaded{
                [](std::nullptr_t) { return "-"; },
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
            }(std::forward<Ts>(args)) ...}
        };
    }

}

#endif