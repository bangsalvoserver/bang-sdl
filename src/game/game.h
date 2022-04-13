#ifndef __GAME_H__
#define __GAME_H__

#include "events.h"
#include "game_table.h"
#include "request_queue.h"
#include "make_all_cards.h"

namespace banggame {

    struct game_options {
        card_expansion_type expansions = enums::flags_all<card_expansion_type>;
    };

    struct draw_check_handler {
        draw_check_function function;
        player *origin = nullptr;
        card *origin_card = nullptr;
    };

    using card_disabler_fun = std::function<bool(card *)>;

    struct game : game_table, event_handler_map, request_queue<game> {
        game_options m_options;
        bool m_game_over = false;

        std::optional<draw_check_handler> m_current_check;
        card_multimap<card_disabler_fun> m_disablers;

        player *m_playing = nullptr;

        player *find_disconnected_player();

        std::vector<game_update> get_game_state_updates(player *owner);

        void start_game(const game_options &options, const all_cards_t &all_cards);

        bool has_expansion(card_expansion_type type) const {
            using namespace enums::flag_operators;
            return bool(m_options.expansions & type);
        }

        request_status_args make_request_update(player *p);
        void send_request_update();

        void draw_check_then(player *origin, card *origin_card, draw_check_function fun);
        void do_draw_check();

        void add_disabler(event_card_key key, card_disabler_fun &&fun);
        void remove_disablers(event_card_key key);
        bool is_disabled(card *target_card) const;

        void check_game_over(player *killer, player *target);
        void player_death(player *killer, player *target);
    };

}

#endif