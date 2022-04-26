#ifndef __FORMATTER_H__
#define __FORMATTER_H__

#include "player.h"

namespace banggame{

    struct game_format_arg_visitor {
        game_format_arg operator()(int value) const {
            return value;
        }

        game_format_arg operator()(std::string value) const {
            return std::move(value);
        }

        game_format_arg operator()(player *value) const {
            return player_format_id{value->id};
        }

        game_format_arg operator()(card *value) const {
            return card_format_id{value->name, value->sign};
        }
    };

    template<std::convertible_to<std::string> T, typename ... Ts>
    game_formatted_string::game_formatted_string(T &&message, Ts && ... args)
        : format_str(std::forward<T>(message))
        , format_args{game_format_arg_visitor{}(std::forward<Ts>(args)) ...} {}

}

#endif