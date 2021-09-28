#ifndef __CLIENT_CARD_H__
#define __CLIENT_CARD_H__

#include "common/card_enums.h"

#include "utils/sdl.h"

#include <concepts>
#include <vector>

namespace banggame {
    struct card_target {
        target_type target = target_type::none;
        int maxdistance = 0;
    };

    struct card_view {
        bool known = false;
        bool inactive = false;

        std::string name;
        std::string image;
        card_suit_type suit;
        card_value_type value;
        card_color_type color;
        std::vector<card_target> targets;
    };

    struct player_view {
        int hp = 0;

        std::string name;
        std::string image;

        int character_id = 0;
        target_type target = target_type::none;

        player_role role = player_role::unknown;
    };

    sdl::texture make_card_texture(const card_view &card);
    sdl::texture make_backface_texture();
}

#endif