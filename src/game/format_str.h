#ifndef __FORMAT_STR_H__
#define __FORMAT_STR_H__

#include "card_enums.h"

#include <stdexcept>
#include <optional>

namespace banggame {

    struct card_format_id {REFLECTABLE(
        (std::string) name,
        (card_sign) sign
    )};

    struct player_format_id {REFLECTABLE(
        (int) player_id
    )};

    using game_format_arg = std::variant<int, std::string, card_format_id, player_format_id>;
    
    struct game_formatted_string {
        REFLECTABLE(
            (std::string) format_str,
            (std::vector<game_format_arg>) format_args
        )

        game_formatted_string() = default;
    
        template<std::convertible_to<std::string> T, typename ... Ts>
        game_formatted_string(T &&message, Ts && ... args);
    };

    using opt_fmt_str = std::optional<game_formatted_string>;

    struct game_error : std::exception, game_formatted_string {
        using game_formatted_string::game_formatted_string;

        const char *what() const noexcept override {
            return format_str.c_str();
        }
    };

    using opt_error = std::optional<game_error>;
}

#endif