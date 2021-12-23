#ifndef __SCENE_GAME_H__
#define __SCENE_GAME_H__

#include "../scene_base.h"

#include "animation.h"
#include "game_ui.h"

#include "target_finder.h"

#include <list>
#include <optional>
#include <random>

#define UPDATE_TAG(name) enums::enum_constant<game_update_type::name>

namespace banggame {

    class game_scene : public scene_base, private card_textures {
    public:
        game_scene(class game_manager *parent);

        sdl::color bg_color() override {
            return {0x07, 0x63, 0x25, 0xff};
        }

        void init(const game_started_args &args);
        
        void resize(int width, int height) override;
        void render(sdl::renderer &renderer) override;
        void handle_event(const sdl::event &event) override;

        void handle_game_update(const game_update &update) override;
        
        void add_chat_message(const std::string &message) override;
        
        void show_error(const std::string &message) override;

        void add_user(int id, const user_info &args) override;

        void remove_user(int id) override;

    private:
        void handle_game_update(UPDATE_TAG(game_over),        const game_over_update &args);
        void handle_game_update(UPDATE_TAG(game_log),         const game_log_update &args);
        void handle_game_update(UPDATE_TAG(add_cards),        const add_cards_update &args);
        void handle_game_update(UPDATE_TAG(move_card),        const move_card_update &args);
        void handle_game_update(UPDATE_TAG(add_cubes),        const add_cubes_update &args);
        void handle_game_update(UPDATE_TAG(move_cube),        const move_cube_update &args);
        void handle_game_update(UPDATE_TAG(deck_shuffled),    const card_pile_type &pile);
        void handle_game_update(UPDATE_TAG(virtual_card),     const virtual_card_update &args);
        void handle_game_update(UPDATE_TAG(show_card),        const show_card_update &args);
        void handle_game_update(UPDATE_TAG(hide_card),        const hide_card_update &args);
        void handle_game_update(UPDATE_TAG(tap_card),         const tap_card_update &args);
        void handle_game_update(UPDATE_TAG(player_add),       const player_user_update &args);
        void handle_game_update(UPDATE_TAG(player_hp),        const player_hp_update &args);
        void handle_game_update(UPDATE_TAG(player_gold),      const player_gold_update &args);
        void handle_game_update(UPDATE_TAG(player_add_character), const player_character_update &args);
        void handle_game_update(UPDATE_TAG(player_remove_character), const player_remove_character_update &args);
        void handle_game_update(UPDATE_TAG(player_show_role), const player_show_role_update &args);
        void handle_game_update(UPDATE_TAG(switch_turn),      const switch_turn_update &args);
        void handle_game_update(UPDATE_TAG(request_handle),   const request_view &args);
        void handle_game_update(UPDATE_TAG(status_clear));
        
        void pop_update();

        void move_player_views();

        void handle_card_click(const sdl::point &mouse_pt);
        void find_overlay(const sdl::point &mouse_pt);

        game_ui m_ui;
        target_finder m_target;

        std::list<game_update> m_pending_updates;
        std::list<animation> m_animations;

        card_pile_view m_shop_deck{0};
        card_pile_view m_shop_discard{0};
        card_pile_view m_hidden_deck{0};
        card_pile_view m_shop_selection{sizes::shop_selection_width, true};

        card_pile_view m_main_deck{0};
        card_pile_view m_discard_pile{0};

        card_pile_view m_scenario_deck{0};
        card_pile_view m_scenario_card{0};

        card_pile_view m_selection{sizes::selection_width};

        std::map<int, card_view> m_cards;
        std::map<int, player_view> m_players;
        std::map<int, cube_widget> m_cubes;

        card_widget *m_overlay = nullptr;

        card_expansion_type m_expansions;
        
        int m_player_own_id = 0;
        int m_playing_id = 0;
        
        std::default_random_engine rng;

        request_view m_current_request;

        card_view *find_card(int id) {
            if (auto it = m_cards.find(id); it != m_cards.end()) {
                return &it->second;
            }
            return nullptr;
        }

        card_widget *find_card_widget(int id) {
            if (auto it = m_cards.find(id); it != m_cards.end()) {
                return &it->second;
            }
            for (auto &[player_id, p] : m_players) {
                if (auto it = std::ranges::find(p.m_characters, id, &character_card::id); it != p.m_characters.end()) {
                    return &*it;
                }
            }
            return nullptr;
        }

        player_view *find_player(int id) {
            if (auto it = m_players.find(id); it != m_players.end()) {
                return &it->second;
            }
            return nullptr;
        }

        std::string get_card_name(card_widget *card);

        friend class game_ui;
        friend class target_finder;
    };

}

#endif