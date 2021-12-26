#ifndef __FORMAT_STR_H__
#define __FORMAT_STR_H__

#include "common/utils.h"

namespace banggame {

    DEFINE_SERIALIZABLE(card_format_id,
        (player_id, int)
        (card_id, int)
    )

    DEFINE_SERIALIZABLE(player_format_id,
        (player_id, int)
    )

    using game_format_arg = std::variant<int, std::string, card_format_id, player_format_id>;
    
    struct game_formatted_string {
        SERIALIZABLE_DATA(game_formatted_string,
            (localized, bool)
            (format_str, std::string)
            (format_args, std::vector<game_format_arg>)
        )

        game_formatted_string() = default;
    
        template<std::convertible_to<std::string> T, typename ... Ts>
        game_formatted_string(T &&message, Ts && ... args);
    };
}

#endif