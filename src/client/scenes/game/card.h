#ifndef __CLIENT_CARD_H__
#define __CLIENT_CARD_H__

#include "common/card_enums.h"
#include "common/update_enums.h"

#include "utils/sdl.h"

#include <concepts>
#include <vector>

namespace banggame {

    struct card_view {
        bool known = false;
        bool inactive = false;

        std::string name;
        std::string image;
        card_suit_type suit;
        card_value_type value;
        card_color_type color;
        std::vector<card_target_data> targets;

        card_pile_type pile = card_pile_type::main_deck;
        int player_id = 0;
        
        sdl::texture texture_front;
        static inline sdl::texture texture_back;

        SDL_Point pos;
        float flip_amt = 0.f;
        float rotation = 0.f;

        void render(sdl::renderer &renderer);
    };

    struct player_view {
        int hp = 0;

        std::string name;
        std::string image;

        int character_id = 0;
        target_type target = target_type::none;

        player_role role = player_role::unknown;

        std::vector<int> hand;
        std::vector<int> table;
    };

    sdl::texture make_card_texture(const card_view &card);
    sdl::texture make_backface_texture();

    inline void scale_rect(SDL_Rect &rect, int new_w) {
        rect.h = new_w * rect.h / rect.w;
        rect.w = new_w;
    }
}

#endif