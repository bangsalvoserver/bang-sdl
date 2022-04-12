#ifndef __GAME_H__
#define __GAME_H__

#include <deque>
#include <vector>
#include <algorithm>
#include <functional>
#include <optional>
#include <random>

#include "make_all_cards.h"
#include "player.h"
#include "events.h"

#include "holders.h"
#include "formatter.h"

#include "game_table.h"
#include "game_net.h"
#include "request_queue.h"

namespace banggame {

    struct game_options {
        card_expansion_type expansions = enums::flags_all<card_expansion_type>;
    };

    struct draw_check_handler {
        draw_check_function function;
        player *origin = nullptr;
        card *origin_card = nullptr;
    };

    DEFINE_ENUM_FLAGS(scenario_flags,
        (invert_rotation) // inverti giro
        (ghosttown) // citta' fantasma
        (judge) // non si puo' equipaggiare
        (abandonedmine) // fase 1 : pesca dagli scarti, fase 3 : scarta coperto nel mazzo
        (deadman) // il primo morto ritorna in vita con 2 carte e 2 hp nel suo turno
    )

    using card_disabler_fun = std::function<bool(card *)>;

    struct game : game_table, game_net_manager, request_queue<game>, event_handler_map {
        std::default_random_engine rng;

        game_options m_options;
        bool m_game_over = false;

        std::optional<draw_check_handler> m_current_check;
        card_multimap<card_disabler_fun> m_disablers;
        scenario_flags m_scenario_flags{};

        player *m_playing = nullptr;
        player *m_first_player = nullptr;
        player *m_first_dead = nullptr;

        game() {
            std::random_device rd;
            rng.seed(rd());
        }

        player *find_disconnected_player();

        void shuffle_cards_and_ids(std::vector<card *> &vec);

        std::vector<game_update> get_game_state_updates(player *owner);

        void send_card_update(const card &c, player *owner = nullptr, show_card_flags flags = {});

        void start_game(const game_options &options, const all_cards_t &all_cards);

        bool has_expansion(card_expansion_type type) const {
            using namespace enums::flag_operators;
            return bool(m_options.expansions & type);
        }

        bool has_scenario(scenario_flags type) const {
            using namespace enums::flag_operators;
            return bool(m_scenario_flags & type);
        }

        request_status_args make_request_update(player *p);
        void update_request();

        std::vector<card *>::iterator move_to(card *c, card_pile_type pile, bool known = true, player *owner = nullptr, show_card_flags flags = {});
        card *draw_card_to(card_pile_type pile, player *owner = nullptr, show_card_flags flags = {});
        card *draw_phase_one_card_to(card_pile_type pile, player *owner = nullptr, show_card_flags flags = {});

        card *draw_shop_card();
        
        void draw_scenario_card();

        void draw_check_then(player *origin, card *origin_card, draw_check_function fun);
        void do_draw_check();

        void add_disabler(event_card_key key, card_disabler_fun &&fun);
        void remove_disablers(event_card_key key);
        bool is_disabled(card *target_card) const;

        player *get_next_in_turn(player *p);

        void check_game_over(player *killer, player *target);
        void player_death(player *killer, player *target);
    };

}

#endif