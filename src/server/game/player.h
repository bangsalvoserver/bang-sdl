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
        character m_character;
        player_role m_role;

        struct predraw_check_t {
            int card_id;
            int priority;
        };

        std::vector<predraw_check_t> m_predraw_checks;
        std::vector<predraw_check_t> m_pending_predraw_checks;

        int m_range_mod = 0;
        int m_weapon_range = 1;
        int m_distance_mod = 0;

        int m_hp = 0;
        int m_max_hp = 0;

        int m_infinite_bangs = 0;

        int m_num_checks = 1;

    public:
        explicit player(game *game) : m_game(game) {}

        void equip_card(card &&target);
        bool has_card_equipped(const std::string &name) const;

        void discard_weapon();

        card &get_hand_card(int card_id);
        card &get_table_card(int card_id);

        bool is_hand_empty() const {
            return m_hand.empty();
        }
        
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

        int get_distance() const {
            return m_distance_mod;
        }

        void add_range(int diff) {
            m_range_mod += diff;
        }

        int get_range() const {
            return m_range_mod;
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

        void add_predraw_check(int card_id, int priority) {
            m_predraw_checks.emplace_back(card_id, priority);
        }

        void remove_predraw_check(int card_id) {
            m_predraw_checks.erase(std::ranges::find(m_predraw_checks, card_id, &predraw_check_t::card_id));
        }

        bool is_top_predraw_check(const card &e) {
            int top_priority = std::ranges::max(m_pending_predraw_checks | std::views::transform(&predraw_check_t::priority));
            if (e.effects.empty()) return false;
            auto it = std::ranges::find(m_pending_predraw_checks, e.id, &predraw_check_t::card_id);
            return it != m_pending_predraw_checks.end() && it->priority == top_priority;
        }

        void next_predraw_check(int card_id);

        void set_character_and_role(const character &c, player_role role);

        player_role role() const { return m_role; }

        void add_to_hand(card &&c);

        bool verify_card_targets(const card &c, const std::vector<play_card_target> &targets);
        void do_play_card(card &c, const std::vector<play_card_target> &targets);

        void play_card(const play_card_args &args);
        void respond_card(const play_card_args &args);

        void start_of_turn();
        void end_of_turn();
    };

}

#endif