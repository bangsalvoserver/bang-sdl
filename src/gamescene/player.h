#ifndef __CLIENT_PLAYER_H__
#define __CLIENT_PLAYER_H__

#include "card.h"

#include "cards/filters.h"

#include "../widgets/stattext.h"
#include "../widgets/profile_pic.h"

namespace banggame {
    class game_scene;

    class player_view : public game_style_set {
    private:
        struct player_state_vtable {
            void (*set_position)(player_view *self, sdl::point pos);
            void (*render)(player_view *self, sdl::renderer &renderer);
        };

        static const player_state_vtable state_alive;
        static const player_state_vtable state_dead;

        const player_state_vtable *m_state = &state_alive;
    
    public:
        player_view(game_scene *game, int id);

        game_scene *m_game;

        int id;

        int user_id;

        int hp = 0;
        int gold = 0;

        wide_pocket hand{options.card_pocket_width, pocket_type::player_hand, this};
        wide_pocket table{options.card_pocket_width, pocket_type::player_table, this};
        
        point_pocket_view scenario_deck{pocket_type::scenario_deck};
        point_pocket_view wws_scenario_deck{pocket_type::wws_scenario_deck};

        sdl::rect m_bounding_rect;

        character_pile m_characters{pocket_type::player_character, this};
        point_pocket_view m_backup_characters{pocket_type::player_backup, this};
        
        role_card m_role;

        widgets::stattext m_username_text;

        widgets::stattext m_gold_text;

        widgets::profile_pic m_propic;

        player_flags m_player_flags{};

        int m_range_mod = 0;
        int m_weapon_range = 1;
        int m_distance_mod = 0;

        void set_user_id(int value);

        bool has_player_flags(player_flags flags) const {
            return (m_player_flags & flags) == flags;
        }

        bool alive() const {
            return filters::is_player_alive(this);
        }

        void set_hp_marker_position(float hp);
        void set_gold(int amount);

        void set_to_dead() {
            m_state = &state_dead;
        }

        sdl::point get_position() const {
            return sdl::rect_center(m_bounding_rect);
        }

        void set_position(sdl::point pos) {
            m_state->set_position(this, pos);
        }

        void render(sdl::renderer &renderer) {
            m_state->render(this, renderer);
        }
    };
}
#endif