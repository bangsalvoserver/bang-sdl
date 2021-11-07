#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <list>
#include <vector>
#include <algorithm>
#include <optional>

#include "card.h"

namespace banggame {

    struct game;
    
    using draw_check_function = std::function<void(card_suit_type, card_value_type)>;
    using bang_modifier = std::function<void(request_bang &req)>;
    
    struct virtual_card {
        card *corresponding_card = nullptr;
        card virtual_card;
    };

    struct player {
        game *m_game;
        int id;

        std::vector<card *> m_hand;
        std::vector<card *> m_table;
        std::vector<character *> m_characters;

        std::optional<virtual_card> m_virtual;
        player_role m_role;

        struct predraw_check {
            int priority;
            bool resolved;
            draw_check_function check_fun;
        };

        std::map<card *, predraw_check> m_predraw_checks;

        int m_range_mod = 0;
        int m_weapon_range = 1;
        int m_distance_mod = 0;

        int m_hp = 0;
        int m_max_hp = 0;
        int m_initial_cards = 0;
        bool m_dead = false;

        int m_infinite_bangs = 0;
        int m_calumets = 0;
        int m_belltower = 0;
        bool m_ghost = false;

        int m_bangs_played = 0;
        int m_bangs_per_turn = 1;

        std::list<bang_modifier> m_bang_mods;

        bool m_cant_play_missedcard = false;

        int m_beer_strength = 1;

        int m_num_checks = 1;
        
        int m_num_cards_to_draw = 2;
        int m_num_drawn_cards = 0;

        card *m_last_played_card = nullptr;

        int m_gold = 0;
        
        std::vector<int> m_max_cards_mods;
        std::multimap<card *, player *> m_current_card_targets;

        explicit player(game *game);

        card *equip_card(card *card);
        bool has_card_equipped(const std::string &name) const;

        card *random_hand_card();

        bool is_hand_empty() const {
            return m_hand.empty();
        }

        void add_cubes(card *target, int ncubes);
        void pay_cubes(card *target, int ncubes);
        void move_cubes(card *origin, card *target, int ncubes);
        void drop_all_cubes(card *target);

        bool can_receive_cubes() const;

        bool can_escape(card *card, effect_flags flags) const;
        
        card *add_to_hand(card *card);
        
        card *discard_card(card *card);
        card *steal_card(player *target, card *card);

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

        void damage(card *origin_card, player *source, int value, bool is_bang = false);
        void do_damage(card *origin_card, player *source, int value, bool is_bang = false);

        void heal(int value);

        void add_gold(int amount);

        bool immune_to(const card &c) {
            return m_calumets > 0 && c.suit == card_suit_type::diamonds;
        }

        bool can_play_bang() const {
            return m_infinite_bangs > 0 || m_bangs_played < m_bangs_per_turn;
        }

        void add_bang_mod(bang_modifier &&mod) {
            m_bang_mods.push_back(std::move(mod));
        }

        void apply_bang_mods(request_bang &req) {
            while (!m_bang_mods.empty()) {
                m_bang_mods.front()(req);
                m_bang_mods.pop_front();
            }
        }
        
        void discard_all();

        void add_predraw_check(card *target_card, int priority, draw_check_function &&fun) {
            m_predraw_checks.try_emplace(target_card, priority, false, std::move(fun));
        }

        void remove_predraw_check(card *target_card) {
            m_predraw_checks.erase(target_card);
        }

        predraw_check *get_if_top_predraw_check(card *target_card);

        void next_predraw_check(card *target_card);

        void set_character_and_role(character *c, player_role role);

        void verify_modifiers(card *c, const std::vector<int> &modifier_ids);
        void play_modifiers(const std::vector<int> &modifier_ids);
        void verify_equip_target(card *c, const std::vector<play_card_target> &targets);
        void verify_card_targets(card *c, bool is_response, const std::vector<play_card_target> &targets);
        void do_play_card(card *c, bool is_response, const std::vector<play_card_target> &targets);

        void play_card(const play_card_args &args);
        void respond_card(const play_card_args &args);

        void draw_from_deck();

        void start_of_turn();
        void pass_turn();
        void end_of_turn();

        void play_virtual_card(card *corresponding_card, card virtual_card);
    };

}

#endif