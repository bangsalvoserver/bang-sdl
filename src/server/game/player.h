#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <vector>
#include <algorithm>
#include <optional>

#include "card.h"

namespace banggame {

    struct game;
    struct player;

    struct player : public util::id_counter<player> {
        game *m_game;

        std::vector<deck_card> m_hand;
        std::vector<deck_card> m_table;
        std::vector<character> m_characters;

        std::optional<std::pair<int, deck_card>> m_virtual;
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
        int m_initial_cards = 0;
        bool m_dead = false;

        int m_infinite_bangs = 0;
        int m_calumets = 0;
        bool m_ghost = false;

        int m_bangs_played = 0;
        int m_bangs_per_turn = 1;
        bool m_cant_play_missedcard = false;

        int m_beer_strength = 1;

        int m_num_checks = 1;
        
        int m_num_cards_to_draw = 2;
        int m_num_drawn_cards = 0;

        int m_last_played_card = 0;

        std::vector<int> m_max_cards_mods;
        std::multimap<int, int> m_current_card_targets;

        explicit player(game *game) : m_game(game) {}

        void equip_card(deck_card &&target);
        bool has_card_equipped(const std::string &name) const;

        void discard_weapon(int card_id);

        deck_card &random_hand_card();
        deck_card &find_hand_card(int card_id);
        deck_card &find_table_card(int card_id);

        character &find_character(int card_id);

        bool is_hand_empty() const {
            return m_hand.empty();
        }
        
        deck_card &add_to_hand(deck_card &&c);
        
        deck_card &discard_card(int card_id);
        deck_card &steal_card(player *target, int card_id);

        int num_hand_cards() const {
            return m_hand.size();
        }

        int get_initial_cards() const {
            return m_initial_cards == 0 ? m_max_hp : m_initial_cards;
        }

        int max_cards_end_of_turn() const {
            return m_max_cards_mods.empty() ? m_hp : std::ranges::min(m_max_cards_mods);
        }

        bool alive() const { return !m_dead || m_ghost; }

        void damage(player *source, int value, bool is_bang = false);
        void heal(int value);

        bool immune_to(const deck_card &c) {
            return m_calumets > 0 && c.suit == card_suit_type::diamonds;
        }

        bool can_play_bang() const {
            return m_infinite_bangs > 0 || m_bangs_played < m_bangs_per_turn;
        }
        
        void discard_all();

        void add_predraw_check(int card_id, int priority) {
            auto it = std::ranges::find(m_predraw_checks, card_id, &predraw_check_t::card_id);
            if (it == m_predraw_checks.end()) {
                m_predraw_checks.emplace_back(card_id, priority);
            }
        }

        void remove_predraw_check(int card_id) {
            auto it = std::ranges::find(m_predraw_checks, card_id, &predraw_check_t::card_id);
            if (it != m_predraw_checks.end()) {
                m_predraw_checks.erase(it);
            }
        }

        bool is_top_predraw_check(const deck_card &e) {
            int top_priority = std::ranges::max(m_pending_predraw_checks | std::views::transform(&predraw_check_t::priority));
            auto it = std::ranges::find(m_pending_predraw_checks, e.id, &predraw_check_t::card_id);
            return it != m_pending_predraw_checks.end() && it->priority == top_priority;
        }

        void next_predraw_check(int card_id);

        void send_character_update(const character &c, int index);
        void set_character_and_role(character &&c, player_role role);

        bool verify_equip_target(const card &c, const std::vector<play_card_target> &targets);
        bool verify_card_targets(const card &c, bool is_response, const std::vector<play_card_target> &targets);
        void do_play_card(int card_id, bool is_response, const std::vector<play_card_target> &targets);

        void play_card(const play_card_args &args);
        void respond_card(const play_card_args &args);

        void draw_from_deck();

        void start_of_turn();
        void end_of_turn();

        void play_virtual_card(deck_card vcard);
    };

}

#endif