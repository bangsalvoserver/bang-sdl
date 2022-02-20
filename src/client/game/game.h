#ifndef __SCENE_GAME_H__
#define __SCENE_GAME_H__

#include "../scenes/scene_base.h"

#include "animation.h"
#include "game_ui.h"

#include "target_finder.h"

#include <list>
#include <optional>
#include <random>

#define HANDLE_UPDATE(name, ...) handle_game_update(enums::enum_constant<game_update_type::name> __VA_OPT__(,) __VA_ARGS__)

namespace banggame {

    class game_scene : public scene_base {
    public:
        game_scene(class game_manager *parent);

        void init(const game_started_args &args);
        
        void resize(int width, int height) override;
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
        void HANDLE_UPDATE(player_hp,        const player_hp_update &args);
        void HANDLE_UPDATE(player_gold,      const player_gold_update &args);
        void HANDLE_UPDATE(player_clear_characters, const player_clear_characters_update &args);
        void HANDLE_UPDATE(player_show_role, const player_show_role_update &args);
        void HANDLE_UPDATE(player_status,     const player_status_update &args);
        void HANDLE_UPDATE(switch_turn,      const switch_turn_update &args);
        void HANDLE_UPDATE(request_status,   const request_status_args &args);
        void HANDLE_UPDATE(request_respond,   const request_respond_args &args);
        void HANDLE_UPDATE(status_clear);
        void HANDLE_UPDATE(confirm_play);
        
        void pop_update();

        void move_player_views();

        void handle_card_click();
        void find_overlay();

        card_textures m_card_textures;

        game_ui m_ui;
        target_finder m_target;

        std::list<game_update> m_pending_updates;
        std::list<animation> m_animations;

        card_pile_view m_shop_deck;
        card_pile_view m_shop_discard;
        card_pile_view m_hidden_deck;
        card_pile_view m_shop_selection{options::shop_selection_width, true};
        card_pile_view m_shop_choice{options::shop_choice_width};

        card_pile_view m_main_deck;
        card_pile_view m_discard_pile;

        widgets::stattext m_main_deck_count;
        void update_main_deck_count();

        card_pile_view m_scenario_deck;
        card_pile_view m_scenario_card;

        card_pile_view m_selection{options::selection_width};

        card_pile_view m_specials;

        std::map<int, card_view> m_cards;
        std::map<int, player_view> m_players;
        std::map<int, cube_widget> m_cubes;

        sdl::point m_mouse_pt;
        int m_mouse_motion_timer = 0;
        bool m_middle_click = false;
        card_view *m_overlay = nullptr;

        card_expansion_type m_expansions;
        
        int m_player_own_id = 0;
        int m_playing_id = 0;
        
        std::default_random_engine rng;

        std::optional<request_status_args> m_current_request;
        player_role m_winner_role = player_role::unknown;

        bool has_player_flags(player_flags flags) {
            if (player_view *p = find_player(m_player_own_id)) {
                return p->has_player_flags(flags);
            }
            return false;
        }

        card_view *find_card(int id) {
            if (auto it = m_cards.find(id); it != m_cards.end()) {
                return &it->second;
            }
            return nullptr;
        }

        player_view *find_player(int id) {
            if (auto it = m_players.find(id); it != m_players.end()) {
                return &it->second;
            }
            return nullptr;
        }

        std::string evaluate_format_string(const game_formatted_string &str);

        friend class game_ui;
        friend class target_finder;
    };

}

#endif