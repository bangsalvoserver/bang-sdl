#ifndef __GAME_TABLE_H__
#define __GAME_TABLE_H__

#include <stdexcept>

#include "player.h"
#include "format_str.h"

#include "utils/id_map.h"

namespace banggame {

    struct game_error : std::exception, game_formatted_string {
        using game_formatted_string::game_formatted_string;

        const char *what() const noexcept override {
            return format_str.c_str();
        }
    };

    struct game_table {
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
        
        card *find_card(int card_id) {
            if (auto it = m_cards.find(card_id); it != m_cards.end()) {
                return &*it;
            }
            throw game_error("server.find_card: ID not found"_nonloc);
        }

        player *find_player(int player_id) {
            if (auto it = m_players.find(player_id); it != m_players.end()) {
                return &*it;
            }
            throw game_error("server.find_player: ID not found"_nonloc);
        }
        
        std::vector<card *> &get_pile(card_pile_type pile, player *owner = nullptr) {
            switch (pile) {
            case card_pile_type::player_hand:       return owner->m_hand;
            case card_pile_type::player_table:      return owner->m_table;
            case card_pile_type::player_character:  return owner->m_characters;
            case card_pile_type::player_backup:     return owner->m_backup_character;
            case card_pile_type::main_deck:         return m_deck;
            case card_pile_type::discard_pile:      return m_discards;
            case card_pile_type::selection:         return m_selection;
            case card_pile_type::shop_deck:         return m_shop_deck;
            case card_pile_type::shop_selection:    return m_shop_selection;
            case card_pile_type::shop_discard:      return m_shop_discards;
            case card_pile_type::hidden_deck:       return m_hidden_deck;
            case card_pile_type::scenario_deck:     return m_scenario_deck;
            case card_pile_type::scenario_card:     return m_scenario_cards;
            case card_pile_type::specials:          return m_specials;
            default: throw std::runtime_error("Invalid Pile");
            }
        }

        player *get_next_player(player *p) {
            auto it = m_players.find(p->id);
            do {
                if (++it == m_players.end()) it = m_players.begin();
            } while(!it->alive());
            return &*it;
        }

        int calc_distance(player *from, player *to) {
            if (from == to) return 0;
            if (from->check_player_flags(player_flags::disable_player_distances)) return to->m_distance_mod;
            if (from->check_player_flags(player_flags::see_everyone_range_1)) return 1;
            int d1=0, d2=0;
            for (player *counter = from; counter != to; counter = get_next_player(counter), ++d1);
            for (player *counter = to; counter != from; counter = get_next_player(counter), ++d2);
            return std::min(d1, d2) + to->m_distance_mod;
        }

        int num_alive() const {
            return std::ranges::count_if(m_players, &player::alive);
        }
    };

}

#endif