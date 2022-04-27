#ifndef __GAME_TABLE_H__
#define __GAME_TABLE_H__

#include <random>

#include "player.h"
#include "formatter.h"
#include "game_net.h"
#include "events.h"

#include "utils/id_map.h"

namespace banggame {

    DEFINE_ENUM_FLAGS(scenario_flags,
        (invert_rotation) // inverti giro
        (ghosttown) // citta' fantasma
        (judge) // non si puo' equipaggiare
        (abandonedmine) // fase 1 : pesca dagli scarti, fase 3 : scarta coperto nel mazzo
        (deadman) // il primo morto ritorna in vita con 2 carte e 2 hp nel suo turno
    )

    using card_disabler_fun = std::function<bool(card *)>;

    struct game_table : game_net_manager {
        std::default_random_engine rng;

        util::id_map<card> m_cards;
        util::id_map<player> m_players;
        
        std::vector<card *> m_deck;
        std::vector<card *> m_discards;
        std::vector<card *> m_selection;

        std::vector<card *> m_shop_deck;
        std::vector<card *> m_shop_discards;
        std::vector<card *> m_hidden_deck;
        std::vector<card *> m_shop_selection;
        std::vector<card *> m_specials;

        std::vector<card *> m_scenario_deck;
        std::vector<card *> m_scenario_cards;
        
        std::vector<int> m_cubes;

        scenario_flags m_scenario_flags{};
        game_options m_options;

        player *m_first_player = nullptr;
        player *m_first_dead = nullptr;

        card_multimap<card_disabler_fun> m_disablers;

        game_table() {
            std::random_device rd;
            rng.seed(rd());
        }
        
        card *find_card(int card_id);
        player *find_player(int player_id);
        
        std::vector<card *> &get_pocket(pocket_type pocket, player *owner = nullptr);

        player *get_next_player(player *p);
        player *get_next_in_turn(player *p);

        int calc_distance(player *from, player *to);

        int num_alive() const;

        bool has_scenario(scenario_flags type) const;

        void shuffle_cards_and_ids(std::vector<card *> &vec);

        void send_card_update(card *c, player *owner = nullptr, show_card_flags flags = {});

        void move_card(card *c, pocket_type pocket, player *owner = nullptr, show_card_flags flags = {});
        card *draw_card_to(pocket_type pocket, player *owner = nullptr, show_card_flags flags = {});
        card *draw_phase_one_card_to(pocket_type pocket, player *owner = nullptr, show_card_flags flags = {});
        card *phase_one_drawn_card();

        card *draw_shop_card();
        
        void draw_scenario_card();

        void add_disabler(event_card_key key, card_disabler_fun &&fun);
        void remove_disablers(event_card_key key);
        bool is_disabled(card *target_card) const;

        bool has_expansion(card_expansion_type type) const {
            using namespace enums::flag_operators;
            return bool(m_options.expansions & type);
        }
    };

}

#endif