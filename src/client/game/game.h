#ifndef __SCENE_GAME_H__
#define __SCENE_GAME_H__

#include "../scenes/scene_base.h"

#include "animation.h"
#include "game_ui.h"

#include "target_finder.h"

#include "utils/id_map.h"

#include <deque>
#include <random>

#define HANDLE_UPDATE(name, ...) handle_game_update(enums::enum_constant<game_update_type::name> __VA_OPT__(,) __VA_ARGS__)

namespace banggame {

    class game_scene : public scene_base {
    public:
        game_scene(class client_manager *parent, const game_started_args &args);
        
        void refresh_layout() override;
        void render(sdl::renderer &renderer) override;
        void handle_event(const sdl::event &event) override;

        void handle_game_update(const game_update &update) override;

        void add_user(int id, const user_info &args) override;

        void remove_user(int id) override;

    private:
        void HANDLE_UPDATE(game_over,        const game_over_update &args);
        void HANDLE_UPDATE(game_error,       const game_formatted_string &args);
        void HANDLE_UPDATE(game_log,         const game_formatted_string &args);
        void HANDLE_UPDATE(add_cards,        const add_cards_update &args);
        void HANDLE_UPDATE(remove_cards,     const remove_cards_update &args);
        void HANDLE_UPDATE(move_card,        const move_card_update &args);
        void HANDLE_UPDATE(add_cubes,        const add_cubes_update &args);
        void HANDLE_UPDATE(move_cube,        const move_cube_update &args);
        void HANDLE_UPDATE(deck_shuffled,    const card_pile_type &pile);
        void HANDLE_UPDATE(show_card,        const show_card_update &args);
        void HANDLE_UPDATE(hide_card,        const hide_card_update &args);
        void HANDLE_UPDATE(tap_card,         const tap_card_update &args);
        void HANDLE_UPDATE(last_played_card, const card_id_args &args);
        void HANDLE_UPDATE(force_play_card,  const card_id_args &args);
        void HANDLE_UPDATE(player_add,       const player_user_update &args);
        void HANDLE_UPDATE(player_remove,    const player_remove_update &args);
        void HANDLE_UPDATE(player_hp,        const player_hp_update &args);
        void HANDLE_UPDATE(player_gold,      const player_gold_update &args);
        void HANDLE_UPDATE(player_show_role, const player_show_role_update &args);
        void HANDLE_UPDATE(player_status,     const player_status_update &args);
        void HANDLE_UPDATE(switch_turn,      const switch_turn_update &args);
        void HANDLE_UPDATE(request_status,   const request_status_args &args);
        void HANDLE_UPDATE(status_clear);
        void HANDLE_UPDATE(confirm_play);
        
        void pop_update();

        template<typename T, typename ... Ts>
        void add_animation(int duration, Ts && ... args) {
            m_animations.emplace_back(duration, std::in_place_type<T>, std::forward<Ts>(args) ... );
        }

        void move_player_views();

        void handle_card_click();
        void find_overlay();

        card_pile_view &get_pile(card_pile_type pile, int player_id = 0);

        card_textures m_card_textures;

        game_ui m_ui;
        target_finder m_target;

        std::deque<game_update> m_pending_updates;
        std::deque<animation> m_animations;

        counting_card_pile m_shop_deck;
        card_pile_view m_shop_discard;
        card_pile_view m_hidden_deck;
        flipped_card_pile m_shop_selection{options.shop_selection_width};
        wide_card_pile m_shop_choice{options.shop_choice_width};

        counting_card_pile m_main_deck;
        card_pile_view m_discard_pile;

        role_pile m_dead_roles_pile;

        card_pile_view m_scenario_deck;
        card_pile_view m_scenario_card;

        wide_card_pile m_selection{options.selection_width};

        card_pile_view m_specials;

        util::id_map<card_view> m_cards;
        util::id_map<player_view> m_players;
        util::id_map<cube_widget> m_cubes;
        util::id_map<role_card> m_role_cards;

        sdl::point m_mouse_pt;
        int m_mouse_motion_timer = 0;
        bool m_middle_click = false;
        card_view *m_overlay = nullptr;

        card_expansion_type m_expansions;
        
        int m_player_own_id = 0;
        int m_playing_id = 0;
        int m_request_origin_id = 0;
        int m_request_target_id = 0;
        
        std::default_random_engine rng;

        player_role m_winner_role = player_role::unknown;

        bool has_player_flags(player_flags flags) {
            if (player_view *p = find_player(m_player_own_id)) {
                return p->has_player_flags(flags);
            }
            return false;
        }

        card_view *find_card(int id) {
            if (auto it = m_cards.find(id); it != m_cards.end()) {
                return &*it;
            }
            return nullptr;
        }

        player_view *find_player(int id) {
            if (auto it = m_players.find(id); it != m_players.end()) {
                return &*it;
            }
            return nullptr;
        }

        std::string evaluate_format_string(const game_formatted_string &str);

        friend class game_ui;
        friend class target_finder;
    };

}

#endif