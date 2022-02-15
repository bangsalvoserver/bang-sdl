#ifndef __TARGET_FINDER_H__
#define __TARGET_FINDER_H__

#include "player.h"

#include "common/holders.h"

#include <vector>

namespace banggame {

    class game_scene;

    struct target_pair {
        player_view *player = nullptr;
        card_view *card = nullptr;
        bool auto_target = false;
    };

    struct target_status {
        card_view *m_playing_card = nullptr;
        std::vector<card_view *> m_modifiers;

        std::vector<std::vector<target_pair>> m_targets;
        std::vector<cube_widget *> m_selected_cubes;

        bool m_equipping = false;
        bool m_response = false;
    };

    class target_finder : private target_status {
    public:
        target_finder(game_scene *parent) : m_game(parent) {}
        
        void set_border_colors();

        bool is_current_player_targeted() const;
        bool can_respond_with(card_view *card) const;
        bool can_pick(card_pile_type pile, player_view *player, card_view *card) const;
        bool can_confirm() const;

        void set_response_highlights(const request_respond_args &args);
        void clear_status();
        void clear_targets();

        void set_last_played_card(card_view *card) {
            m_last_played_card = card;
        }

        void set_forced_card(card_view *card);

        void confirm_play();
        bool waiting_confirm() const {
            return m_waiting_confirm;
        }

        bool is_card_clickable() const;

        void on_click_discard_pile();
        void on_click_main_deck();
        void on_click_selection_card(card_view *card);
        void on_click_shop_card(card_view *card);
        void on_click_table_card(player_view *player, card_view *card);
        void on_click_hand_card(player_view *player, card_view *card);
        void on_click_character(player_view *player, card_view *card);
        void on_click_scenario_card(card_view *card);
        bool on_click_player(player_view *player);

        void on_click_confirm();

        bool is_playing_card(card_view *card) const {
            return m_playing_card == card;
        }
    
    private:
        bool is_bangcard(card_view *card);

        void add_modifier(card_view *card);
        bool verify_modifier(card_view *card);

        void handle_auto_targets();
        void add_card_target(target_pair target);
        void add_character_target(target_pair target);
        bool add_player_targets(const std::vector<target_pair> &targets);
        
        int calc_distance(player_view *from, player_view *to);

        const std::vector<card_target_data> &get_current_card_targets() const;
        const std::vector<card_target_data> &get_optional_targets() const;

        void send_pick_card(card_pile_type pile, player_view *player = nullptr, card_view *card = nullptr);
        void send_play_card();

        target_type get_target_type(int index);
        int num_targets_for(target_type type);
        int get_target_index();
        
    private:
        game_scene *m_game;

        std::vector<card_view *> m_response_highlights;
        std::vector<std::tuple<card_pile_type, player_view *, card_view *>> m_picking_highlights;

        card_view *m_last_played_card = nullptr;
        card_view *m_forced_card = nullptr;
        bool m_waiting_confirm = false;

        template<game_action_type T, typename ... Ts>
        void add_action(Ts && ... args);
    };

}

#endif