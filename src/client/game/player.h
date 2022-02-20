#ifndef __CLIENT_PLAYER_H__
#define __CLIENT_PLAYER_H__

#include "card.h"

#include "../widgets/stattext.h"

namespace banggame {
    struct player_view {
        int id;

        int user_id;
        
        explicit player_view(int id);

        int hp = 0;
        int gold = 0;
        bool dead = false;

        card_pile_view hand{options::player_hand_width};
        card_pile_view table{options::player_hand_width};

        sdl::rect m_bounding_rect;
        uint32_t border_color = 0;

        character_pile m_characters;
        card_pile_view m_backup_characters;
        
        role_card m_role;

        widgets::stattext m_username_text;

        widgets::stattext m_gold_text;

        const sdl::texture *m_profile_image = nullptr;
        sdl::rect m_profile_rect;

        player_flags m_player_flags = enums::flags_none<player_flags>;

        int m_range_mod = 0;
        int m_weapon_range = 1;
        int m_distance_mod = 0;

        bool has_player_flags(player_flags flags) const {
            using namespace enums::flag_operators;
            return (m_player_flags & flags) == flags;
        }

        void set_position(sdl::point pos, bool flipped = false);

        void set_hp_marker_position(float hp);

        void set_gold(int amount);

        void set_username(const std::string &name);
        void set_profile_image(const sdl::texture *image);

        void render(sdl::renderer &renderer);
    };
}
#endif