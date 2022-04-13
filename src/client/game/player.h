#ifndef __CLIENT_PLAYER_H__
#define __CLIENT_PLAYER_H__

#include "card.h"

#include "../widgets/stattext.h"
#include "../widgets/profile_pic.h"

namespace banggame {
    struct player_view {
        int id;

        int user_id;
        
        explicit player_view(int id);

        int hp = 0;
        int gold = 0;
        bool dead = false;

        wide_pocket hand{options.player_hand_width};
        wide_pocket table{options.player_hand_width};

        sdl::rect m_bounding_rect;
        sdl::color border_color{};

        character_pile m_characters;
        pocket_view m_backup_characters;
        
        role_card *m_role;

        widgets::stattext m_username_text;

        widgets::stattext m_gold_text;

        widgets::profile_pic m_propic;

        player_flags m_player_flags{};

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

        void render(sdl::renderer &renderer);
    };
}
#endif