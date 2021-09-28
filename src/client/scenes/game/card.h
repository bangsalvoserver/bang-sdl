#ifndef __CLIENT_CARD_H__
#define __CLIENT_CARD_H__

#include "common/card_enums.h"
#include "common/update_enums.h"

#include "utils/sdl.h"

#include <concepts>
#include <vector>

namespace banggame {

    struct card_pile_view : std::vector<int> {
        SDL_Point pos;
        int xoffset;

        card_pile_view(int xoffset = 30) : xoffset(xoffset) {}

        auto find(int card_id) const {
            return std::ranges::find(*this, card_id);
        }

        SDL_Point get_position(int card_id) const {
            return SDL_Point{(int)(pos.x + xoffset *
                (std::ranges::distance(begin(), find(card_id)) - (size() - 1) * .5f)),
                pos.y};
        }

        void erase_card(int card_id) {
            if (auto it = find(card_id); it != end()) {
                erase(it);
            }
        }
    };

    struct card_view {
        bool known = false;
        bool inactive = false;

        std::string name;
        std::string image;
        card_suit_type suit;
        card_value_type value;
        card_color_type color;
        std::vector<card_target_data> targets;

        card_pile_view *pile = nullptr;
        
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

        card_pile_view hand;
        card_pile_view table;

        void set_position(SDL_Point pos, bool flipped = false) {
            hand.pos = table.pos = pos;
            if (flipped) {
                hand.pos.y += 60;
                table.pos.y -= 60;
            } else {
                hand.pos.y -= 60;
                table.pos.y += 60;
            }
        }
    };

    sdl::texture make_card_texture(const card_view &card);
    sdl::texture make_backface_texture();

    inline void scale_rect(SDL_Rect &rect, int new_w) {
        rect.h = new_w * rect.h / rect.w;
        rect.w = new_w;
    }
}

#endif