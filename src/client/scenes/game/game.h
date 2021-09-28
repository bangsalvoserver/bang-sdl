#ifndef __SCENE_GAME_H__
#define __SCENE_GAME_H__

#include "../scene_base.h"

#include "common/responses.h"

#include "animation.h"

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

    private:
        void handle_update(enums::enum_constant<game_update_type::game_notify>,      game_notify_type args);
        void handle_update(enums::enum_constant<game_update_type::add_cards>,        const add_cards_update &args);
        void handle_update(enums::enum_constant<game_update_type::move_card>,        const move_card_update &args);
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

        void move_player_views();

        std::list<game_update> m_pending_updates;
        std::list<animation> m_animations;
        std::vector<lobby_chat_args> m_messages;

        card_pile_view main_deck{0};

        card_pile_view discard_pile{0};
        card_pile_view temp_table;

        std::map<int, card_view> m_cards;
        std::map<int, player_view> m_players;

        card_view &get_card(int id) {
            auto it = m_cards.find(id);
            if (it == m_cards.end()) throw std::runtime_error("ID Carta non trovato");
            return it->second;
        }

        player_view &get_player(int id) {
            auto it = m_players.find(id);
            if (it == m_players.end()) throw std::runtime_error("ID Giocatore non trovato");
            return it->second;
        }

        int m_player_own_id = 0;

        int m_playing_id;

        response_view m_current_response;
    };

}

#endif