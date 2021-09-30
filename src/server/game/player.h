#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <vector>
#include <algorithm>

#include "card.h"

namespace banggame {

    struct game;
    struct player;

    struct player : public util::id_counter<player> {
        game *m_game;

        std::vector<deck_card> m_hand;
        std::vector<deck_card> m_table;
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
        bool m_dead = false;

        int m_infinite_bangs = 0;
        int m_calumets = 0;

        int m_bangs_played = 0;
        int m_bangs_per_turn = 1;

        int m_bang_strength = 1;
        int m_beer_strength = 1;

        int m_character_usages = 0;

        int m_num_checks = 1;
        int m_num_drawn_cards = 2;

        std::vector<int> m_max_cards_mods;

        bool m_has_drawn = false;

        explicit player(game *game) : m_game(game) {}

        void equip_card(deck_card &&target);
        bool has_card_equipped(const std::string &name) const;

        void discard_weapon();

        deck_card &find_hand_card(int card_id);
        deck_card &find_table_card(int card_id);
        deck_card &random_hand_card();
        card &find_any_card(int card_id);

        bool is_hand_empty() const {
            return m_hand.empty();
        }
        
        deck_card get_card_removed(int card_id);
        deck_card &discard_card(int card_id);
        void steal_card(player *target, int card_id);

        int num_hand_cards() const {
            return m_hand.size();
        }

        int max_cards_end_of_turn() const {
            return m_max_cards_mods.empty() ? m_hp : std::ranges::min(m_max_cards_mods);
        }

        bool alive() const { return !m_dead; }

        void damage(player *source, int value);
        void heal(int value);

        bool immune_to(const deck_card &c) {
            return m_calumets > 0 && c.suit == card_suit_type::diamonds;
        }

        bool can_play_bang() const {
            return m_infinite_bangs > 0 || m_bangs_played < m_bangs_per_turn;
        }
        
        void handle_death();
        void discard_all();

        void add_predraw_check(int card_id, int priority) {
            m_predraw_checks.emplace_back(card_id, priority);
        }

        void remove_predraw_check(int card_id) {
            m_predraw_checks.erase(std::ranges::find(m_predraw_checks, card_id, &predraw_check_t::card_id));
        }

        bool is_top_predraw_check(const deck_card &e) {
            int top_priority = std::ranges::max(m_pending_predraw_checks | std::views::transform(&predraw_check_t::priority));
            auto it = std::ranges::find(m_pending_predraw_checks, e.id, &predraw_check_t::card_id);
            return it != m_pending_predraw_checks.end() && it->priority == top_priority;
        }

        void next_predraw_check(int card_id);

        void set_character_and_role(const character &c, player_role role);

        void add_to_hand(deck_card &&c);

        bool verify_equip_target(const card &c, const std::vector<play_card_target> &targets);
        bool verify_card_targets(const card &c, const std::vector<play_card_target> &targets);
        void do_play_card(int card_id, const std::vector<play_card_target> &targets);

        void play_card(const play_card_args &args);
        void respond_card(const play_card_args &args);

        void draw_from_deck();
        void start_of_turn();
        void end_of_turn();
    };

}

#endif