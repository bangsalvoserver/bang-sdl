#ifndef __GAME_H__
#define __GAME_H__

#include <vector>

#include "card.h"
#include "player.h"

namespace banggame {

    struct game {
        std::vector<card> m_deck;
        std::vector<card> m_discards;
        std::vector<player> m_players;

        card &add_to_discards(card &&c) {
            return m_discards.emplace_back(std::move(c));
        }

        card draw_card() {
            card c = std::move(m_deck.back());
            m_deck.pop_back();
            return c;
        }

        card *draw_check() {
            return &add_to_discards(draw_card());
        }

        card &add_to_deck(card &&c) {
            return m_deck.emplace_back(std::move(c));
        }
        
        int num_players() {
            return m_players.size();
        }

        player &get_next_player(player *p) {
            auto it = m_players.begin() + (p - m_players.data());
            if (++it == m_players.end()) it = m_players.begin();
            return *it;
        }
    };

}

#endif