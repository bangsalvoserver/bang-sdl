#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <vector>
#include <algorithm>

#include "card.h"

namespace banggame {

    struct game;

    class player {
    private:
        game *m_game;

        std::vector<card> m_hand;
        std::vector<card> m_table;

        using predraw_check_t = std::pair<card_effect *, int>;
        std::vector<predraw_check_t> m_predraw_checks;

        int m_range = 1;
        int m_weapon_range = 1;
        int m_distance_mod = 0;

        unsigned int m_hp = 0;
        unsigned int m_max_hp = 0;

        int m_infinite_bangs = 0;

    public:
        player(game *game) : m_game(game) {}

        void equip_card(card &&target);
        void discard_weapon();

        card get_card_removed(card *target);
        card &discard_card(card *target);
        void steal_card(player *target, card *target_card);

        void set_weapon_range(int range) {
            m_weapon_range = range;
        }

        void play_card(card *card, player *target);

        void set_hp(int value) {
            m_hp = value;
        }

        void damage(int value);
        void heal(int value);

        void add_distance(int diff) {
            m_distance_mod += diff;
        }

        void add_range(int diff) {
            m_range += diff;
        }

        void add_infinite_bangs(int value) {
            m_infinite_bangs += value;
        }

        game *get_game() {
            return m_game;
        }

        void add_predraw_check(card_effect *effect, int priority) {
            m_predraw_checks.emplace_back(effect, priority);
        }

        void remove_predraw_check(card_effect *effect) {
            m_predraw_checks.erase(std::ranges::find(m_predraw_checks, effect, &predraw_check_t::first));
        }

        void draw_card();

        player &get_next_player();
    };

    struct player_card {
        player *player;
        card *card;
    };

}

#endif