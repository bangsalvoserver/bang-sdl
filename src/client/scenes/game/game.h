#ifndef __SCENE_GAME_H__
#define __SCENE_GAME_H__

#include "../scene_base.h"

#include "common/responses.h"

#include "animation.h"
#include "game_ui.h"

#include <list>

namespace banggame {

    struct response_view {
        response_type type = response_type::none;
        int origin_id;
        int target_id;
    };

    class game_scene : public scene_base {
    public:
        game_scene(class game_manager *parent);

        SDL_Color bg_color() override {
            return {0x07, 0x63, 0x25, 0xff};
        }
        
        void resize(int width, int height) override;
        void render(sdl::renderer &renderer) override;
        void handle_event(const SDL_Event &event) override;

        void handle_update(const game_update &update);
        
        void add_chat_message(const lobby_chat_args &args);
        
        void show_error(const std::string &message);

    private:
        void handle_update(enums::enum_constant<game_update_type::game_over>,        const game_over_update &args);
        void handle_update(enums::enum_constant<game_update_type::add_cards>,        const add_cards_update &args);
        void handle_update(enums::enum_constant<game_update_type::move_card>,        const move_card_update &args);
        void handle_update(enums::enum_constant<game_update_type::deck_shuffled>);
        void handle_update(enums::enum_constant<game_update_type::show_card>,        const show_card_update &args);
        void handle_update(enums::enum_constant<game_update_type::hide_card>,        const hide_card_update &args);
        void handle_update(enums::enum_constant<game_update_type::tap_card>,         const tap_card_update &args);
        void handle_update(enums::enum_constant<game_update_type::player_add>,       const player_id_update &args);
        void handle_update(enums::enum_constant<game_update_type::player_own_id>,    const player_id_update &args);
        void handle_update(enums::enum_constant<game_update_type::player_hp>,        const player_hp_update &args);
        void handle_update(enums::enum_constant<game_update_type::player_character>, const player_character_update &args);
        void handle_update(enums::enum_constant<game_update_type::player_show_role>, const player_show_role_update &args);
        void handle_update(enums::enum_constant<game_update_type::switch_turn>,      const switch_turn_update &args);
        void handle_update(enums::enum_constant<game_update_type::response_handle>,  const response_handle_update &args);
        void handle_update(enums::enum_constant<game_update_type::response_done>);
        
        void pop_update();

        template<game_action_type T, typename ... Ts>
        void add_action(Ts && ... args);

        void add_pass_action();
        void add_resolve_action();

        void move_player_views();

        void handle_card_click(const SDL_Point &mouse_pt);

        void on_click_main_deck();
        void on_click_discard_pile();
        void on_click_temp_table_card(int card_id);
        void on_click_table_card(int player_id, int card_id);
        void on_click_hand_card(int player_id, int card_id);
        void on_click_character(int player_id, int card_id);
        void on_click_player(int player_id);

        void handle_auto_targets(bool is_response);
        void add_card_targets(bool is_response, const std::vector<target_card_id> &targets);
        void add_player_targets(bool is_response, const std::vector<target_player_id> &targets);
        void clear_targets();

        game_ui m_ui;

        std::list<game_update> m_pending_updates;
        std::list<animation> m_animations;

        card_pile_view main_deck{0};

        card_pile_view discard_pile{0};
        card_pile_view temp_table;

        std::map<int, card_view> m_cards;
        std::map<int, player_view> m_players;

        card_view &get_card(int id) {
            auto it = m_cards.find(id);
            if (it == m_cards.end()) throw std::runtime_error("ID carta non trovato");
            return it->second;
        }

        card_widget_base &get_card_widget(int id) {
            if (auto it = m_cards.find(id); it != m_cards.end()) {
                return it->second;
            } else if (auto it = std::ranges::find(m_players, id, [](const decltype(m_players)::value_type &pair) {
                return pair.second.m_character.id;
            }); it != m_players.end()) {
                return it->second.m_character;
            } else {
                throw std::runtime_error("ID carta non trovato");
            }
        }

        player_view &get_player(int id) {
            auto it = m_players.find(id);
            if (it == m_players.end()) throw std::runtime_error("ID Giocatore non trovato");
            return it->second;
        }

        int m_player_own_id = 0;

        int m_playing_id;

        play_card_args m_play_card_args;
        std::vector<int> m_highlights;

        response_view m_current_response;

        friend class game_ui;
    };

}

#endif