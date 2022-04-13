#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <map>
#include <vector>
#include <algorithm>
#include <functional>

#include "game_update.h"

namespace banggame {

    struct game;
    struct player;
    
    struct card : card_data {
        int8_t usages = 0;
        bool inactive = false;
        std::vector<int> cubes;

        pocket_type pocket = pocket_type::none;
        player *owner = nullptr;
    };

    using draw_check_function = std::function<void(card *drawn_card)>;

    struct player {
        game *m_game;
        int id;
        int client_id = 0;

        std::vector<card *> m_hand;
        std::vector<card *> m_table;
        std::vector<card *> m_characters;
        std::vector<card *> m_backup_character;

        player_role m_role;

        struct predraw_check {
            int priority;
            bool resolved;
            draw_check_function check_fun;
        };

        std::map<card *, predraw_check> m_predraw_checks;

        std::optional<std::function<void()>> m_prompt;

        int8_t m_range_mod = 0;
        int8_t m_weapon_range = 1;
        int8_t m_distance_mod = 0;

        int8_t m_hp = 0;
        int8_t m_max_hp = 0;

        int8_t m_bangs_played = 0;
        int8_t m_bangs_per_turn = 1;

        int8_t m_num_checks = 1;
        
        int8_t m_num_cards_to_draw = 2;
        int8_t m_num_drawn_cards = 0;
        
        int8_t m_extra_turns = 0;

        card *m_last_played_card = nullptr;
        card *m_forced_card = nullptr;
        card *m_mandatory_card = nullptr;

        player_flags m_player_flags{};

        int8_t m_gold = 0;

        player(game *game, int id) : m_game(game), id(id) {}

        void equip_card(card *card);
        card *find_equipped_card(card *card);

        void equip_if_enabled(card *target_card);
        void unequip_if_enabled(card *target_card);

        card *random_hand_card();

        card *chosen_card_or(card *c);

        void add_cubes(card *target, int ncubes);
        void pay_cubes(card *target, int ncubes);
        void move_cubes(card *origin, card *target, int ncubes);
        void drop_all_cubes(card *target);

        bool can_receive_cubes() const;

        bool can_escape(player *origin, card *origin_card, effect_flags flags) const;
        
        void add_to_hand(card *card);
        
        std::vector<card *>::iterator move_card_to(card *card, pocket_type pocket,
            bool known = false, player *owner = nullptr, show_card_flags flags = {});

        void discard_card(card *card);
        void steal_card(player *target, card *card);

        int get_initial_cards();

        int max_cards_end_of_turn();

        bool alive() const;

        void damage(card *origin_card, player *source, int value, bool is_bang = false, bool instant = false);

        void heal(int value);

        void add_gold(int amount);

        bool immune_to(card *c);
        bool can_respond_with(card *c);
        
        void discard_all();

        void add_predraw_check(card *target_card, int priority, draw_check_function &&fun) {
            m_predraw_checks.try_emplace(target_card, priority, false, std::move(fun));
        }

        void remove_predraw_check(card *target_card) {
            m_predraw_checks.erase(target_card);
        }

        void next_predraw_check(card *target_card);

        void set_role(player_role role);

        void set_last_played_card(card *c);

        bool is_possible_to_play(card *c);
        void set_forced_card(card *c);
        void set_mandatory_card(card *c);

        bool is_bangcard(card *card_ptr);

        void verify_modifiers(card *c, const std::vector<card *> &modifiers);
        void play_modifiers(const std::vector<card *> &modifiers);

        void verify_effect_player_target(target_player_filter filter, player *target);
        void verify_effect_card_target(const effect_holder &effect, player *target, card *target_card);

        void verify_equip_target(card *c, const std::vector<play_card_target> &targets);
        void verify_card_targets(card *c, bool is_response, const std::vector<play_card_target> &targets);

        void play_card_action(card *c);
        void log_played_card(card *c, bool is_response);
        void do_play_card(card *c, bool is_response, const std::vector<play_card_target> &targets);

        void pick_card(const pick_card_args &args);
        void play_card(const play_card_args &args);
        void respond_card(const play_card_args &args);

        void check_prompt(card *c, bool is_response, const std::vector<play_card_target> &targets, std::function<void()> &&fun);
        void check_prompt_equip(card *c, player *target, std::function<void()> &&fun);
        void prompt_response(bool response);

        void draw_from_deck();

        void start_of_turn();
        void request_drawing();
        
        void verify_pass_turn();
        void pass_turn();
        void skip_turn();

        card_sign get_card_sign(card *target_card);

        void send_player_status();
        void add_player_flags(player_flags flags);
        void remove_player_flags(player_flags flags);
        bool check_player_flags(player_flags flags) const;

        int count_cubes() const;

        void untap_inactive_cards();
    };

}

#endif