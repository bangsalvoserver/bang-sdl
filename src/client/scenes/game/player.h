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

        character_pile m_characters;
        character_pile m_backup_characters;
        
        role_card m_role;

        sdl::stattext m_username_text;

        sdl::stattext m_gold_text;

        sdl::texture *m_profile_image = nullptr;
        sdl::rect m_profile_rect;

        player_flags m_player_flags = enums::flags_none<player_flags>;

        int m_range_mod = 0;
        int m_weapon_range = 1;
        int m_distance_mod = 0;

        bool has_player_flags(player_flags flags) const {
            using namespace enums::flag_operators;
            return (m_player_flags & flags) == flags;
        }
        
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