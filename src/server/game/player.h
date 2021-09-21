#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <vector>
#include <algorithm>

#include "card.h"

namespace banggame {

    struct game;
    struct player;

    class player : public util::id_counter<player> {
    private:
        game *m_game;

        std::vector<card> m_hand;
        std::vector<card> m_table;
        std::vector<int> m_inactive_cards;
        character m_character;
        player_role m_role;

        using predraw_check_t = std::pair<effect_holder, int>;
        std::vector<predraw_check_t> m_predraw_checks;
        std::vector<predraw_check_t> m_pending_predraw_checks;

        int m_range = 1;
        int m_weapon_range = 1;
        int m_distance_mod = 0;

        unsigned int m_hp = 0;
        unsigned int m_max_hp = 0;

        int m_infinite_bangs = 0;

        int m_num_checks = 1;

    public:
        explicit player(game *game) : m_game(game) {}

        void equip_card(card &&target);
        void unequip_card(card &target);

        void discard_weapon();

        void discard_hand_card(int index);
        
        card get_card_removed(card *target);
        card &discard_card(card *target);
        void steal_card(player *target, card *target_card);

        int num_hand_cards() const {
            return m_hand.size();
        }

        void set_weapon_range(int range) {
            m_weapon_range = range;
        }

        int get_hp() const { return m_hp; }
        bool alive() const { return m_hp > 0; }

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

        void add_num_checks(int diff) {
            m_num_checks += diff;
        }

        int get_num_checks() const {
            return m_num_checks;
        }
        
        void discard_all();

        game *get_game() {
            return m_game;
        }

        template<std::derived_from<card_effect> T>
        void add_predraw_check(int priority) {
            m_predraw_checks.emplace_back(effect_holder::make<T>(), priority);
        }

        template<std::derived_from<card_effect> T>
        void remove_predraw_check() {
            m_predraw_checks.erase(std::ranges::find_if(m_predraw_checks, &effect_holder::is<T>, &predraw_check_t::first));
        }

        void set_character_and_role(const character &c, player_role role);

        player_role role() const { return m_role; }

        void add_to_hand(card &&c);

        player &get_next_player();

        void do_play_card(card &c, const std::vector<play_card_target> &targets);

        void play_card(const play_card_args &args);
        void respond_card(const play_card_args &args);

        void start_of_turn();
        void end_of_turn();
    };

}

#endif