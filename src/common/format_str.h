#ifndef __FORMAT_STR_H__
#define __FORMAT_STR_H__

#include "common/utils.h"

namespace banggame {

    struct card_format_id {REFLECTABLE(
        (int) player_id,
        (int) card_id
    )};

    struct player_format_id {REFLECTABLE(
        (int) player_id
    )};

    using game_format_arg = std::variant<int, std::string, card_format_id, player_format_id>;
    
    struct game_formatted_string {
        REFLECTABLE(
            (bool) localized,
            (std::string) format_str,
            (std::vector<game_format_arg>) format_args
        )

        game_formatted_string() = default;
    
        template<std::convertible_to<std::string> T, typename ... Ts>
        game_formatted_string(T &&message, Ts && ... args);
    };
}

#endif