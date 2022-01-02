#ifndef __CLIENT_PLAYER_H__
#define __CLIENT_PLAYER_H__

#include "card.h"

namespace banggame {
    struct player_view {
        int id;

        int user_id;
        
        explicit player_view(int id) : id(id) {}

        int hp = 0;
        int gold = 0;
        bool dead = false;

        card_pile_view hand{sizes::player_hand_width};
        card_pile_view table{sizes::player_hand_width};

        sdl::rect m_bounding_rect;

        card_pile_view m_characters;
        
        sdl::point m_hp_marker_pos;
        role_card m_role;

        sdl::stattext m_username_text;

        sdl::stattext m_gold_text;

        sdl::texture *m_profile_image = nullptr;
        sdl::rect m_profile_rect;
        
        static inline sdl::texture m_gold_texture;

        void set_position(sdl::point pos, bool flipped = false);

        void set_hp_marker_position(float hp);

        void set_gold(int amount);

        void set_username(const std::string &name);
        void set_profile_image(sdl::texture *image);

        void render(sdl::renderer &renderer);
        void render_turn_indicator(sdl::renderer &renderer);
        void render_request_origin_indicator(sdl::renderer &renderer);
        void render_request_target_indicator(sdl::renderer &renderer);
    };
}
#endif