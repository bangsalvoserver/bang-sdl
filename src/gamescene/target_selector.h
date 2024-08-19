#ifndef __TARGET_SELECTOR_H__
#define __TARGET_SELECTOR_H__

#include "net/card_data.h"

namespace banggame {

    class target_selector {
    private:
        game_scene *m_game;

    public:
        target_selector(game_scene *m_game)
            : m_game{m_game} {}
        
        bool can_confirm() const;

        bool finished() const;

        bool is_card_clickable() const;

        void on_click_card(pocket_type pocket, player_ptr player, card_ptr card);

        void on_click_player(player_ptr player);

        void clear_targets();

        void clear_status();

        void handle_auto_select();

        void send_prompt_response(bool response);

        void set_response_cards(const request_status_args &args);

        void set_play_cards(const status_ready_args &args);
    };
}

#endif