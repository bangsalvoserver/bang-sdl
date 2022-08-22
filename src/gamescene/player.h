#ifndef __CLIENT_PLAYER_H__
#define __CLIENT_PLAYER_H__

#include "card.h"

#include "../widgets/stattext.h"
#include "../widgets/profile_pic.h"

namespace banggame {
    class game_scene;

    class player_view {
    public:
        game_scene *m_game;
        int id;

        int user_id;
        
        player_view(game_scene *game, int id);

        virtual ~player_view() = default;

        int hp = 0;
        int gold = 0;

        wide_pocket hand{options.player_hand_width};
        wide_pocket table{options.player_hand_width};
        
        pocket_view scenario_deck;

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

        bool alive() const {
            return !has_player_flags(player_flags::dead)
                || has_player_flags(player_flags::ghost)
                || has_player_flags(player_flags::temp_ghost);
        }

        void set_hp_marker_position(float hp);
        void set_gold(int amount);

        sdl::point get_position() const;
        virtual void set_position(sdl::point pos) = 0;
        virtual void set_username(const std::string &name) = 0;
        virtual void render(sdl::renderer &renderer) = 0;
    };

    class alive_player_view : public player_view {
    public:
        using player_view::player_view;
        void set_position(sdl::point pos) override;
        void set_username(const std::string &name) override;
        void render(sdl::renderer &renderer) override;
    };

    class dead_player_view : public player_view {
    public:
        using player_view::player_view;
        void set_position(sdl::point pos) override;
        void set_username(const std::string &name) override;
        void render(sdl::renderer &renderer) override;
    };
}
#endif