#ifndef __GAME_TABLE_H__
#define __GAME_TABLE_H__

#include <stdexcept>
#include <random>

#include "player.h"
#include "formatter.h"
#include "game_net.h"

#include "utils/id_map.h"

namespace banggame {

    struct game_error : std::exception, game_formatted_string {
        using game_formatted_string::game_formatted_string;

        const char *what() const noexcept override {
            return format_str.c_str();
        }
    };

    DEFINE_ENUM_FLAGS(scenario_flags,
        (invert_rotation) // inverti giro
        (ghosttown) // citta' fantasma
        (judge) // non si puo' equipaggiare
        (abandonedmine) // fase 1 : pesca dagli scarti, fase 3 : scarta coperto nel mazzo
        (deadman) // il primo morto ritorna in vita con 2 carte e 2 hp nel suo turno
    )

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

        player *m_first_player = nullptr;
        player *m_first_dead = nullptr;

        game_table() {
            std::random_device rd;
            rng.seed(rd());
        }
        
        card *find_card(int card_id);
        player *find_player(int player_id);
        
        std::vector<card *> &get_pile(card_pile_type pile, player *owner = nullptr);

        player *get_next_player(player *p);
        player *get_next_in_turn(player *p);

        int calc_distance(player *from, player *to);

        int num_alive() const;

        bool has_scenario(scenario_flags type) const;

        void shuffle_cards_and_ids(std::vector<card *> &vec);

        void send_card_update(const card &c, player *owner = nullptr, show_card_flags flags = {});

        std::vector<card *>::iterator move_to(card *c, card_pile_type pile, bool known = true, player *owner = nullptr, show_card_flags flags = {});
        card *draw_card_to(card_pile_type pile, player *owner = nullptr, show_card_flags flags = {});
        card *draw_phase_one_card_to(card_pile_type pile, player *owner = nullptr, show_card_flags flags = {});

        card *draw_shop_card();
        
        void draw_scenario_card();
    };

}

#endif