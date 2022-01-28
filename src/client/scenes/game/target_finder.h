#ifndef __TARGET_FINDER_H__
#define __TARGET_FINDER_H__

#include "player.h"

#include "common/requests.h"

#include <vector>

namespace banggame {

    class game_scene;

    struct target_pair {
        player_view *player;
        card_view *card;
    };

    struct target_status {
        card_view *m_playing_card = nullptr;
        std::vector<card_view *> m_modifiers;

        std::vector<std::vector<target_pair>> m_targets;
        std::vector<cube_widget *> m_selected_cubes;

        play_card_flags m_flags = enums::flags_none<play_card_flags>;
    };

    class target_finder : private target_status {
    public:
        target_finder(game_scene *parent) : m_game(parent) {}
        
        void render(sdl::renderer &renderer);

        play_card_flags get_flags() const {
            return m_flags;
        }

        void on_click_main_deck();
        void on_click_selection_card(card_view *card);
        void on_click_shop_card(card_view *card);
        void on_click_table_card(player_view *player, card_view *card);
        void on_click_hand_card(player_view *player, card_view *card);
        void on_click_character(player_view *player, card_view *card);
        void on_click_scenario_card(card_view *card);
        bool on_click_player(player_view *player);

        void on_click_pass_turn();
        void on_click_resolve();

        void clear_targets();

        void set_response_highlights(const std::vector<int> &card_ids);

        bool is_playing_card(card_view *card) const {
            return m_playing_card == card;
        }
    
    private:
        void add_modifier(card_view *card);
        bool verify_modifier(card_view *card);

        void handle_auto_targets();
        void add_card_target(target_pair target);
        void add_character_target(target_pair target);
        bool add_player_targets(const std::vector<target_pair> &targets);
        
        int calc_distance(player_view *from, player_view *to);

        std::vector<card_target_data> &get_current_card_targets();
        std::vector<card_target_data> &get_optional_targets();

        void send_play_card();

        target_type get_target_type(int index);
        int num_targets_for(target_type type);
        int get_target_index();
        
    private:
        game_scene *m_game;

        std::vector<card_view *> m_response_highlights;

        template<game_action_type T, typename ... Ts>
        void add_action(Ts && ... args);
    };

}

#endif